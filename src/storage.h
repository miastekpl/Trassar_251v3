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

    // Tryb pracy
    void saveMode(MachineMode mode);
    MachineMode loadMode();

    // Wzorzec wlasny (3 sloty)
    void saveCustomPattern(const CustomPatternCfg& cfg, int slot = 0);
    CustomPatternCfg loadCustomPattern(int slot = 0);

    // Licznik strzalow pistoletow (lifetime)
    void saveGunShotCounts(const uint32_t counts[NUM_GUNS]);
    void loadGunShotCounts(uint32_t counts[NUM_GUNS]);

    // Tryb przelaczania wzorcow (smart/instant)
    void saveSwitchMode(bool smart);
    bool loadSwitchMode();

private:
    void checkNvsVersion();
};

extern StorageManager storage;
