// ============================================================
// TrassarV3 - Zapis trasy GPS (GPX) podczas malowania
// v2.52.0 - Ring buffer w PSRAM, eksport .gpx + .geojson na SD
//
// Podczas malowania co GPX_RECORD_INTERVAL_MS (5s) zapisuje punkt
// {lat, lng, alt, speed, time} do ring bufora w PSRAM.
// Po zapelnieniu: nadpisuje najstarsze punkty (bufor cykliczny).
// Ostrzezenie na wyswietlaczu gdy bufor sie zawinął.
// Po STOP: zapis calej trasy jako .gpx i .geojson do /tracks/ na SD.
// GPX 1.1 — Google Earth, QGIS, Strava
// GeoJSON — systemy GIS, Leaflet, Mapbox, geojson.io
// ============================================================

#include "gps_track.h"
#include "gps_handler.h"
#include "rtc_handler.h"
#include "report_logger.h"
#include "sys_log.h"
#include "event_log.h"
#include <SD.h>
#include <esp_heap_caps.h>

GpsTrack gpsTrack;

void GpsTrack::begin() {
    // Alokuj bufor w PSRAM (jezeli dostepna)
    size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (psramFree > sizeof(GpxPoint) * 100) {
        maxPoints = GPX_MAX_POINTS;
        // Ogranicz do faktycznie dostepnej PSRAM (zostaw 64KB zapasu)
        size_t available = (psramFree > 65536) ? (psramFree - 65536) : 0;
        uint16_t maxFromRam = (uint16_t)(available / sizeof(GpxPoint));
        if (maxFromRam < maxPoints) maxPoints = maxFromRam;

        buffer = (GpxPoint*)heap_caps_calloc(maxPoints, sizeof(GpxPoint), MALLOC_CAP_SPIRAM);
        psramOk = (buffer != nullptr);
    }

    if (psramOk) {
        LOG_INFO("GPX", "Ring buffer w PSRAM: %u punktow (%.1f KB)",
                 maxPoints, (float)(maxPoints * sizeof(GpxPoint)) / 1024.0f);
    } else {
        // Fallback: maly bufor w RAM (300 punktow = ~9.6 KB, ~25 min przy 5s)
        maxPoints = 300;
        buffer = (GpxPoint*)calloc(maxPoints, sizeof(GpxPoint));
        if (buffer) {
            LOG_WARN("GPX", "Bufor w RAM (brak PSRAM): %u punktow", maxPoints);
        } else {
            maxPoints = 0;
            LOG_ERROR("GPX", "Brak pamieci na bufor GPS track!");
        }
    }

    pointCount = 0;
    writeIdx = 0;
    overflowed = false;
    recording = false;
}

void GpsTrack::startRecording() {
    if (!buffer || maxPoints == 0) return;
    taskENTER_CRITICAL(&bufMux);
    pointCount = 0;
    writeIdx = 0;
    overflowed = false;
    recording = true;
    taskEXIT_CRITICAL(&bufMux);
    lastRecordMs = 0;  // Wymusza natychmiastowy zapis pierwszego punktu
    LOG_INFO("GPX", "Nagrywanie trasy rozpoczete");
}

void GpsTrack::stopRecording() {
    taskENTER_CRITICAL(&bufMux);
    bool wasRecording = recording;
    recording = false;
    taskEXIT_CRITICAL(&bufMux);
    if (!wasRecording) return;

    uint16_t stored = getStoredCount();
    if (stored > 0) {
        if (!reportLogger.isReady()) {
            LOG_WARN("GPX", "Karta SD niedostepna — trasa utracona (%u pkt)", stored);
            pointCount = 0;
            writeIdx = 0;
            overflowed = false;
            return;
        }

        // Generuj spolna nazwe pliku (ten sam timestamp dla obu formatow)
        DateTime now = rtcModule.now();
        char gpxPath[48], geoPath[52];
        snprintf(gpxPath, sizeof(gpxPath), "/tracks/%04d%02d%02d_%02d%02d%02d.gpx",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second());
        snprintf(geoPath, sizeof(geoPath), "/tracks/%04d%02d%02d_%02d%02d%02d.geojson",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second());

        if (!SD_LOCK()) {
            LOG_ERROR("GPX", "Nie mozna zdobyc mutexu SD");
            pointCount = 0;
            writeIdx = 0;
            overflowed = false;
            return;
        }

        if (!SD.exists("/tracks")) {
            if (!SD.mkdir("/tracks")) {
                LOG_ERROR("GPX", "Nie mozna utworzyc /tracks");
            }
        }

        bool gpxOk = writeGpxFile(gpxPath);
        bool geoOk = writeGeoJsonFile(geoPath);
        SD_UNLOCK();

        LOG_INFO("GPX", "Trasa: %u pkt %s(GPX:%s GeoJSON:%s)",
                 stored, overflowed ? "[OVERFLOW] " : "",
                 gpxOk ? "OK" : "BLAD", geoOk ? "OK" : "BLAD");
    } else {
        LOG_INFO("GPX", "Brak punktow — pliki nie utworzone");
    }
    taskENTER_CRITICAL(&bufMux);
    pointCount = 0;
    writeIdx = 0;
    overflowed = false;
    taskEXIT_CRITICAL(&bufMux);
}

void GpsTrack::cancelRecording() {
    taskENTER_CRITICAL(&bufMux);
    recording = false;
    pointCount = 0;
    writeIdx = 0;
    overflowed = false;
    taskEXIT_CRITICAL(&bufMux);
    LOG_INFO("GPX", "Nagrywanie anulowane");
}

void GpsTrack::update() {
    if (!recording || !buffer) return;

    unsigned long now = millis();
    if (now - lastRecordMs < GPX_RECORD_INTERVAL_MS) return;
    lastRecordMs = now;

    // Zapisz punkt tylko gdy GPS ma fix
    if (gpsHandler.hasFix()) {
        addPoint();
    }
}

void GpsTrack::addPoint() {
    // Przygotuj dane punktu poza sekcja krytyczna
    GpxPoint pt;
    pt.lat   = gpsHandler.getLat();
    pt.lng   = gpsHandler.getLng();
    pt.alt   = (float)gpsHandler.getAltitude();
    pt.speed = gpsHandler.getGpsSpeed();

    // Czas z RTC (pewniejszy niz GPS time)
    DateTime now = rtcModule.now();
    pt.timeUtc = dateTimeToUnix(now.year(), now.month(), now.day(),
                                now.hour(), now.minute(), now.second());
    pt._pad = 0;

    bool justOverflowed = false;
    taskENTER_CRITICAL(&bufMux);
    buffer[writeIdx] = pt;
    writeIdx++;
    pointCount++;

    // Ring buffer: zawijanie po osiagnieciu maxPoints
    if (writeIdx >= maxPoints) {
        writeIdx = 0;
        if (!overflowed) {
            overflowed = true;
            justOverflowed = true;
        }
    }
    taskEXIT_CRITICAL(&bufMux);

    if (justOverflowed) {
        LOG_WARN("GPX", "Bufor GPS pelny (%u pkt) — nadpisywanie najstarszych", maxPoints);
        eventLog.logf("GPX", "Ring buffer overflow — najstarsze punkty nadpisywane (max=%u)", maxPoints);
    }
}

// Zwraca ilosc aktualnie przechowywanych punktow
uint16_t GpsTrack::getStoredCount() const {
    return overflowed ? maxPoints : writeIdx;
}

// Zwraca punkt z ring bufora (0 = najstarszy przechowywany)
const GpxPoint& GpsTrack::getPoint(uint16_t logicalIdx) const {
    if (overflowed) {
        // writeIdx wskazuje na najstarszy element (bo zostal nadpisany)
        uint16_t realIdx = (writeIdx + logicalIdx) % maxPoints;
        return buffer[realIdx];
    }
    return buffer[logicalIdx];
}

// ============================================================
// Zapis pliku GPX na karte SD
// ============================================================
bool GpsTrack::writeGpxFile(const char* path) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        LOG_ERROR("GPX", "Nie mozna otworzyc: %s", path);
        return false;
    }

    uint16_t stored = getStoredCount();
    writeGpxHeader(f);

    for (uint16_t i = 0; i < stored; i++) {
        writeGpxPoint(f, getPoint(i));

        // Co 50 punktow: flush + yield, zeby nie blokowac WDT i innych taskow
        if (i % 50 == 49) {
            f.flush();
            yield();
        }
    }

    writeGpxFooter(f);
    f.close();

    LOG_INFO("GPX", "Zapisano: %s (%u pkt)", path, stored);
    return true;
}

// ============================================================
// Zapis pliku GeoJSON na karte SD (LineString)
// Kompatybilny z Leaflet, Mapbox, QGIS, geojson.io
// ============================================================
bool GpsTrack::writeGeoJsonFile(const char* path) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        LOG_ERROR("GPX", "GeoJSON: nie mozna otworzyc: %s", path);
        return false;
    }

    uint16_t stored = getStoredCount();

    f.print(F("{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","));
    f.print(F("\"geometry\":{\"type\":\"LineString\",\"coordinates\":["));

    for (uint16_t i = 0; i < stored; i++) {
        const GpxPoint& pt = getPoint(i);
        char coord[48];
        snprintf(coord, sizeof(coord), "%s[%.7f,%.7f,%.1f]",
                 i > 0 ? "," : "",
                 pt.lng, pt.lat, pt.alt);
        f.print(coord);
        // Co 50 punktow: flush + yield, zeby nie blokowac WDT i innych taskow
        if (i % 50 == 49) {
            f.flush();
            yield();
        }
    }

    f.print(F("]},\"properties\":{\"name\":\"Trassar "));

    if (stored > 0) {
        char timeBuf[24];
        unixToISO8601(getPoint(0).timeUtc, timeBuf, sizeof(timeBuf));
        f.print(timeBuf);
    }

    f.print(F("\",\"creator\":\"TrassarV3\",\"points\":"));
    f.print(stored);
    f.println(F("}}]}"));

    f.close();
    LOG_INFO("GPX", "GeoJSON: %s (%u pkt)", path, stored);
    return true;
}

void GpsTrack::writeGpxHeader(File& f) {
    f.println(F("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    f.println(F("<gpx version=\"1.1\" creator=\"TrassarV3\""));
    f.println(F("  xmlns=\"http://www.topografix.com/GPX/1/1\">"));
    f.println(F("  <trk>"));
    f.print(F("    <name>Trassar "));

    // Data sesji w nazwie trasy
    uint16_t stored = getStoredCount();
    if (stored > 0) {
        char timeBuf[24];
        unixToISO8601(getPoint(0).timeUtc, timeBuf, sizeof(timeBuf));
        f.print(timeBuf);
    }
    f.println(F("</name>"));
    f.println(F("    <trkseg>"));
}

void GpsTrack::writeGpxPoint(File& f, const GpxPoint& pt) {
    // <trkpt lat="51.123456" lon="17.123456">
    //   <ele>120.5</ele>
    //   <time>2026-02-24T14:30:00Z</time>
    //   <speed>8.5</speed>
    // </trkpt>
    char buf[180];
    char timeBuf[24];
    unixToISO8601(pt.timeUtc, timeBuf, sizeof(timeBuf));

    snprintf(buf, sizeof(buf),
             "      <trkpt lat=\"%.7f\" lon=\"%.7f\">\n"
             "        <ele>%.1f</ele>\n"
             "        <time>%s</time>\n"
             "        <speed>%.1f</speed>\n"
             "      </trkpt>",
             pt.lat, pt.lng, pt.alt, timeBuf, pt.speed);
    f.println(buf);
}

void GpsTrack::writeGpxFooter(File& f) {
    f.println(F("    </trkseg>"));
    f.println(F("  </trk>"));
    f.println(F("</gpx>"));
}

// ============================================================
// Pomocnicze: konwersja czasu
// ============================================================

// Prosty Unix timestamp (bez stref czasowych — zakladamy UTC/lokalny RTC)
uint32_t GpsTrack::dateTimeToUnix(int year, int month, int day,
                                   int hour, int minute, int second) {
    // Uproszczona konwersja (poprawna dla lat 2000-2099)
    static const uint16_t daysBeforeMonth[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    uint32_t days = 0;
    // Lata od 1970
    for (int y = 1970; y < year; y++) {
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    days += daysBeforeMonth[month - 1];
    // Rok przestepny: dodaj 1 dzien po lutym
    if (month > 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
        days++;
    }
    days += day - 1;

    return days * 86400UL + hour * 3600UL + minute * 60UL + second;
}

void GpsTrack::unixToISO8601(uint32_t ts, char* buf, size_t len) {
    // Odwrotna konwersja: unix -> YYYY-MM-DDTHH:MM:SSZ
    uint32_t s = ts % 60; ts /= 60;
    uint32_t m = ts % 60; ts /= 60;
    uint32_t h = ts % 24; ts /= 24;

    // Dni od 1970-01-01
    uint32_t days = ts;
    int year = 1970;
    while (true) {
        uint32_t diy = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
        if (days < diy) break;
        days -= diy;
        year++;
    }

    static const uint16_t dpm[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    int month = 0;
    for (month = 0; month < 12; month++) {
        uint16_t dim = dpm[month];
        if (month == 1 && leap) dim++;
        if (days < dim) break;
        days -= dim;
    }

    snprintf(buf, len, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             year, month + 1, (int)days + 1,
             (int)h, (int)m, (int)s);
}
