// ============================================================
// TrassarV3 - Implementacja systemu menu v2.0.0
// Komputer pokładowy malowarki pasów drogowych
// ============================================================

#include "menu.h"
#include "display_manager.h"
#include "rtc_handler.h"
#include "web_server.h"
#include "patterns.h"
#include "encoder_distance.h"
#include "painting_engine.h"
#include "statistics.h"
#include "guns.h"

MenuSystem menu;

// ============ Inicjalizacja ============

void MenuSystem::begin() {
    g_state.currentScreen = SCREEN_HOME;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
    timeSettingsField = 0;
}

// ============ Przejście między ekranami ============

void MenuSystem::goToScreen(ScreenID screen) {
    g_state.currentScreen = screen;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
    timeSettingsField = 0;
}

// ============ Dyspozycja zdarzeń ============

void MenuSystem::handleEvent(ButtonEvent event) {
    if (event == EVT_NONE) return;

    switch (g_state.currentScreen) {
        case SCREEN_HOME:           handleHomeScreen(event);     break;
        case SCREEN_PAINTING:       handlePaintingScreen(event); break;
        case SCREEN_MAIN_MENU:      handleMainMenu(event);       break;
        case SCREEN_PATTERN_SELECT: handlePatternSelect(event);  break;
        case SCREEN_CALIBRATION:    handleCalibration(event);    break;
        case SCREEN_STATISTICS:     handleStatistics(event);     break;
        case SCREEN_WIFI_INFO:      handleWifiInfo(event);       break;
        case SCREEN_SYSTEM_INFO:    handleSystemInfo(event);     break;
        case SCREEN_TIME_SETTINGS:  handleTimeSettings(event);   break;
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
            goToScreen(SCREEN_MAIN_MENU);
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
            // Cykl do następnego wzorca
            int next = (int)g_state.currentPattern + 1;
            if (next >= PAT_COUNT) next = 0;
            paintEngine.setPattern((PatternID)next);
            g_state.displayNeedsUpdate = true;
            break;
        }

        case EVT_ENC_CCW: {
            // Cykl do poprzedniego wzorca
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

// ============ SCREEN_MAIN_MENU ============
// 6 pozycji: "Wybor wzorca", "Kalibracja", "Statystyki",
//            "Czas i data", "Info WiFi", "Info systemowe"

void MenuSystem::handleMainMenu(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
        case EVT_ENC_CW:
            g_state.menuIndex++;
            if (g_state.menuIndex >= MAIN_MENU_ITEMS) g_state.menuIndex = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CCW:
            g_state.menuIndex--;
            if (g_state.menuIndex < 0) g_state.menuIndex = MAIN_MENU_ITEMS - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
        case EVT_ENC_SHORT:
            switch (g_state.menuIndex) {
                case 0: goToScreen(SCREEN_PATTERN_SELECT); break;
                case 1: goToScreen(SCREEN_CALIBRATION);    break;
                case 2: goToScreen(SCREEN_STATISTICS);     break;
                case 3: goToScreen(SCREEN_TIME_SETTINGS);  break;
                case 4: goToScreen(SCREEN_WIFI_INFO);      break;
                case 5: goToScreen(SCREEN_SYSTEM_INFO);    break;
            }
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ SCREEN_PATTERN_SELECT ============

void MenuSystem::handlePatternSelect(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
        case EVT_ENC_CW:
            g_state.menuIndex++;
            if (g_state.menuIndex >= PAT_COUNT) g_state.menuIndex = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CCW:
            g_state.menuIndex--;
            if (g_state.menuIndex < 0) g_state.menuIndex = PAT_COUNT - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
        case EVT_ENC_SHORT:
            patternMgr.setPattern((PatternID)g_state.menuIndex);
            goToScreen(SCREEN_HOME);
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_MAIN_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_CALIBRATION ============

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
            goToScreen(SCREEN_MAIN_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_STATISTICS ============

void MenuSystem::handleStatistics(ButtonEvent e) {
    if (e == EVT_STOP_LONG) {
        goToScreen(SCREEN_MAIN_MENU);
    }
}

// ============ SCREEN_WIFI_INFO ============

void MenuSystem::handleWifiInfo(ButtonEvent e) {
    if (e == EVT_STOP_LONG) {
        goToScreen(SCREEN_MAIN_MENU);
    }
}

// ============ SCREEN_SYSTEM_INFO ============

void MenuSystem::handleSystemInfo(ButtonEvent e) {
    if (e == EVT_STOP_LONG) {
        goToScreen(SCREEN_MAIN_MENU);
    }
}

// ============ SCREEN_TIME_SETTINGS ============

void MenuSystem::handleTimeSettings(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            timeSettingsField = (timeSettingsField + 1) % 6;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CW:
        case EVT_ENC_CCW: {
            int dir = (e == EVT_ENC_CW) ? 1 : -1;
            DateTime now = rtcModule.now();
            int vals[6] = {
                now.hour(), now.minute(), now.second(),
                now.day(),  now.month(),  (int)now.year()
            };
            const int maxVals[6] = {23, 59, 59, 31, 12, 2099};
            const int minVals[6] = { 0,  0,  0,  1,  1, 2020};

            vals[timeSettingsField] += dir;
            if (vals[timeSettingsField] > maxVals[timeSettingsField])
                vals[timeSettingsField] = minVals[timeSettingsField];
            if (vals[timeSettingsField] < minVals[timeSettingsField])
                vals[timeSettingsField] = maxVals[timeSettingsField];

            rtcModule.setTime(vals[0], vals[1], vals[2]);
            rtcModule.setDate(vals[5], vals[4], vals[3]);
            g_state.displayNeedsUpdate = true;
            break;
        }

        case EVT_STOP_LONG:
            goToScreen(SCREEN_MAIN_MENU);
            break;

        default:
            break;
    }
}

// ============ Renderowanie ekranów ============

void MenuSystem::update() {
    if (!g_state.displayNeedsUpdate) return;
    g_state.displayNeedsUpdate = false;

    bool fullRedraw = g_state.forceFullRedraw;
    g_state.forceFullRedraw = false;

    switch (g_state.currentScreen) {

        // ---- Ekran główny ----
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
                g_state.patternReversed
            );
            break;
        }

        // ---- Menu główne ----
        case SCREEN_MAIN_MENU:
            display.drawMainMenu(g_state.menuIndex);
            break;

        // ---- Wybór wzorca ----
        case SCREEN_PATTERN_SELECT:
            display.drawPatternSelect(g_state.menuIndex);
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

        // ---- Statystyki ----
        case SCREEN_STATISTICS:
            display.drawStatisticsScreen(
                stats.getSessionDistance(),
                stats.getSessionArea(),
                stats.getSessionTimeSec(),
                stats.getLifetimeDistance(),
                stats.getLifetimeArea(),
                stats.getLifetimePaintTimeSec()
            );
            break;

        // ---- Info WiFi ----
        case SCREEN_WIFI_INFO: {
            String ip = webServer.getIPAddress();
            display.drawWifiInfo(
                WIFI_AP_SSID,
                ip.c_str(),
                webServer.getConnectedClients()
            );
            break;
        }

        // ---- Info systemowe ----
        case SCREEN_SYSTEM_INFO:
            display.drawSystemInfo(
                FW_VERSION,
                ESP.getFreeHeap(),
                millis()
            );
            break;

        // ---- Czas i data ----
        case SCREEN_TIME_SETTINGS:
            display.drawTimeSettings(
                rtcModule.getTimeStr(),
                rtcModule.getDateStr(),
                timeSettingsField
            );
            break;
    }
}
