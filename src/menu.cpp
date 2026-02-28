// ============================================================
// TrassarV3 - System menu v2.9  (tryby pracy + menu serwisowe)
// ============================================================

#include "menu.h"
#include "display_manager.h"
#include "rtc_handler.h"
#include "patterns.h"
#include "encoder_distance.h"
#include "painting_engine.h"
#include "statistics.h"
#include "guns.h"
#include "button_handler.h"
#include "report_logger.h"
#include "storage.h"
#include "buzzer.h"
#include "gps_handler.h"

MenuSystem menu;

// ============ Inicjalizacja ============

void MenuSystem::begin() {
    g_state.currentScreen = SCREEN_HOME;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
}

// ============ Przejście między ekranami ============

void MenuSystem::goToScreen(ScreenID screen) {
    // Upewnij sie ze pistolety sa wylaczone przy wyjsciu z czyszczenia
    if (g_state.currentScreen == SCREEN_NOZZLE_CLEAN) {
        guns.allOff();
    }
    g_state.currentScreen = screen;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
}

// ============ Dyspozycja zdarzeń ============

void MenuSystem::handleEvent(ButtonEvent event) {
    if (event == EVT_NONE) return;

    switch (g_state.currentScreen) {
        case SCREEN_HOME:           handleHomeScreen(event);      break;
        case SCREEN_PAINTING:       handlePaintingScreen(event);  break;
        case SCREEN_SERVICE_MENU:   handleServiceMenu(event);     break;
        case SCREEN_CALIBRATION:    handleCalibration(event);     break;
        case SCREEN_DISTANCE_METER: handleDistanceMeter(event);   break;
        case SCREEN_REPORTS:        handleReports(event);         break;
        case SCREEN_NOZZLE_CLEAN:   handleNozzleClean(event);     break;
        case SCREEN_SETUP:          handleSetup(event);           break;
        case SCREEN_SESSION_RESET:  handleSessionReset(event);    break;
        case SCREEN_SUMMARY:        handleSummary(event);         break;
    }
}

// ============ SCREEN_HOME ============

void MenuSystem::handleHomeScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            paintEngine.start();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_START_LONG:
            // Dlugie przytrzymanie START na HOME = ekran przygotowania (SETUP)
            if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
                setupCursor = 0;
                setupMode = (int)g_state.machineMode;
                setupSmart = paintEngine.isSmartSwitch();
                setupGapStart = false;
                goToScreen(SCREEN_SETUP);
            }
            break;

        case EVT_GAP_START:
            // Dedykowany przycisk "Start od przerwy" (GPIO 7)
            paintEngine.startFromGap();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        case EVT_SELECT_SHORT:
            // Odwracanie wzorca (tylko P-3a, P-3b)
            if (patternMgr.getCurrent().hasReverse) {
                patternMgr.toggleReverse();
                g_state.displayNeedsUpdate = true;
            }
            break;

        default:
            break;
    }
}

// ============ SCREEN_PAINTING ============

void MenuSystem::handlePaintingScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (g_state.machineMode == MODE_SEMI_AUTO &&
                g_state.machineState == STATE_PAINTING &&
                paintEngine.isSemiLineComplete()) {
                // Semi-auto: START wyzwala kolejna linie
                paintEngine.semiNextLine();
                g_state.displayNeedsUpdate = true;
            } else if (g_state.machineMode == MODE_MANUAL) {
                // Manual: ignoruj krotkie START (trzymanie = strzal w update)
                // Ale jesli na pauzie, wznow
                if (g_state.machineState == STATE_PAUSED) {
                    paintEngine.resume();
                }
            } else {
                // Auto / Semi (nie czeka na linie): pauza/wznowienie
                if (g_state.machineState == STATE_PAINTING) {
                    paintEngine.pause();
                } else if (g_state.machineState == STATE_PAUSED) {
                    paintEngine.resume();
                }
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT: {
            // Zachowaj dane podsumowania PRZED zatrzymaniem
            summaryDist = stats.getSessionDistance();
            summaryArea = stats.getSessionArea();
            summaryTime = stats.getSessionTimeSec();
            if (summaryTime > 0) {
                float distKm = summaryDist / 1000.0f;
                float timeH = (float)summaryTime / 3600.0f;
                summaryAvgSpeed = (timeH > 0) ? (distKm / timeH) : 0;
            } else {
                summaryAvgSpeed = 0;
            }
            strncpy(summaryPatCode, patternMgr.getCurrent().code, sizeof(summaryPatCode) - 1);
            summaryHasGps = gpsHandler.hasFix();
            summaryLat = summaryHasGps ? gpsHandler.getLat() : 0;
            summaryLon = summaryHasGps ? gpsHandler.getLng() : 0;

            paintEngine.stop();
            goToScreen(SCREEN_SUMMARY);
            break;
        }

        case EVT_SELECT_SHORT:
            // Odwracanie wzorca (tylko P-3a, P-3b)
            if (patternMgr.getCurrent().hasReverse) {
                paintEngine.toggleReverse();
                g_state.displayNeedsUpdate = true;
            }
            break;

        default:
            break;
    }
}

// ============ SCREEN_SETUP (PRZYGOTOWANIE) ============
// SEL(krotki)  = kursor w dol (0->1->2->0)
// STOP(krotki) = kursor w gore (2->1->0->2)
// SEL(dlugi)   = zmien wartosc wybranej opcji
// START        = rozpocznij malowanie z biezacymi ustawieniami
// STOP(dlugi)  = powrot do HOME bez zmian

void MenuSystem::handleSetup(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            // Kursor w dol
            setupCursor++;
            if (setupCursor > 2) setupCursor = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            // Kursor w gore
            setupCursor--;
            if (setupCursor < 0) setupCursor = 2;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            // Zmien wartosc wybranej opcji
            switch (setupCursor) {
                case 0:  // Tryb pracy: AUTO -> SEMI -> RECZNY -> AUTO
                    setupMode++;
                    if (setupMode > 2) setupMode = 0;
                    break;
                case 1:  // Przelaczanie: Smart <-> Instant
                    setupSmart = !setupSmart;
                    break;
                case 2:  // Start: Normalny <-> Od przerwy
                    setupGapStart = !setupGapStart;
                    break;
            }
            buzzer.beep(1500, 60);
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_START_SHORT:
        case EVT_START_LONG: {
            // Zapisz ustawienia i rozpocznij malowanie
            MachineMode newMode = (MachineMode)setupMode;
            if (newMode != g_state.machineMode) {
                g_state.machineMode = newMode;
                storage.saveMode(newMode);
            }
            if (setupSmart != paintEngine.isSmartSwitch()) {
                paintEngine.setSmartSwitch(setupSmart);
                storage.saveSwitchMode(setupSmart);
            }

            const char* modeNames[] = {"AUTO", "SEMI-AUTO", "RECZNY"};
            Serial.printf("[MENU] SETUP -> Tryb: %s, Smart: %s, Start: %s\n",
                          modeNames[setupMode],
                          setupSmart ? "TAK" : "NIE",
                          setupGapStart ? "OD PRZERWY" : "NORMALNY");

            // Uruchom malowanie
            if (setupGapStart) {
                paintEngine.startFromGap();
            } else {
                paintEngine.start();
            }
            buzzer.beep(2000, 150);
            goToScreen(SCREEN_PAINTING);
            break;
        }

        case EVT_STOP_LONG:
            // Powrot do HOME bez zmian
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SERVICE_MENU  (4 pozycje) ============

void MenuSystem::handleServiceMenu(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            g_state.menuIndex++;
            if (g_state.menuIndex >= SERVICE_MENU_ITEMS) g_state.menuIndex = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            g_state.menuIndex--;
            if (g_state.menuIndex < 0) g_state.menuIndex = SERVICE_MENU_ITEMS - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            switch (g_state.menuIndex) {
                case 0: goToScreen(SCREEN_CALIBRATION);    break;
                case 1: goToScreen(SCREEN_DISTANCE_METER); break;
                case 2: goToScreen(SCREEN_REPORTS);         break;
                case 3:
                    nozzlePatternIdx = (int)g_state.currentPattern;
                    // Ogranicz do predefiniowanych wzorcow
                    if (nozzlePatternIdx >= PatternManager::PREDEFINED_PAT_COUNT)
                        nozzlePatternIdx = 0;
                    goToScreen(SCREEN_NOZZLE_CLEAN);
                    break;
                case 4:
                    goToScreen(SCREEN_SESSION_RESET);
                    break;
            }
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ SCREEN_CALIBRATION ============
// START = rozpocznij/zakoncz pomiar 10m
// STOP(1s) = powrot

void MenuSystem::handleCalibration(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (!encoderDist.isCalibrating()) {
                encoderDist.startCalibration();
            } else {
                encoderDist.finishCalibration();
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            encoderDist.cancelCalibration();
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_DISTANCE_METER ============
// START = start/pauza pomiaru
// STOP  = reset pomiaru
// STOP(1s) = powrot

void MenuSystem::handleDistanceMeter(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (!distMeasuring) {
                // Rozpocznij lub wznow pomiar
                distMeasuring = true;
                distMeterLast = encoderDist.getDistanceMeters();
            } else {
                // Pauza
                distMeasuring = false;
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            // Reset
            distMeasuring = false;
            distMeterValue = 0;
            distMeterLast = encoderDist.getDistanceMeters();
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            distMeasuring = false;
            distMeterValue = 0;
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_REPORTS ============
// STOP(1s) = powrot

void MenuSystem::handleReports(ButtonEvent e) {
    if (e == EVT_STOP_LONG) {
        goToScreen(SCREEN_SERVICE_MENU);
    }
}

// ============ SCREEN_NOZZLE_CLEAN ============
// SEL = zmiana wzorca
// START (trzymaj) = otwiera pistolety na czas trzymania
// STOP(1s) = powrot

void MenuSystem::handleNozzleClean(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            nozzlePatternIdx++;
            if (nozzlePatternIdx >= PatternManager::PREDEFINED_PAT_COUNT)
                nozzlePatternIdx = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            nozzlePatternIdx--;
            if (nozzlePatternIdx < 0)
                nozzlePatternIdx = PatternManager::PREDEFINED_PAT_COUNT - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            guns.allOff();
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SESSION_RESET ============
// START = potwierdz reset (zeruj liczniki sesji)
// STOP  = anuluj (powrot do menu serwisowego)

void MenuSystem::handleSessionReset(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
        case EVT_START_LONG: {
            // Reset licznikow sesji
            stats.resetSession();
            encoderDist.resetDistance();
            g_state.machineState = STATE_IDLE;
            buzzer.beep(2000, 150);  // Sygnal potwierdzenia
            Serial.println("[MENU] Reset etapu - liczniki wyzerowane");
            goToScreen(SCREEN_HOME);
            break;
        }

        case EVT_STOP_SHORT:
        case EVT_STOP_LONG:
            // Anuluj - powrot do menu
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SUMMARY ============
// START         = kontynuuj malowanie (wznow z biezacymi licznikami)
// STOP (krotki) = nowy etap (reset licznikow, powrot do HOME)
// STOP (dlugi)  = powrot do HOME bez resetu

void MenuSystem::handleSummary(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
        case EVT_START_LONG:
            // Kontynuuj malowanie — zachowaj liczniki sesji
            paintEngine.start();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_STOP_SHORT: {
            // Nowy etap — zeruj liczniki sesji
            stats.resetSession();
            encoderDist.resetDistance();
            g_state.machineState = STATE_IDLE;
            buzzer.beep(2000, 100);
            Serial.println("[MENU] Podsumowanie -> Nowy etap (reset sesji)");
            goToScreen(SCREEN_HOME);
            break;
        }

        case EVT_STOP_LONG:
            // Powrot do HOME bez resetu (zachowaj liczniki)
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ Renderowanie + logika ciagla ============

void MenuSystem::update() {
    // --- Logika ciagla: pomiar dystansu ---
    if (g_state.currentScreen == SCREEN_DISTANCE_METER && distMeasuring) {
        float current = encoderDist.getDistanceMeters();
        float delta = current - distMeterLast;
        distMeterLast = current;
        if (delta > 0) distMeterValue += delta;
    }

    // --- Logika ciagla: czyszczenie dysz ---
    if (g_state.currentScreen == SCREEN_NOZZLE_CLEAN) {
        bool held = buttons.isStartHeld();
        const PatternDef& pat = patternMgr.getPattern((PatternID)nozzlePatternIdx);
        for (int i = 0; i < NUM_GUNS; i++) {
            bool active = (pat.guns[i].mode != GUN_OFF);
            guns.setGun((GunID)i, held && active);
        }
        // Wymusz odswiezanie zeby pokazac stan pistoletow
        g_state.displayNeedsUpdate = true;
    }

    // --- Renderowanie ---
    if (!g_state.displayNeedsUpdate) return;

    bool fullRedraw = g_state.forceFullRedraw;
    g_state.displayNeedsUpdate = false;
    g_state.forceFullRedraw = false;

    // Pelne czyszczenie tylko przy zmianie ekranu (eliminacja migania)
    if (fullRedraw) {
        display.clear();
    }

    switch (g_state.currentScreen) {

        // ---- Ekran glowny ----
        case SCREEN_HOME: {
            const PatternDef& pat = patternMgr.getCurrent();
            display.drawHomeScreen(
                pat.code,
                pat.name,
                encoderDist.getSpeedKmh(),
                stats.getSessionArea(),
                pat.guns,
                g_state.patternReversed,
                pat.hasReverse
            );
            break;
        }

        // ---- Ekran malowania ----
        case SCREEN_PAINTING: {
            const PatternDef& pat = patternMgr.getCurrent();
            bool gunStates[6];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunStates[i] = guns.getState(i);
            }
            display.drawPaintingScreen(
                g_state.machineState,
                pat.code,
                encoderDist.getSpeedKmh(),
                stats.getSessionArea(),
                pat.guns,
                gunStates,
                g_state.patternReversed,
                paintEngine.isGapStart(),
                paintEngine.isOverspeed(),
                paintEngine.isLowSpeed(),
                stats.getSessionTimeSec(),
                stats.getSessionDistance(),
                paintEngine.getPatternDistance()
            );
            break;
        }

        // ---- Menu serwisowe ----
        case SCREEN_SERVICE_MENU:
            display.drawServiceMenu(g_state.menuIndex);
            break;

        // ---- Kalibracja ----
        case SCREEN_CALIBRATION:
            display.drawCalibrationScreen(
                encoderDist.isCalibrating(),
                encoderDist.getCalibrationPulses(),
                encoderDist.getPulsesPerMeter(),
                encoderDist.isCalibrated()
            );
            break;

        // ---- Pomiar dystansu ----
        case SCREEN_DISTANCE_METER:
            display.drawDistanceMeter(distMeterValue, distMeasuring);
            break;

        // ---- Raporty ----
        case SCREEN_REPORTS: {
            char lastReport[128] = {};
            reportLogger.getLastReport(lastReport, sizeof(lastReport));
            display.drawReportsScreen(
                reportLogger.isReady(),
                reportLogger.getReportCount(),
                lastReport
            );
            break;
        }

        // ---- Czyszczenie dysz ----
        case SCREEN_NOZZLE_CLEAN: {
            const PatternDef& pat = patternMgr.getPattern((PatternID)nozzlePatternIdx);
            bool gunStates[6];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunStates[i] = guns.getState(i);
            }
            display.drawNozzleClean(pat.code, pat.name, pat.guns, gunStates);
            break;
        }

        // ---- Ekran przygotowania (SETUP) ----
        case SCREEN_SETUP:
            display.drawSetupScreen(setupCursor, (MachineMode)setupMode,
                                    setupSmart, setupGapStart);
            break;

        // ---- Reset etapu ----
        case SCREEN_SESSION_RESET:
            display.drawSessionResetScreen(
                stats.getSessionDistance(),
                stats.getSessionArea(),
                stats.getSessionTimeSec()
            );
            break;

        // ---- Podsumowanie etapu ----
        case SCREEN_SUMMARY:
            display.drawSummaryScreen(
                summaryPatCode, summaryDist, summaryArea,
                summaryTime, summaryAvgSpeed,
                summaryHasGps, summaryLat, summaryLon
            );
            break;
    }
}
