#include "sys_log.h"
// ============================================================
// TrassarV3 - Statystyki malowania
// ============================================================

#include "statistics.h"
#include "storage.h"
#include "paint_consumption.h"

StatisticsManager stats;

void StatisticsManager::begin() {
    resetSession();
    loadLifetime();
    loadMTH();
    storage.loadGunShotCounts(gunShotCounts);
    for (int i = 0; i < NUM_GUNS; i++) gunWasOn[i] = false;
    DBG_PRINTF("[STATS] Gun shots lifetime: P1=%u P2=%u P3=%u P4=%u P5=%u P6=%u\n",
                  gunShotCounts[0], gunShotCounts[1], gunShotCounts[2],
                  gunShotCounts[3], gunShotCounts[4], gunShotCounts[5]);
    DBG_PRINTF("[STATS] MTH: %u s (%.1f h)\n", mthTotalSec, mthTotalSec / 3600.0f);
}

void StatisticsManager::updatePainting(float distanceDelta, const bool gunStates[NUM_GUNS]) {
    if (distanceDelta <= 0) return;

    unsigned long now = millis();

    taskENTER_CRITICAL(&statsMux);

    sessionDistance += distanceDelta;
    lifetime.totalDistance += distanceDelta;

    float deltaArea = 0;
    for (int i = 0; i < NUM_GUNS; i++) {
        if (gunStates[i]) {
            float area = distanceDelta * GUN_WIDTHS_M[i];
            gunDistances[i] += distanceDelta;
            sessionArea += area;
            lifetime.totalArea += area;
            deltaArea += area;
            // Zlicz tranzycje OFF->ON (= nowy strzal) z debounce
            if (!gunWasOn[i] && (now - gunLastOnMs[i] >= GUN_SHOT_DEBOUNCE_MS)) {
                gunShotCounts[i]++;
                gunLastOnMs[i] = now;
            }
        }
        gunWasOn[i] = gunStates[i];
    }

    // Sledzenie wzorcow w sesji
    patCurrentDist += distanceDelta;
    patCurrentArea += deltaArea;

    taskEXIT_CRITICAL(&statsMux);

    // Aktualizuj poziom farby w zbiorniku
    if (deltaArea > 0) {
        paintConsumption.subtractUsage(deltaArea);
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
    patEntryCount = 0;
    patCurrentDist = 0;
    patCurrentArea = 0;
    currentPatternTracked = g_state.currentPattern;
}

void StatisticsManager::notifyPatternChange(PatternID newPattern) {
    if (newPattern == currentPatternTracked) return;
    finalizeCurrentPattern();
    currentPatternTracked = newPattern;
    patCurrentDist = 0;
    patCurrentArea = 0;
}

void StatisticsManager::finalizeCurrentPattern() {
    if (patCurrentDist <= 0 && patCurrentArea <= 0) return;
    // Szukaj istniejacego wpisu dla tego wzorca
    for (int i = 0; i < patEntryCount; i++) {
        if (patEntries[i].pattern == currentPatternTracked) {
            patEntries[i].distance += patCurrentDist;
            patEntries[i].area += patCurrentArea;
            return;
        }
    }
    // Nowy wpis
    if (patEntryCount < MAX_PATTERN_ENTRIES) {
        patEntries[patEntryCount].pattern = currentPatternTracked;
        patEntries[patEntryCount].distance = patCurrentDist;
        patEntries[patEntryCount].area = patCurrentArea;
        patEntryCount++;
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
    LifetimeStats ltCopy;
    uint32_t shotsCopy[NUM_GUNS];

    taskENTER_CRITICAL(&statsMux);
    ltCopy = lifetime;
    if (sessionTimerRunning) {
        ltCopy.totalPaintTimeSec += getSessionTimeSec();
    }
    for (int i = 0; i < NUM_GUNS; i++) shotsCopy[i] = gunShotCounts[i];
    taskEXIT_CRITICAL(&statsMux);

    storage.saveLifetimeStats(ltCopy);
    storage.saveGunShotCounts(shotsCopy);
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
