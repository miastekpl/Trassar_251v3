// ============================================================
// TrassarV3 - Statystyki malowania
// ============================================================

#include "statistics.h"
#include "storage.h"

StatisticsManager stats;

void StatisticsManager::begin() {
    resetSession();
    loadLifetime();
    loadMTH();
    storage.loadGunShotCounts(gunShotCounts);
    for (int i = 0; i < NUM_GUNS; i++) gunWasOn[i] = false;
    Serial.printf("[STATS] Gun shots lifetime: P1=%u P2=%u P3=%u P4=%u P5=%u P6=%u\n",
                  gunShotCounts[0], gunShotCounts[1], gunShotCounts[2],
                  gunShotCounts[3], gunShotCounts[4], gunShotCounts[5]);
    Serial.printf("[STATS] MTH: %u s (%.1f h)\n", mthTotalSec, mthTotalSec / 3600.0f);
}

void StatisticsManager::updatePainting(float distanceDelta, const bool gunStates[NUM_GUNS]) {
    if (distanceDelta <= 0) return;

    sessionDistance += distanceDelta;
    lifetime.totalDistance += distanceDelta;

    for (int i = 0; i < NUM_GUNS; i++) {
        if (gunStates[i]) {
            float area = distanceDelta * GUN_WIDTHS_M[i];
            gunDistances[i] += distanceDelta;
            sessionArea += area;
            lifetime.totalArea += area;
            // Zlicz tranzycje OFF->ON (= nowy strzal)
            if (!gunWasOn[i]) {
                gunShotCounts[i]++;
            }
        }
        gunWasOn[i] = gunStates[i];
    }
}

void StatisticsManager::resetSession() {
    sessionDistance = 0;
    sessionArea = 0;
    sessionAccumMs = 0;
    sessionStartMs = 0;
    sessionPausedMs = 0;
    sessionTimerRunning = false;
    for (int i = 0; i < NUM_GUNS; i++) {
        gunDistances[i] = 0;
    }
}

void StatisticsManager::resetAll() {
    resetSession();
    lifetime = LifetimeStats();
    for (int i = 0; i < NUM_GUNS; i++) {
        gunShotCounts[i] = 0;
        gunWasOn[i] = false;
    }
}

unsigned long StatisticsManager::getSessionTimeSec() const {
    unsigned long total = sessionAccumMs;
    if (sessionTimerRunning) {
        total += millis() - sessionStartMs;
    }
    return total / 1000;
}

void StatisticsManager::startSessionTimer() {
    sessionStartMs = millis();
    sessionTimerRunning = true;
}

void StatisticsManager::pauseSessionTimer() {
    if (sessionTimerRunning) {
        sessionAccumMs += millis() - sessionStartMs;
        sessionTimerRunning = false;
    }
}

void StatisticsManager::resumeSessionTimer() {
    sessionStartMs = millis();
    sessionTimerRunning = true;
}

void StatisticsManager::saveLifetime() {
    if (sessionTimerRunning) {
        lifetime.totalPaintTimeSec += getSessionTimeSec();
    }
    storage.saveLifetimeStats(lifetime);
    storage.saveGunShotCounts(gunShotCounts);
}

void StatisticsManager::loadLifetime() {
    lifetime = storage.loadLifetimeStats();
}

// ============ Motogodziny (MTH) ============

void StatisticsManager::startMTH() {
    if (!mthRunning) {
        mthStartMs = millis();
        mthRunning = true;
    }
}

void StatisticsManager::stopMTH() {
    if (mthRunning) {
        mthTotalSec += (millis() - mthStartMs) / 1000;
        mthRunning = false;
    }
}

void StatisticsManager::saveMTH() {
    uint32_t sec = getMTHSeconds();
    storage.saveMTH(sec);
}

void StatisticsManager::loadMTH() {
    mthTotalSec = storage.loadMTH();
    mthRunning = false;
}

uint32_t StatisticsManager::getMTHSeconds() const {
    uint32_t total = mthTotalSec;
    if (mthRunning) {
        total += (millis() - mthStartMs) / 1000;
    }
    return total;
}
