#pragma once
// ============================================================
// TrassarV3 - Zapis trasy GPS (GPX) podczas malowania
// v2.19.0 - Bufor punktow w PSRAM, eksport .gpx + .geojson na SD
// ============================================================

#include "config.h"

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

    bool isRecording() const { return recording; }
    uint16_t getPointCount() const { return pointCount; }

private:
    bool recording = false;
    unsigned long lastRecordMs = 0;

    // Bufor w PSRAM (alokowany dynamicznie w begin())
    GpxPoint* buffer = nullptr;
    uint16_t pointCount = 0;
    uint16_t maxPoints = 0;
    bool psramOk = false;

    void addPoint();
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
