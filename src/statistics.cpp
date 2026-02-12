// ============================================================
// TrassarV3 - Statystyki malowania
// ============================================================

#include "statistics.h"
#include "storage.h"

StatisticsManager stats;

void StatisticsManager::begin() {
    resetSession();
    loadLifetime();
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
        }
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
}

void StatisticsManager::loadLifetime() {
    lifetime = storage.loadLifetimeStats();
}
