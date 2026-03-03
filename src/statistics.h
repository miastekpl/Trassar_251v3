#pragma once
// ============================================================
// TrassarV3 - Statystyki malowania
// ============================================================

#include "config.h"
#include "storage.h"

class StatisticsManager {
public:
    void begin();

    // Aktualizacja (wywoływana w każdym cyklu malowania)
    void updatePainting(float distanceDelta, const bool gunStates[NUM_GUNS]);

    // Sesja bieżąca
    void resetSession();
    // Reset wszystkich licznikow (sesja + lifetime + strzaly)
    void resetAll();
    float getSessionDistance() const { return sessionDistance; }
    float getSessionArea() const { return sessionArea; }
    unsigned long getSessionTimeSec() const;

    void startSessionTimer();
    void pauseSessionTimer();
    void resumeSessionTimer();

    // Lifetime
    float getLifetimeDistance() const { return lifetime.totalDistance; }
    float getLifetimeArea() const { return lifetime.totalArea; }
    uint32_t getLifetimePaintTimeSec() const { return lifetime.totalPaintTimeSec; }

    // Dystans per pistolet w sesji
    float getGunDistance(int gun) const { return gunDistances[gun]; }

    // Licznik strzalow pistoletow (lifetime)
    uint32_t getGunShotCount(int gun) const { return gunShotCounts[gun]; }

    // Zapis lifetime do NVS
    void saveLifetime();
    void loadLifetime();

    // Motogodziny (MTH) - calkowity czas pracy silnika
    void startMTH();
    void stopMTH();
    void saveMTH();
    void loadMTH();
    uint32_t getMTHSeconds() const;

private:
    // Sesja
    float sessionDistance = 0;
    float sessionArea = 0;
    float gunDistances[NUM_GUNS] = {};
    unsigned long sessionStartMs = 0;
    unsigned long sessionPausedMs = 0;
    unsigned long sessionAccumMs = 0;
    bool  sessionTimerRunning = false;

    // Lifetime
    LifetimeStats lifetime;

    // Licznik strzalow pistoletow (lifetime, zlicza tranzycje OFF->ON)
    uint32_t gunShotCounts[NUM_GUNS] = {};
    bool     gunWasOn[NUM_GUNS] = {};  // Stan poprzedni (do detekcji tranzycji)

    // Motogodziny (MTH)
    uint32_t mthTotalSec = 0;
    unsigned long mthStartMs = 0;
    bool mthRunning = false;
};

extern StatisticsManager stats;
