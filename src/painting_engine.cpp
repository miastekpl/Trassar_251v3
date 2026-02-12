// ============================================================
// TrassarV3 - Silnik malowania
// ============================================================

#include "painting_engine.h"
#include "patterns.h"
#include "guns.h"
#include "encoder_distance.h"
#include "statistics.h"
#include "storage.h"
#include "report_logger.h"
#include <math.h>

PaintingEngine paintEngine;

void PaintingEngine::begin() {
    lastEncoderDist = 0;
    patternStartDist = 0;
    gapStartActive = false;
}

bool PaintingEngine::shouldGunFire(GunID gun, float distFromPatternStart) const {
    GunPatternCfg cfg = patternMgr.getGunConfig(gun);

    switch (cfg.mode) {
        case GUN_OFF:
            return false;
        case GUN_CONTINUOUS:
            return true;
        case GUN_DASHED: {
            float cycle = cfg.lineLen + cfg.gapLen;
            if (cycle <= 0) return false;
            float pos = fmodf(distFromPatternStart, cycle);
            return (pos < cfg.lineLen);
        }
    }
    return false;
}

void PaintingEngine::update() {
    if (g_state.machineState != STATE_PAINTING) return;

    float currentDist = encoderDist.getDistanceMeters();
    float deltaDist = currentDist - lastEncoderDist;
    lastEncoderDist = currentDist;

    float distFromPatternStart = currentDist - patternStartDist;
    if (distFromPatternStart < 0) distFromPatternStart = 0;

    // Bezpieczenstwo: pistolety tylko przy >= 3 km/h
    bool speedOK = (encoderDist.getSpeedKmh() >= MIN_PAINT_SPEED_KMH);

    // Aktualizuj stan każdego pistoletu
    bool gunStates[NUM_GUNS];
    for (int i = 0; i < NUM_GUNS; i++) {
        bool fire = speedOK && shouldGunFire((GunID)i, distFromPatternStart);
        guns.setGun((GunID)i, fire);
        gunStates[i] = fire;
    }

    // Aktualizuj statystyki
    if (deltaDist > 0) {
        stats.updatePainting(deltaDist, gunStates);
    }
}

void PaintingEngine::start() {
    if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
        encoderDist.resetDistance();
        stats.resetSession();
        lastEncoderDist = 0;
        patternStartDist = 0;
        gapStartActive = false;
        g_state.machineState = STATE_PAINTING;
        stats.startSessionTimer();
        g_state.currentScreen = SCREEN_PAINTING;
        g_state.displayNeedsUpdate = true;
        g_state.forceFullRedraw = true;
        Serial.printf("[ENGINE] Start malowania - wzorzec %s\n",
                      patternMgr.getCurrent().code);
    }
}

void PaintingEngine::startFromGap() {
    if (g_state.machineState != STATE_IDLE && g_state.machineState != STATE_STOPPED)
        return;

    // Znajdz pierwszy pistolet DASHED w biezacym wzorcu
    const PatternDef& pat = patternMgr.getCurrent();
    float lineLen = 0;
    for (int i = 0; i < NUM_GUNS; i++) {
        GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);
        if (cfg.mode == GUN_DASHED && cfg.lineLen > 0) {
            lineLen = cfg.lineLen;
            break;
        }
    }

    if (lineLen <= 0) {
        // Brak przerw we wzorcu - normalny start
        start();
        return;
    }

    encoderDist.resetDistance();
    stats.resetSession();
    lastEncoderDist = 0;
    // Przesuniecie o lineLen sprawia, ze cykl zaczyna od przerwy
    // shouldGunFire: pos = fmod(dist + lineLen, cycle) = lineLen → gap
    patternStartDist = -lineLen;
    gapStartActive = true;
    g_state.machineState = STATE_PAINTING;
    stats.startSessionTimer();
    g_state.currentScreen = SCREEN_PAINTING;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
    Serial.printf("[ENGINE] Start OD PRZERWY - wzorzec %s, offset %.1fm\n",
                  pat.code, lineLen);
}

void PaintingEngine::pause() {
    if (g_state.machineState == STATE_PAINTING) {
        g_state.machineState = STATE_PAUSED;
        guns.allOff();
        stats.pauseSessionTimer();
        g_state.displayNeedsUpdate = true;
        Serial.println("[ENGINE] Pauza");
    }
}

void PaintingEngine::resume() {
    if (g_state.machineState == STATE_PAUSED) {
        g_state.machineState = STATE_PAINTING;
        stats.resumeSessionTimer();
        g_state.displayNeedsUpdate = true;
        Serial.println("[ENGINE] Wznowienie");
    }
}

void PaintingEngine::stop() {
    if (g_state.machineState == STATE_PAINTING ||
        g_state.machineState == STATE_PAUSED) {
        g_state.machineState = STATE_STOPPED;
        guns.allOff();
        stats.pauseSessionTimer();
        stats.saveLifetime();

        // Zapis raportu na karte SD
        reportLogger.logSession(
            patternMgr.getCurrent().code,
            stats.getSessionDistance(),
            stats.getSessionArea()
        );

        g_state.currentScreen = SCREEN_HOME;
        g_state.displayNeedsUpdate = true;
        g_state.forceFullRedraw = true;
        Serial.printf("[ENGINE] Stop - dystans: %.1fm  powierzchnia: %.2fm2\n",
                      stats.getSessionDistance(), stats.getSessionArea());
    }
}

void PaintingEngine::setPattern(PatternID pat) {
    patternMgr.setPattern(pat);
    // Reset odległości wzorca przy zmianie
    patternStartDist = encoderDist.getDistanceMeters();
    storage.saveLastPattern(pat);
    g_state.displayNeedsUpdate = true;
    Serial.printf("[ENGINE] Zmiana wzorca -> %s\n",
                  patternMgr.getCurrent().code);
}

void PaintingEngine::toggleReverse() {
    patternMgr.toggleReverse();
    g_state.displayNeedsUpdate = true;
    Serial.printf("[ENGINE] Odwrocenie: %s\n",
                  g_state.patternReversed ? "TAK" : "NIE");
}
