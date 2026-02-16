#pragma once
// ============================================================
// TrassarV3 - Trwały zapis do NVS (Preferences)
// ============================================================

#include "config.h"

struct LifetimeStats {
    float totalDistance = 0;
    float totalArea = 0;
    uint32_t totalPaintTimeSec = 0;
};

class StorageManager {
public:
    void begin();

    // Kalibracja
    void saveCalibration(float pulsesPerMeter);
    float loadCalibration(bool& calibrated);

    // Statystyki lifetime
    void saveLifetimeStats(const LifetimeStats& stats);
    LifetimeStats loadLifetimeStats();

    // Ostatni wzorzec
    void saveLastPattern(PatternID pat);
    PatternID loadLastPattern();

    // Prog predkosci maks.
    void saveMaxSpeed(float kmh);
    float loadMaxSpeed();
};

extern StorageManager storage;
