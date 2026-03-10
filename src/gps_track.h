#pragma once
// ============================================================
// TrassarV3 - Zapis trasy GPS (GPX) podczas malowania
// v2.52.0 - Ring buffer w PSRAM, eksport .gpx + .geojson na SD
//           Bufor cykliczny — po zapelnieniu nadpisuje najstarsze
//           punkty, z ostrzezeniem na wyswietlaczu.
// ============================================================

#include "config.h"
#include <FS.h>

// Punkt trasy GPS (32 bajty — kompaktowy dla PSRAM)
struct GpxPoint {
    double lat;       // 8B  Szerokosc geograficzna
    double lng;       // 8B  Dlugosc geograficzna
    float  alt;       // 4B  Wysokosc [m] n.p.m.
    float  speed;     // 4B  Predkosc [km/h]
    uint32_t timeUtc; // 4B  Unix timestamp (sekundy od epoch)
    uint32_t _pad;    // 4B  Padding do 32B
};

class GpsTrack {
public:
    void begin();

    // Nagrywanie trasy (wywoływane z painting_engine / main loop)
    void startRecording();
    void stopRecording();       // Zatrzymaj + zapis GPX na SD
    void cancelRecording();     // Zatrzymaj bez zapisu
    void update();              // Sprawdz interwał i zapisz punkt

    bool isRecording() const {
        taskENTER_CRITICAL(&bufMux);
        bool r = recording;
        taskEXIT_CRITICAL(&bufMux);
        return r;
    }
    uint16_t getPointCount() const {
        taskENTER_CRITICAL(&bufMux);
        uint16_t c = pointCount;
        taskEXIT_CRITICAL(&bufMux);
        return c;
    }
    uint16_t getMaxPoints() const { return maxPoints; }
    bool isOverflowed() const {
        taskENTER_CRITICAL(&bufMux);
        bool o = overflowed;
        taskEXIT_CRITICAL(&bufMux);
        return o;
    }

private:
    mutable portMUX_TYPE bufMux = portMUX_INITIALIZER_UNLOCKED;

    bool recording = false;
    unsigned long lastRecordMs = 0;

    // Ring buffer w PSRAM (alokowany dynamicznie w begin())
    GpxPoint* buffer = nullptr;
    uint16_t pointCount = 0;   // Calkowita liczba zapisanych punktow
    uint16_t writeIdx = 0;     // Indeks zapisu (head ring bufora)
    uint16_t maxPoints = 0;
    bool psramOk = false;
    bool overflowed = false;   // Czy bufor sie zawinął

    void addPoint();

    // Ring buffer helpers
    const GpxPoint& getPoint(uint16_t logicalIdx) const;
    uint16_t getStoredCount() const;

    bool writeGpxFile(const char* path);
    void writeGpxHeader(File& f);
    void writeGpxPoint(File& f, const GpxPoint& pt);
    void writeGpxFooter(File& f);
    bool writeGeoJsonFile(const char* path);

    // Konwersja DateTime do unix timestamp
    static uint32_t dateTimeToUnix(int year, int month, int day,
                                   int hour, int minute, int second);
    // Konwersja unix timestamp do ISO 8601 string
    static void unixToISO8601(uint32_t ts, char* buf, size_t len);
};

extern GpsTrack gpsTrack;
