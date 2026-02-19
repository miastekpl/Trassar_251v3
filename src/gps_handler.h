#pragma once
// ============================================================
// TrassarV3 - Obsluga modulu GPS GY-NEO6MV2 (NEO-6M)
// UART2, biblioteka TinyGPS++
// ============================================================

#include "config.h"
#include <TinyGPSPlus.h>

class GpsHandler {
public:
    void begin();
    void update();  // Wywolywana w loop()

    bool hasFix() const { return gps.location.isValid() && gps.location.age() < 3000; }
    double getLat() const { return gps.location.lat(); }
    double getLng() const { return gps.location.lng(); }
    int getSatellites() const { return gps.satellites.isValid() ? (int)gps.satellites.value() : 0; }
    float getGpsSpeed() const { return gps.speed.isValid() ? (float)gps.speed.kmph() : 0; }
    double getHdop() const { return gps.hdop.isValid() ? gps.hdop.hdop() : 99.9; }
    uint32_t getCharsProcessed() const { return gps.charsProcessed(); }

private:
    TinyGPSPlus gps;
};

extern GpsHandler gpsHandler;
