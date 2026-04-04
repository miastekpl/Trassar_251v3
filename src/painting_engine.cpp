#include "sys_log.h"
#include <esp_task_wdt.h>
// ============================================================
// TrassarV3 - Silnik malowania
// Tryby: AUTO / SEMI_AUTO / MANUAL
// ============================================================

#include "painting_engine.h"
#include "gun_logic.h"
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
#include "menu.h"
#include <math.h>

PaintingEngine paintEngine;

void PaintingEngine::begin() {
    lastEncoderDist = 0;
    patternStartDist = 0;
    gapStartActive = false;
    lastGunUpdateMs = millis();
    overspeedActive = false;
    overspeedGunsOff = false;
    lowSpeedActive = false;
    lastLowSpeedBuzMs = 0;
    lastOverspeedBuzMs = 0;
    lastAutoResumeMs = 0;
    semiLineDist = 0;
    semiLineComplete = false;
    semiSegmentNum = 0;
    autoPaused = false;
    autoPauseTracking = false;
    lowSpeedStartMs = 0;
    // Wczytaj ustawienie auto-resume z NVS
    autoResumeEnabled = storage.loadAutoResume();
}

bool PaintingEngine::shouldGunFire(GunID gun, float distFromPatternStart) const {
    GunPatternCfg cfg = patternMgr.getGunConfig(gun);
    return shouldGunFirePure(cfg, distFromPatternStart);
}

// Fix #30: Bezpieczne zatrzymanie z Core 0 — ustawia flage zamiast
// wykonywac ciężkie I/O (NVS/SD) na Core 0 co blokowalo WDT.
void PaintingEngine::requestStop() {
    stopRequested = true;
    goHomeAfterStop = true;
    // Natychmiastowa zmiana stanu + wylaczenie pistoletow (szybkie, bezpieczne z Core 0)
    STATE_LOCK();
    bool canStop = (g_state.machineState == STATE_PAINTING ||
                    g_state.machineState == STATE_PAUSED);
    if (canStop) g_state.machineState = STATE_STOPPED;
    STATE_UNLOCK();
    if (canStop) {
        guns.allOff();
    }
}

void PaintingEngine::update() {
    // Fix #30: Obsluz deferred stop z Core 0 (ciężkie I/O na Core 1).
    // requestStop() juz zmienil stan na STOPPED i wylaczyl pistolety.
    // Tutaj wykonujemy finalizacje (NVS save, GPX/CSV zapis) na Core 1
    // zamiast blokowac Core 0 na 800ms-2.5s.
    if (stopRequested) {
        stopRequested = false;
        bool wantsHome = goHomeAfterStop;
        goHomeAfterStop = false;
        stop(true);  // deferred=true: pomija state check, wykonuje finalizacje
        if (wantsHome) {
            menu.goToScreen(SCREEN_HOME);
        }
        return;
    }

    // Atomowy snapshot stanu (g_state modyfikowany z Core 0 przez web server)
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    MachineMode  snapMode  = g_state.machineMode;
    STATE_UNLOCK();

    if (snapState != STATE_PAINTING) {
        overspeedActive = false;
        overspeedGunsOff = false;
        lowSpeedActive = false;

        // --- Auto-resume z histereza: wznowienie po auto-pauzie ---
        // Wymaga utrzymania predkosci >= prog przez AUTO_RESUME_DEBOUNCE_MS
        // aby szum enkodera lub chwilowy skok nie wyzwalal resume.
        if (snapState == STATE_PAUSED && autoPaused && autoResumeEnabled) {
            float speedNow = encoderDist.getSpeedKmh();
            unsigned long now = millis();
            if (speedNow >= minSpeedKmh) {
                if (!resumeSpeedTracking) {
                    resumeSpeedTracking = true;
                    resumeSpeedStartMs = now;
                } else if (now - resumeSpeedStartMs >= AUTO_RESUME_DEBOUNCE_MS) {
                    // Predkosc utrzymywana >= prog przez wymagany czas
                    autoPaused = false;
                    resumeSpeedTracking = false;
                    lastAutoResumeMs = now;  // Cooldown: zapobiega natychmiastowej re-pauzie
                    resume();
                    eventLog.logf("ENGINE", "AUTO-RESUME: predkosc %.1f >= %.1f km/h (debounce %ums)",
                                  speedNow, minSpeedKmh, AUTO_RESUME_DEBOUNCE_MS);
                }
            } else {
                resumeSpeedTracking = false;  // Reset jesli predkosc spadla
            }
        }
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
    // Fix #12: pistolety OFF takze przy przekroczeniu maxSpeedKmh (OVERSPEED_GUN_DISABLE)
    overspeedGunsOff = OVERSPEED_GUN_DISABLE && (speedKmh > maxSpeedKmh);
    bool speedOK = (speedKmh >= minSpeedKmh) && !overspeedGunsOff;

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
            buzzer.beep(BUZ_SEMI_LINE_FREQ, BUZ_SEMI_LINE_DURATION_MS);
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
    } else if (lowSpeedActive && (now - lastLowSpeedBuzMs >= LOW_SPEED_BUZZ_REPEAT_MS)) {
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
        if (overspeedGunsOff) {
            guns.allOff();  // Natychmiastowe wylaczenie przy wejsciu w overspeed
        }
        eventLog.logf("ENGINE", "PRZEKROCZENIE predkosci: %.1f > %.1f km/h | pistolety=%s",
                      speedKmh, maxSpeedKmh, overspeedGunsOff ? "OFF" : "ON");
    } else if (overspeedActive && (now - lastOverspeedBuzMs >= OVERSPEED_BUZZ_REPEAT_MS)) {
        // Powtarzaj co 2s
        buzzer.play(BUZ_OVERSPEED);
        lastOverspeedBuzMs = now;
    }

    // --- Auto-pauza przy zatrzymaniu (tryb AUTO/SEMI/DEMO) ---
    // Fix #9: cooldown po auto-resume — zapobiega oscylacji pauza/resume
    bool cooldownActive = (lastAutoResumeMs > 0) &&
                          (now - lastAutoResumeMs < AUTO_RESUME_COOLDOWN_MS);
    if (snapMode != MODE_MANUAL && !cooldownActive) {
        if (speedKmh < AUTO_PAUSE_SPEED_KMH) {
            if (!autoPauseTracking) {
                autoPauseTracking = true;
                lowSpeedStartMs = now;
            } else if (!autoPaused) {
                // Szybsza auto-pauza gdy enkoder nie generuje impulsow (awaria/rozlaczenie).
                // zeroSpeedCount >= prog = calkowite zatrzymanie potwierdzone wieloma cyklami.
                unsigned long pauseDelay = AUTO_PAUSE_DELAY_MS;
                if (encoderDist.getZeroSpeedCount() >= ENCODER_ZERO_SPEED_THRESHOLD) {
                    pauseDelay = AUTO_PAUSE_ZERO_PULSE_MS;
                }
                if (now - lowSpeedStartMs >= pauseDelay) {
                    autoPaused = true;
                    autoPauseTracking = false;
                    pause();
                    buzzer.play(BUZ_AUTO_PAUSE);
                    eventLog.logf("ENGINE", "AUTO-PAUZA: predkosc %.1f < %.1f km/h (delay=%lums)",
                                  speedKmh, (float)AUTO_PAUSE_SPEED_KMH, pauseDelay);
                }
            }
        } else {
            autoPauseTracking = false;
        }
    }
}

void PaintingEngine::start(float offsetDist) {
    // Atomowe check-and-set: sprawdzenie + zmiana stanu w jednym locku
    // Fix #24: Ustaw lastGunUpdateMs PRZED zmiana stanu na PAINTING.
    // Poprzednio Core 0 mogl odczytac STATE_PAINTING zanim lastGunUpdateMs
    // zostal zaktualizowany — powodowalo to falszywy keepalive alarm
    // z wartoscia ~4294967291 ms (unsigned underflow).
    STATE_LOCK();
    bool canStart = (g_state.machineState == STATE_IDLE ||
                     g_state.machineState == STATE_STOPPED);
    MachineMode snapMode = g_state.machineMode;
    if (canStart) {
        lastGunUpdateMs = millis();
        g_state.machineState = STATE_PAINTING;
        g_state.currentScreen = SCREEN_PAINTING;
        g_state.displayNeedsUpdate = true;
        g_state.forceFullRedraw = true;
    }
    STATE_UNLOCK();

    if (canStart) {
        encoderDist.resetDistance();
        stats.resetSession();
        lastEncoderDist = 0;
        patternStartDist = offsetDist;
        gapStartActive = (offsetDist != 0.0f);
        patternChangePending = false;
        semiLineDist = 0;
        semiLineComplete = false;
        semiSegmentNum = 1;
        autoPaused = false;
        autoPauseTracking = false;
        resumeSpeedTracking = false;
        lastAutoResumeMs = 0;
        overspeedGunsOff = false;

        stats.startSessionTimer();
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
    // Atomowy odczyt stanu i trybu
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
        DBG_PRINTLN("[ENGINE] Semi-auto: start od przerwy (czeka na START)");
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
    DBG_PRINTF("[ENGINE] Start OD PRZERWY - wzorzec %s, offset %.1fm\n",
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
        // Nie graj BUZ_PAINT_STOP przy auto-pauzie (jest BUZ_AUTO_PAUSE)
        if (!autoPaused) {
            buzzer.play(BUZ_PAINT_STOP);
        }
        STATE_LOCK();
        g_state.displayNeedsUpdate = true;
        STATE_UNLOCK();
        eventLog.log("ENGINE", "PAUZA");
    }
}

void PaintingEngine::resume() {
    // Fix #24: lastGunUpdateMs PRZED STATE_PAINTING (jak w start())
    STATE_LOCK();
    bool canResume = (g_state.machineState == STATE_PAUSED);
    if (canResume) {
        lastGunUpdateMs = millis();
        g_state.machineState = STATE_PAINTING;
    }
    STATE_UNLOCK();

    if (canResume) {
        autoPaused = false;
        autoPauseTracking = false;
        stats.resumeSessionTimer();
        buzzer.play(BUZ_PAINT_START);
        STATE_LOCK();
        g_state.displayNeedsUpdate = true;
        STATE_UNLOCK();
        eventLog.log("ENGINE", "WZNOWIENIE");
    }
}

void PaintingEngine::stop(bool deferred) {
    bool canStop;
    if (deferred) {
        // Fix #30: requestStop() juz zmienil stan i wylaczyl pistolety.
        // Pomijamy sprawdzenie stanu — tylko finalizacja (NVS/SD zapis).
        canStop = true;
    } else {
        STATE_LOCK();
        canStop = (g_state.machineState == STATE_PAINTING ||
                   g_state.machineState == STATE_PAUSED);
        if (canStop) g_state.machineState = STATE_STOPPED;
        STATE_UNLOCK();
    }

    if (canStop) {
        autoPaused = false;
        autoPauseTracking = false;
        // Zastosuj oczekujacy wzorzec (zeby po STOP byl aktywny)
        if (patternChangePending) {
            patternMgr.setPattern(pendingPattern);
            storage.saveLastPattern(pendingPattern);
            patternChangePending = false;
            DBG_PRINTF("[ENGINE] Stop: zastosowano oczekujacy wzorzec %s\n",
                          patternMgr.getCurrent().code);
        }
        guns.allOff();
        stats.finalizeCurrentPattern();
        stats.pauseSessionTimer();

        // Fix #26: WDT reset miedzy kazdą ciężką operacją SD/NVS w stop().
        // Poprzednio stop() wykonywal 5+ dlugich operacji (NVS write, GPX zapis
        // do 4320 punktow, raport CSV, raport HTML) bez zadnego esp_task_wdt_reset().
        // Sumaryczny czas latwo przekraczal 5s WDT timeout → restart ESP.
        esp_task_wdt_reset();
        stats.saveLifetime();
        esp_task_wdt_reset();
        storage.savePaintLevel(paintConsumption.getCurrentLevel());
        esp_task_wdt_reset();
        buzzer.play(BUZ_PAINT_STOP);

        // Zapis trasy GPS jako plik .gpx na karte SD
        // (wewnatrz writeGpxFile/writeGeoJsonFile tez sa WDT resety co 50 pkt)
        gpsTrack.stopRecording();
        esp_task_wdt_reset();

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
        esp_task_wdt_reset();

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
        esp_task_wdt_reset();

        // Nie wymuszaj zmiany ekranu — to handler menu (lub caller) decyduje
        // dokad przejsc po zatrzymaniu (SUMMARY, HOME, SERVICE_MENU itd.)
        STATE_LOCK();
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
    // Atomowy snapshot stanu, trybu i biezacego wzorca (g_state modyfikowany z Core 0)
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    PatternID snapPattern = g_state.currentPattern;
    STATE_UNLOCK();

    if (snapState == STATE_PAINTING) {
        if (pat == snapPattern) {
            // Kliknieto biezacy wzorzec → anuluj pending
            if (patternChangePending) {
                patternChangePending = false;
                DBG_PRINTLN("[ENGINE] Anulowano kolejkowana zmiane wzorca");
                STATE_LOCK();
                g_state.displayNeedsUpdate = true;
                STATE_UNLOCK();
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
            DBG_PRINTF("[ENGINE] Wzorzec %s kolejkowany (czeka na koniec cyklu)\n",
                          patternMgr.getPattern(pat).code);
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
        } else {
            // INSTANT: natychmiastowa zmiana (utnij biezacy wzorzec)
            stats.notifyPatternChange(pat);
            patternMgr.setPattern(pat);
            patternStartDist = encoderDist.getDistanceMeters();
            storage.saveLastPattern(pat);
            patternChangePending = false;
            // Reset semi-auto state
            semiLineDist = 0;
            semiLineComplete = false;
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            buzzer.beep(BUZ_CONFIRM_FREQ, BUZ_CONFIRM_DURATION_MS);
            DBG_PRINTF("[ENGINE] Natychmiastowa zmiana wzorca -> %s\n",
                          patternMgr.getCurrent().code);
        }
        return;
    }

    // Nie maluje → natychmiastowa zmiana
    patternMgr.setPattern(pat);
    patternStartDist = encoderDist.getDistanceMeters();
    storage.saveLastPattern(pat);
    patternChangePending = false;
    STATE_LOCK();
    g_state.displayNeedsUpdate = true;
    STATE_UNLOCK();
    DBG_PRINTF("[ENGINE] Zmiana wzorca -> %s\n",
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
    DBG_PRINTF("[ENGINE] Inteligentne przelaczenie -> %s\n",
                  patternMgr.getPattern(pendingPattern).code);
    stats.notifyPatternChange(pendingPattern);
    patternMgr.setPattern(pendingPattern);
    patternStartDist = encoderDist.getDistanceMeters();
    storage.saveLastPattern(pendingPattern);
    patternChangePending = false;
    STATE_LOCK();
    g_state.displayNeedsUpdate = true;
    STATE_UNLOCK();
    buzzer.beep(BUZ_CONFIRM_FREQ, BUZ_CONFIRM_DURATION_MS);  // Krotki sygnal potwierdzenia
}

void PaintingEngine::toggleReverse() {
    patternMgr.toggleReverse();
    STATE_LOCK();
    g_state.displayNeedsUpdate = true;
    STATE_UNLOCK();
    DBG_PRINTF("[ENGINE] Odwrocenie: %s\n",
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
    semiSegmentNum++;
    buzzer.beep(BUZ_CONFIRM_FREQ, BUZ_CONFIRM_DURATION_MS);  // Krotki sygnal potwierdzenia
    DBG_PRINTF("[ENGINE] Semi-auto: rozpoczynam segment %d\n", semiSegmentNum);
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
