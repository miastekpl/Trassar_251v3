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
        case SCREEN_MODE_SELECT:    handleModeSelect(event);      break;
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
            // Dlugie przytrzymanie START na HOME = wybor trybu pracy
            if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
                modeSelectIdx = (int)g_state.machineMode;
                goToScreen(SCREEN_MODE_SELECT);
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

        case EVT_STOP_SHORT:
            paintEngine.stop();
            goToScreen(SCREEN_HOME);
            break;

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

// ============ SCREEN_MODE_SELECT ============
// START(krotki) = przejdz do nastepnego trybu
// START(dlugi)  = zatwierdz wybrany tryb
// STOP(krotki)  = powrot bez zmiany
// STOP(dlugi)   = powrot bez zmiany

void MenuSystem::handleModeSelect(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            modeSelectIdx++;
            if (modeSelectIdx > 2) modeSelectIdx = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_START_LONG: {
            // Zatwierdz wybrany tryb
            MachineMode newMode = (MachineMode)modeSelectIdx;
            g_state.machineMode = newMode;
            storage.saveMode(newMode);
            buzzer.beep(2000, 150);  // Sygnal potwierdzenia

            const char* modeNames[] = {"AUTO", "SEMI-AUTO", "RECZNY"};
            Serial.printf("[MENU] Tryb pracy: %s\n", modeNames[modeSelectIdx]);

            goToScreen(SCREEN_HOME);
            break;
        }

        case EVT_STOP_SHORT:
        case EVT_STOP_LONG:
            // Powrot bez zmiany
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
                stats.getSessionDistance()
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

        // ---- Wybor trybu pracy ----
        case SCREEN_MODE_SELECT:
            display.drawModeSelect(modeSelectIdx, g_state.machineMode);
            break;
    }
}
