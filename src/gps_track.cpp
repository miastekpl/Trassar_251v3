// ============================================================
// TrassarV3 - Zapis trasy GPS (GPX) podczas malowania
// v2.18.0 - Bufor punktow w PSRAM, eksport .gpx na karte SD
//
// Podczas malowania co GPX_RECORD_INTERVAL_MS (5s) zapisuje punkt
// {lat, lng, alt, speed, time} do bufora w PSRAM.
// Po STOP: zapis calej trasy jako /tracks/RRRRMMDD_HHMMSS.gpx na SD.
// Format GPX 1.1 — kompatybilny z Google Earth, QGIS, Strava itp.
// ============================================================

#include "gps_track.h"
#include "gps_handler.h"
#include "rtc_handler.h"
#include "report_logger.h"
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
        Serial.printf("[GPX] Bufor w PSRAM: %u punktow (%.1f KB)\n",
                      maxPoints, (float)(maxPoints * sizeof(GpxPoint)) / 1024.0f);
    } else {
        // Fallback: maly bufor w RAM (300 punktow = ~9.6 KB, ~25 min przy 5s)
        maxPoints = 300;
        buffer = (GpxPoint*)calloc(maxPoints, sizeof(GpxPoint));
        if (buffer) {
            Serial.printf("[GPX] Bufor w RAM (brak PSRAM): %u punktow\n", maxPoints);
        } else {
            maxPoints = 0;
            Serial.println("[GPX] BLAD: brak pamieci na bufor GPS track!");
        }
    }

    pointCount = 0;
    recording = false;
}

void GpsTrack::startRecording() {
    if (!buffer || maxPoints == 0) return;
    pointCount = 0;
    recording = true;
    lastRecordMs = 0;  // Wymusza natychmiastowy zapis pierwszego punktu
    Serial.println("[GPX] Nagrywanie trasy rozpoczete");
}

void GpsTrack::stopRecording() {
    if (!recording) return;
    recording = false;

    if (pointCount > 0) {
        if (writeGpxFile()) {
            Serial.printf("[GPX] Trasa zapisana: %u punktow\n", pointCount);
        } else {
            Serial.println("[GPX] BLAD zapisu pliku GPX!");
        }
    } else {
        Serial.println("[GPX] Brak punktow — plik GPX nie utworzony");
    }
    pointCount = 0;
}

void GpsTrack::cancelRecording() {
    recording = false;
    pointCount = 0;
    Serial.println("[GPX] Nagrywanie anulowane");
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
    if (pointCount >= maxPoints) {
        // Bufor pelny — nadpisz najstarsze (ring buffer uproszczony: po prostu nie zapisuj wiecej)
        // W praktyce 4320 punktow @ 5s = 6h — wystarczajaco duzo
        return;
    }

    GpxPoint& pt = buffer[pointCount];
    pt.lat   = gpsHandler.getLat();
    pt.lng   = gpsHandler.getLng();
    pt.alt   = (float)gpsHandler.getAltitude();
    pt.speed = gpsHandler.getGpsSpeed();

    // Czas z RTC (pewniejszy niz GPS time)
    DateTime now = rtcModule.now();
    pt.timeUtc = dateTimeToUnix(now.year(), now.month(), now.day(),
                                now.hour(), now.minute(), now.second());
    pt._pad = 0;

    pointCount++;
}

// ============================================================
// Zapis pliku GPX na karte SD
// ============================================================
bool GpsTrack::writeGpxFile() {
    if (!reportLogger.isReady()) {
        Serial.println("[GPX] Karta SD niedostepna");
        return false;
    }

    // Utworz katalog /tracks jesli nie istnieje
    if (!SD.exists("/tracks")) {
        SD.mkdir("/tracks");
    }

    // Nazwa pliku: /tracks/RRRRMMDD_HHMMSS.gpx
    DateTime now = rtcModule.now();
    char fname[40];
    snprintf(fname, sizeof(fname), "/tracks/%04d%02d%02d_%02d%02d%02d.gpx",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

    File f = SD.open(fname, FILE_WRITE);
    if (!f) {
        Serial.printf("[GPX] Nie mozna otworzyc: %s\n", fname);
        return false;
    }

    writeGpxHeader(f);

    for (uint16_t i = 0; i < pointCount; i++) {
        writeGpxPoint(f, buffer[i]);

        // Co 100 punktow: flush, zeby nie stracic danych przy utracie zasilania
        if (i % 100 == 99) f.flush();
    }

    writeGpxFooter(f);
    f.close();

    Serial.printf("[GPX] Zapisano: %s (%u punktow)\n", fname, pointCount);
    return true;
}

void GpsTrack::writeGpxHeader(File& f) {
    f.println(F("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    f.println(F("<gpx version=\"1.1\" creator=\"TrassarV3\""));
    f.println(F("  xmlns=\"http://www.topografix.com/GPX/1/1\">"));
    f.println(F("  <trk>"));
    f.print(F("    <name>Trassar "));

    // Data sesji w nazwie trasy
    if (pointCount > 0) {
        char timeBuf[24];
        unixToISO8601(buffer[0].timeUtc, timeBuf, sizeof(timeBuf));
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
