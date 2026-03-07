// ============================================================
// TrassarV3 - Silnik malowania
// Tryby: AUTO / SEMI_AUTO / MANUAL
// ============================================================

#include "painting_engine.h"
#include "patterns.h"
#include "guns.h"
#include "encoder_distance.h"
#include "statistics.h"
#include "storage.h"
#include "report_logger.h"
#include "gps_handler.h"
#include "gps_track.h"
#include "buzzer.h"
#include "button_handler.h"
#include "event_log.h"
#include "session_report.h"
#include "paint_consumption.h"
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
    semiLineDist = 0;
    semiLineComplete = false;
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
    // Atomowy snapshot stanu (g_state modyfikowany z Core 0 przez web server)
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    MachineMode  snapMode  = g_state.machineMode;
    STATE_UNLOCK();

    if (snapState != STATE_PAINTING) {
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

    // --- Inteligentne przelaczanie: sprawdz granice cyklu (tylko AUTO) ---
    if (patternChangePending && snapMode == MODE_AUTO) {
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

    // Bezpieczenstwo: pistolety tylko przy >= prog minimalny
    bool speedOK = (speedKmh >= minSpeedKmh);

    // ============================================================
    // Sterowanie pistoletami zaleznie od trybu
    // ============================================================
    bool gunStates[NUM_GUNS];

    if (snapMode == MODE_MANUAL) {
        // --- TRYB RECZNY ---
        // Pistolety strzelaja gdy operator trzyma START i predkosc OK
        bool held = buttons.isStartHeld();
        for (int i = 0; i < NUM_GUNS; i++) {
            GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);
            bool fire = speedOK && held && (cfg.mode != GUN_OFF);
            guns.setGun((GunID)i, fire);
            gunStates[i] = fire;
        }

    } else if (snapMode == MODE_SEMI_AUTO) {
        // --- TRYB POLAUTOMATYCZNY ---
        // Kazdy pistolet DASHED maluje do swojego lineLen niezaleznie,
        // semiLineComplete dopiero gdy wszystkie pistolety DASHED skonczyly
        if (deltaDist > 0) semiLineDist += deltaDist;

        bool anyDashed = false;
        bool allDashedDone = true;

        for (int i = 0; i < NUM_GUNS; i++) {
            GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);
            bool fire = false;

            if (cfg.mode == GUN_CONTINUOUS) {
                // Ciagly - zawsze aktywny gdy predkosc OK
                fire = speedOK;
            } else if (cfg.mode == GUN_DASHED) {
                anyDashed = true;
                if (!semiLineComplete) {
                    if (semiLineDist < cfg.lineLen) {
                        // Ten pistolet jeszcze maluje swoja kreske
                        fire = speedOK;
                        allDashedDone = false;
                    }
                    // else: ten pistolet skonczyl kreske, fire = false
                }
            }

            guns.setGun((GunID)i, fire);
            gunStates[i] = fire;
        }

        // Ustaw semiLineComplete gdy wszystkie DASHED pistolety skonczyly
        if (anyDashed && !semiLineComplete && allDashedDone) {
            semiLineComplete = true;
            buzzer.beep(1000, 50);  // Krotki sygnal: linia gotowa
        }

    } else if (snapMode == MODE_DEMO) {
        // --- TRYB DEMO (nauka operatora) ---
        // Logika identyczna jak AUTO, ale pistolety NIE strzelaja fizycznie
        // gunStates ustawiane dla wizualizacji na TFT i panelu WWW
        for (int i = 0; i < NUM_GUNS; i++) {
            bool wouldFire = speedOK && shouldGunFire((GunID)i, distFromPatternStart);
            guns.setGun((GunID)i, false);  // Fizycznie zawsze OFF
            gunStates[i] = wouldFire;       // Wizualnie: co by strzelalo
        }

    } else {
        // --- TRYB AUTOMATYCZNY (domyslny) ---
        for (int i = 0; i < NUM_GUNS; i++) {
            bool fire = speedOK && shouldGunFire((GunID)i, distFromPatternStart);
            guns.setGun((GunID)i, fire);
            gunStates[i] = fire;
        }
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
        eventLog.logf("ENGINE", "PRZEKROCZENIE predkosci: %.1f > %.1f km/h",
                      speedKmh, maxSpeedKmh);
    } else if (overspeedActive && (now - lastOverspeedBuzMs >= 2000)) {
        // Powtarzaj co 2s
        buzzer.play(BUZ_OVERSPEED);
        lastOverspeedBuzMs = now;
    }
}

void PaintingEngine::start(float offsetDist) {
    STATE_LOCK();
    MachineState curState = g_state.machineState;
    STATE_UNLOCK();

    if (curState == STATE_IDLE || curState == STATE_STOPPED) {
        encoderDist.resetDistance();
        stats.resetSession();
        lastEncoderDist = 0;
        patternStartDist = offsetDist;
        gapStartActive = (offsetDist != 0.0f);
        patternChangePending = false;
        semiLineDist = 0;
        semiLineComplete = false;

        STATE_LOCK();
        g_state.machineState = STATE_PAINTING;
        g_state.currentScreen = SCREEN_PAINTING;
        g_state.displayNeedsUpdate = true;
        g_state.forceFullRedraw = true;
        MachineMode snapMode = g_state.machineMode;
        STATE_UNLOCK();

        stats.startSessionTimer();
        lastGunUpdateMs = millis();
        buzzer.play(BUZ_PAINT_START);
        gpsTrack.startRecording();

        const char* modeStr = "AUTO";
        if (snapMode == MODE_SEMI_AUTO) modeStr = "SEMI";
        else if (snapMode == MODE_MANUAL) modeStr = "MANUAL";
        eventLog.logf("ENGINE", "START malowania | wzorzec=%s tryb=%s",
                      patternMgr.getCurrent().code, modeStr);
    }
}

void PaintingEngine::startFromGap() {
    STATE_LOCK();
    MachineState curState = g_state.machineState;
    MachineMode  curMode  = g_state.machineMode;
    STATE_UNLOCK();

    if (curState != STATE_IDLE && curState != STATE_STOPPED)
        return;

    // W trybie SEMI_AUTO: start od przerwy = semiLineComplete od razu
    if (curMode == MODE_SEMI_AUTO) {
        start();
        semiLineComplete = true;  // Zaczyna od przerwy - czeka na START
        gapStartActive = true;
        Serial.println("[ENGINE] Semi-auto: start od przerwy (czeka na START)");
        return;
    }

    // W trybie MANUAL: brak przerw - normalny start
    if (curMode == MODE_MANUAL) {
        start();
        return;
    }

    // Tryb AUTO: oblicz offset od lineLen
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
        start();
        return;
    }

    // Przesuniecie o lineLen sprawia, ze cykl zaczyna od przerwy
    start(-lineLen);
    Serial.printf("[ENGINE] Start OD PRZERWY - wzorzec %s, offset %.1fm\n",
                  pat.code, lineLen);
}

void PaintingEngine::pause() {
    STATE_LOCK();
    bool canPause = (g_state.machineState == STATE_PAINTING);
    if (canPause) g_state.machineState = STATE_PAUSED;
    STATE_UNLOCK();

    if (canPause) {
        guns.allOff();
        stats.pauseSessionTimer();
        buzzer.play(BUZ_PAINT_STOP);
        STATE_LOCK();
        g_state.displayNeedsUpdate = true;
        STATE_UNLOCK();
        eventLog.log("ENGINE", "PAUZA");
    }
}

void PaintingEngine::resume() {
    STATE_LOCK();
    bool canResume = (g_state.machineState == STATE_PAUSED);
    if (canResume) g_state.machineState = STATE_PAINTING;
    STATE_UNLOCK();

    if (canResume) {
        lastGunUpdateMs = millis();
        stats.resumeSessionTimer();
        buzzer.play(BUZ_PAINT_START);
        STATE_LOCK();
        g_state.displayNeedsUpdate = true;
        STATE_UNLOCK();
        eventLog.log("ENGINE", "WZNOWIENIE");
    }
}

void PaintingEngine::stop() {
    STATE_LOCK();
    bool canStop = (g_state.machineState == STATE_PAINTING ||
                    g_state.machineState == STATE_PAUSED);
    if (canStop) g_state.machineState = STATE_STOPPED;
    STATE_UNLOCK();

    if (canStop) {
        // Zastosuj oczekujacy wzorzec (zeby po STOP byl aktywny)
        if (patternChangePending) {
            patternMgr.setPattern(pendingPattern);
            storage.saveLastPattern(pendingPattern);
            patternChangePending = false;
            Serial.printf("[ENGINE] Stop: zastosowano oczekujacy wzorzec %s\n",
                          patternMgr.getCurrent().code);
        }
        guns.allOff();
        stats.pauseSessionTimer();
        stats.saveLifetime();
        buzzer.play(BUZ_PAINT_STOP);

        // Zapis trasy GPS jako plik .gpx na karte SD
        gpsTrack.stopRecording();

        // Zapis raportu CSV na karte SD (z koordynatami GPS jesli dostepne)
        bool gFix = gpsHandler.hasFix();
        double gLat = gFix ? gpsHandler.getLat() : 0;
        double gLon = gFix ? gpsHandler.getLng() : 0;
        reportLogger.logSession(
            patternMgr.getCurrent().code,
            stats.getSessionDistance(),
            stats.getSessionArea(),
            gLat, gLon
        );

        // Automatyczny raport HTML
        float sessionDist = stats.getSessionDistance();
        float sessionArea = stats.getSessionArea();
        unsigned long sessionTime = stats.getSessionTimeSec();
        float avgSpeed = 0;
        if (sessionTime > 0) {
            avgSpeed = (sessionDist / 1000.0f) / ((float)sessionTime / 3600.0f);
        }
        sessionReport.generateReport(
            patternMgr.getCurrent().code,
            sessionDist, sessionArea, sessionTime, avgSpeed,
            gFix, gLat, gLon,
            paintConsumption.getUsedLiters(sessionArea),
            paintConsumption.getRemainingLiters(stats.getLifetimeArea() + sessionArea)
        );

        STATE_LOCK();
        g_state.currentScreen = SCREEN_HOME;
        g_state.displayNeedsUpdate = true;
        g_state.forceFullRedraw = true;
        STATE_UNLOCK();

        eventLog.logf("ENGINE", "STOP | dist=%.1fm area=%.2fm2 czas=%us",
                      stats.getSessionDistance(), stats.getSessionArea(),
                      stats.getSessionTimeSec());
    }
}

void PaintingEngine::setPattern(PatternID pat) {
    // --- Przelaczanie wzorcow (smart lub instant) ---
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    STATE_UNLOCK();

    if (snapState == STATE_PAINTING) {
        if (pat == g_state.currentPattern) {
            // Kliknieto biezacy wzorzec → anuluj pending
            if (patternChangePending) {
                patternChangePending = false;
                Serial.println("[ENGINE] Anulowano kolejkowana zmiane wzorca");
                g_state.displayNeedsUpdate = true;
            }
            return;
        }

        if (smartSwitch) {
            // SMART: kolejkuj zmiane do konca cyklu (linia+przerwa)
            pendingPattern = pat;
            patternChangePending = true;
            float cycle = getPrimaryCycle();
            float dist = encoderDist.getDistanceMeters() - patternStartDist;
            if (dist < 0) dist = 0;
            pendingCycleCount = (cycle > 0 && dist > 0)
                                ? (int)(dist / cycle) : -1;
            Serial.printf("[ENGINE] Wzorzec %s kolejkowany (czeka na koniec cyklu)\n",
                          patternMgr.getPattern(pat).code);
            g_state.displayNeedsUpdate = true;
        } else {
            // INSTANT: natychmiastowa zmiana (utnij biezacy wzorzec)
            patternMgr.setPattern(pat);
            patternStartDist = encoderDist.getDistanceMeters();
            storage.saveLastPattern(pat);
            patternChangePending = false;
            // Reset semi-auto state
            semiLineDist = 0;
            semiLineComplete = false;
            g_state.displayNeedsUpdate = true;
            buzzer.beep(1500, 80);
            Serial.printf("[ENGINE] Natychmiastowa zmiana wzorca -> %s\n",
                          patternMgr.getCurrent().code);
        }
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
                  patternMgr.getPattern(pendingPattern).code);
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
// Semi-auto: wyzwolenie kolejnej linii przez operatora
// Wywolywane z menu po wcisnieciu START na ekranie malowania
// ============================================================
void PaintingEngine::semiNextLine() {
    STATE_LOCK();
    MachineMode snapMode = g_state.machineMode;
    STATE_UNLOCK();
    if (snapMode != MODE_SEMI_AUTO) return;
    if (!semiLineComplete) return;
    semiLineDist = 0;
    semiLineComplete = false;
    buzzer.beep(1500, 80);  // Krotki sygnal potwierdzenia
    Serial.println("[ENGINE] Semi-auto: rozpoczynam kolejna linie");
}

float PaintingEngine::getPatternDistance() const {
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    STATE_UNLOCK();
    if (snapState != STATE_PAINTING && snapState != STATE_PAUSED) return 0;
    float totalDist = encoderDist.getDistanceMeters();
    return totalDist - patternStartDist;
}

// ============================================================
// Gun keepalive - awaryjne wylaczenie pistoletow
// Wywolywane w loop() niezaleznie od update()
// Jesli update() nie bylo wywolane >300ms a pistolety sa otwarte
// ============================================================
void PaintingEngine::checkGunKeepAlive() {
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    STATE_UNLOCK();
    if (snapState != STATE_PAINTING) return;

    unsigned long now = millis();
    if (now - lastGunUpdateMs > GUN_KEEPALIVE_TIMEOUT_MS) {
        // Awaryjne wylaczenie wszystkich pistoletow
        guns.allOff();
        eventLog.logf("ENGINE", "KEEPALIVE: awaryjne guns.allOff() (brak update %lu ms)",
                      now - lastGunUpdateMs);
    }
}
