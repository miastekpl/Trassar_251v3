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
#include "buzzer.h"
#include <math.h>

PaintingEngine paintEngine;

void PaintingEngine::begin() {
    lastEncoderDist = 0;
    patternStartDist = 0;
    gapStartActive = false;
    lastGunUpdateMs = millis();
    overspeedActive = false;
    lowSpeedActive = false;
    lastLowSpeedBuzMs = 0;
    lastOverspeedBuzMs = 0;
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
    if (g_state.machineState != STATE_PAINTING) {
        overspeedActive = false;
        lowSpeedActive = false;
        return;
    }

    unsigned long now = millis();
    lastGunUpdateMs = now;  // Znacznik keepalive

    float currentDist = encoderDist.getDistanceMeters();
    float deltaDist = currentDist - lastEncoderDist;
    lastEncoderDist = currentDist;

    float distFromPatternStart = currentDist - patternStartDist;
    if (distFromPatternStart < 0) distFromPatternStart = 0;

    // --- Inteligentne przelaczanie: sprawdz granice cyklu ---
    if (patternChangePending) {
        float cycle = getPrimaryCycle();
        if (cycle <= 0 || pendingCycleCount < 0) {
            // Wzorzec ciagly - przelacz natychmiast
            applyPendingPattern();
            // Przelicz distFromPatternStart po zmianie
            distFromPatternStart = currentDist - patternStartDist;
            if (distFromPatternStart < 0) distFromPatternStart = 0;
        } else {
            int curCycle = (int)(distFromPatternStart / cycle);
            if (curCycle > pendingCycleCount) {
                // Cykl sie skonczyl - przelacz
                applyPendingPattern();
                distFromPatternStart = currentDist - patternStartDist;
                if (distFromPatternStart < 0) distFromPatternStart = 0;
            }
        }
    }

    float speedKmh = encoderDist.getSpeedKmh();

    // Bezpieczenstwo: pistolety tylko przy >= 3 km/h
    bool speedOK = (speedKmh >= MIN_PAINT_SPEED_KMH);

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

    // --- Alarm niskiej predkosci (<3 km/h podczas malowania) ---
    bool wasLow = lowSpeedActive;
    lowSpeedActive = !speedOK;
    if (lowSpeedActive && !wasLow) {
        // Dopiero spadla ponizej progu - natychmiastowy sygnal
        buzzer.play(BUZ_LOW_SPEED);
        lastLowSpeedBuzMs = now;
    } else if (lowSpeedActive && (now - lastLowSpeedBuzMs >= 3000)) {
        // Powtarzaj co 3s dopoki predkosc jest za niska
        buzzer.play(BUZ_LOW_SPEED);
        lastLowSpeedBuzMs = now;
    }

    // --- Alarm przekroczenia predkosci ---
    bool wasOver = overspeedActive;
    overspeedActive = (speedKmh > maxSpeedKmh);
    if (overspeedActive && !wasOver) {
        buzzer.play(BUZ_OVERSPEED);
        lastOverspeedBuzMs = now;
        Serial.printf("[ENGINE] UWAGA: predkosc %.1f km/h > %.1f km/h!\n",
                      speedKmh, maxSpeedKmh);
    } else if (overspeedActive && (now - lastOverspeedBuzMs >= 2000)) {
        // Powtarzaj co 2s
        buzzer.play(BUZ_OVERSPEED);
        lastOverspeedBuzMs = now;
    }
}

void PaintingEngine::start() {
    if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
        encoderDist.resetDistance();
        stats.resetSession();
        lastEncoderDist = 0;
        patternStartDist = 0;
        gapStartActive = false;
        patternChangePending = false;
        g_state.machineState = STATE_PAINTING;
        stats.startSessionTimer();
        g_state.currentScreen = SCREEN_PAINTING;
        g_state.displayNeedsUpdate = true;
        g_state.forceFullRedraw = true;
        lastGunUpdateMs = millis();
        buzzer.play(BUZ_PAINT_START);
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
    patternChangePending = false;
    g_state.machineState = STATE_PAINTING;
    stats.startSessionTimer();
    g_state.currentScreen = SCREEN_PAINTING;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
    lastGunUpdateMs = millis();
    buzzer.play(BUZ_PAINT_START);
    Serial.printf("[ENGINE] Start OD PRZERWY - wzorzec %s, offset %.1fm\n",
                  pat.code, lineLen);
}

void PaintingEngine::pause() {
    if (g_state.machineState == STATE_PAINTING) {
        g_state.machineState = STATE_PAUSED;
        guns.allOff();
        stats.pauseSessionTimer();
        buzzer.play(BUZ_PAINT_STOP);
        g_state.displayNeedsUpdate = true;
        Serial.println("[ENGINE] Pauza");
    }
}

void PaintingEngine::resume() {
    if (g_state.machineState == STATE_PAUSED) {
        g_state.machineState = STATE_PAINTING;
        lastGunUpdateMs = millis();
        stats.resumeSessionTimer();
        buzzer.play(BUZ_PAINT_START);
        g_state.displayNeedsUpdate = true;
        Serial.println("[ENGINE] Wznowienie");
    }
}

void PaintingEngine::stop() {
    if (g_state.machineState == STATE_PAINTING ||
        g_state.machineState == STATE_PAUSED) {
        // Zastosuj oczekujacy wzorzec (zeby po STOP byl aktywny)
        if (patternChangePending) {
            patternMgr.setPattern(pendingPattern);
            storage.saveLastPattern(pendingPattern);
            patternChangePending = false;
            Serial.printf("[ENGINE] Stop: zastosowano oczekujacy wzorzec %s\n",
                          patternMgr.getCurrent().code);
        }
        g_state.machineState = STATE_STOPPED;
        guns.allOff();
        stats.pauseSessionTimer();
        stats.saveLifetime();
        buzzer.play(BUZ_PAINT_STOP);

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
    // --- Inteligentne przelaczanie wzorcow ---
    // Podczas malowania: kolejkuj zmiane do konca cyklu (linia+przerwa)
    // Nie malujac: zmiana natychmiastowa
    if (g_state.machineState == STATE_PAINTING) {
        if (pat == g_state.currentPattern) {
            // Kliknieto biezacy wzorzec → anuluj pending
            if (patternChangePending) {
                patternChangePending = false;
                Serial.println("[ENGINE] Anulowano kolejkowana zmiane wzorca");
                g_state.displayNeedsUpdate = true;
            }
            return;
        }
        pendingPattern = pat;
        patternChangePending = true;
        float cycle = getPrimaryCycle();
        float dist = encoderDist.getDistanceMeters() - patternStartDist;
        if (dist < 0) dist = 0;
        pendingCycleCount = (cycle > 0 && dist > 0)
                            ? (int)(dist / cycle) : -1;
        Serial.printf("[ENGINE] Wzorzec %s kolejkowany (czeka na koniec cyklu)\n",
                      PatternManager::patterns[pat].code);
        g_state.displayNeedsUpdate = true;
        return;
    }

    // Nie maluje → natychmiastowa zmiana
    patternMgr.setPattern(pat);
    patternStartDist = encoderDist.getDistanceMeters();
    storage.saveLastPattern(pat);
    patternChangePending = false;
    g_state.displayNeedsUpdate = true;
    Serial.printf("[ENGINE] Zmiana wzorca -> %s\n",
                  patternMgr.getCurrent().code);
}

// Cykl (linia+przerwa) glownego pistoletu DASHED biezacego wzorca
float PaintingEngine::getPrimaryCycle() const {
    for (int i = 0; i < NUM_GUNS; i++) {
        GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);
        if (cfg.mode == GUN_DASHED && cfg.lineLen > 0) {
            return cfg.lineLen + cfg.gapLen;
        }
    }
    return 0;  // Wzorzec ciagly (brak cyklu)
}

// Zastosuj oczekujaca zmiane wzorca
void PaintingEngine::applyPendingPattern() {
    Serial.printf("[ENGINE] Inteligentne przelaczenie -> %s\n",
                  PatternManager::patterns[pendingPattern].code);
    patternMgr.setPattern(pendingPattern);
    patternStartDist = encoderDist.getDistanceMeters();
    storage.saveLastPattern(pendingPattern);
    patternChangePending = false;
    g_state.displayNeedsUpdate = true;
    buzzer.beep(1500, 80);  // Krotki sygnal potwierdzenia
}

void PaintingEngine::toggleReverse() {
    patternMgr.toggleReverse();
    g_state.displayNeedsUpdate = true;
    Serial.printf("[ENGINE] Odwrocenie: %s\n",
                  g_state.patternReversed ? "TAK" : "NIE");
}

// ============================================================
// Gun keepalive - awaryjne wylaczenie pistoletow
// Wywolywane w loop() niezaleznie od update()
// Jesli update() nie bylo wywolane >300ms a pistolety sa otwarte
// ============================================================
void PaintingEngine::checkGunKeepAlive() {
    if (g_state.machineState != STATE_PAINTING) return;

    unsigned long now = millis();
    if (now - lastGunUpdateMs > GUN_KEEPALIVE_TIMEOUT_MS) {
        // Awaryjne wylaczenie wszystkich pistoletow
        guns.allOff();
        Serial.printf("[ENGINE] KEEPALIVE: awaryjne guns.allOff() (brak update od %lu ms)\n",
                      now - lastGunUpdateMs);
    }
}
