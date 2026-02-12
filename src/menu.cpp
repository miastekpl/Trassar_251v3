// ============================================================
// TrassarV3 - System menu v2.1  (menu serwisowe)
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
    }
}

// ============ SCREEN_HOME ============

void MenuSystem::handleHomeScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            paintEngine.start();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        case EVT_SELECT_SHORT:
        case EVT_ENC_CW:
            patternMgr.nextPattern();
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CCW:
            patternMgr.prevPattern();
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            if (patternMgr.getCurrent().hasReverse) {
                patternMgr.toggleReverse();
                g_state.displayNeedsUpdate = true;
            }
            break;

        case EVT_ENC_SHORT:
            // Start od przerwy (przycisk enkodera)
            paintEngine.startFromGap();
            break;

        default:
            break;
    }
}

// ============ SCREEN_PAINTING ============

void MenuSystem::handlePaintingScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (g_state.machineState == STATE_PAINTING) {
                paintEngine.pause();
            } else if (g_state.machineState == STATE_PAUSED) {
                paintEngine.resume();
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            paintEngine.stop();
            goToScreen(SCREEN_HOME);
            break;

        case EVT_SELECT_SHORT:
        case EVT_ENC_CW: {
            int next = (int)g_state.currentPattern + 1;
            if (next >= PAT_COUNT) next = 0;
            paintEngine.setPattern((PatternID)next);
            g_state.displayNeedsUpdate = true;
            break;
        }

        case EVT_ENC_CCW: {
            int prev = (int)g_state.currentPattern - 1;
            if (prev < 0) prev = PAT_COUNT - 1;
            paintEngine.setPattern((PatternID)prev);
            g_state.displayNeedsUpdate = true;
            break;
        }

        case EVT_SELECT_LONG:
            paintEngine.toggleReverse();
            g_state.displayNeedsUpdate = true;
            break;

        default:
            break;
    }
}

// ============ SCREEN_SERVICE_MENU  (4 pozycje) ============

void MenuSystem::handleServiceMenu(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
        case EVT_ENC_CW:
            g_state.menuIndex++;
            if (g_state.menuIndex >= SERVICE_MENU_ITEMS) g_state.menuIndex = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CCW:
            g_state.menuIndex--;
            if (g_state.menuIndex < 0) g_state.menuIndex = SERVICE_MENU_ITEMS - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
        case EVT_ENC_SHORT:
            switch (g_state.menuIndex) {
                case 0: goToScreen(SCREEN_CALIBRATION);    break;
                case 1: goToScreen(SCREEN_DISTANCE_METER); break;
                case 2: goToScreen(SCREEN_REPORTS);         break;
                case 3:
                    nozzlePatternIdx = (int)g_state.currentPattern;
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
// ENC/SEL = zmiana wzorca
// START (trzymaj) = otwiera pistolety na czas trzymania
// STOP(1s) = powrot

void MenuSystem::handleNozzleClean(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
        case EVT_ENC_CW:
            nozzlePatternIdx++;
            if (nozzlePatternIdx >= PAT_COUNT) nozzlePatternIdx = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CCW:
            nozzlePatternIdx--;
            if (nozzlePatternIdx < 0) nozzlePatternIdx = PAT_COUNT - 1;
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
        const PatternDef& pat = PatternManager::patterns[nozzlePatternIdx];
        for (int i = 0; i < NUM_GUNS; i++) {
            bool active = (pat.guns[i].mode != GUN_OFF);
            guns.setGun((GunID)i, held && active);
        }
        // Wymusz odswiezanie zeby pokazac stan pistoletow
        g_state.displayNeedsUpdate = true;
    }

    // --- Renderowanie ---
    if (!g_state.displayNeedsUpdate) return;
    g_state.displayNeedsUpdate = false;
    g_state.forceFullRedraw = false;

    switch (g_state.currentScreen) {

        // ---- Ekran glowny ----
        case SCREEN_HOME: {
            const PatternDef& pat = patternMgr.getCurrent();
            display.drawHomeScreen(
                rtcModule.getTimeStr(),
                rtcModule.getDateStr(),
                pat.code,
                pat.name,
                encoderDist.getSpeedKmh(),
                encoderDist.getDistanceMeters(),
                encoderDist.isCalibrated(),
                g_state.patternReversed
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
                stats.getSessionDistance(),
                stats.getSessionArea(),
                stats.getSessionTimeSec(),
                gunStates,
                g_state.patternReversed,
                paintEngine.isGapStart()
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
            const PatternDef& pat = PatternManager::patterns[nozzlePatternIdx];
            bool gunStates[6];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunStates[i] = guns.getState(i);
            }
            display.drawNozzleClean(pat.code, pat.name, pat.guns, gunStates);
            break;
        }
    }
}
