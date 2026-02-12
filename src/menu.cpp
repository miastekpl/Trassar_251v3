// ============================================================
// TrassarV3 - Implementacja systemu menu
// ============================================================

#include "menu.h"
#include "display_manager.h"
#include "rtc_handler.h"
#include "web_server.h"

MenuSystem menu;

void MenuSystem::begin() {
    g_state.currentScreen = SCREEN_HOME;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
}

void MenuSystem::goToScreen(ScreenID screen) {
    g_state.currentScreen = screen;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    paintSettingsField = 0;
    timeSettingsField = 0;
    timeEditMode = false;
}

void MenuSystem::handleEvent(ButtonEvent event) {
    if (event == EVT_NONE) return;

    switch (g_state.currentScreen) {
        case SCREEN_HOME:           handleHomeScreen(event); break;
        case SCREEN_MAIN_MENU:      handleMainMenu(event); break;
        case SCREEN_PAINT_SETTINGS: handlePaintSettings(event); break;
        case SCREEN_TIME_SETTINGS:  handleTimeSettings(event); break;
        case SCREEN_WIFI_INFO:      handleWifiInfo(event); break;
        case SCREEN_SYSTEM_INFO:    handleSystemInfo(event); break;
        case SCREEN_PAINTING:       handlePaintingScreen(event); break;
    }
}

void MenuSystem::update() {
    if (!g_state.displayNeedsUpdate) return;
    g_state.displayNeedsUpdate = false;

    switch (g_state.currentScreen) {
        case SCREEN_HOME:
            display.drawHomeScreen(rtcModule.getTimeStr(), rtcModule.getDateStr());
            break;
        case SCREEN_MAIN_MENU:
            display.drawMainMenu(g_state.menuIndex);
            break;
        case SCREEN_PAINT_SETTINGS:
            display.drawPaintSettings(paintSettingsField, g_state.paintSpeed, g_state.paintPasses);
            break;
        case SCREEN_TIME_SETTINGS:
            display.drawTimeSettings(rtcModule.getTimeStr(), rtcModule.getDateStr(), timeSettingsField);
            break;
        case SCREEN_WIFI_INFO: {
            String ip = webServer.getIPAddress();
            display.drawWifiInfo(WIFI_AP_SSID, ip.c_str(), webServer.getConnectedClients());
            break;
        }
        case SCREEN_SYSTEM_INFO:
            display.drawSystemInfo(FW_VERSION, FW_DATE, ESP.getFreeHeap(), millis());
            break;
        case SCREEN_PAINTING: {
            unsigned long elapsed = 0;
            if (g_state.machineState == STATE_RUNNING && g_state.paintStartTime > 0) {
                elapsed = millis() - g_state.paintStartTime - g_state.totalPauseTime;
            } else if (g_state.machineState == STATE_PAUSED && g_state.paintStartTime > 0) {
                elapsed = g_state.pauseStartTime - g_state.paintStartTime - g_state.totalPauseTime;
            }
            display.drawPaintingScreen(g_state.machineState, g_state.paintSpeed,
                                       g_state.currentPass, g_state.paintPasses, elapsed);
            break;
        }
    }
}

// ============ Ekran główny ============
void MenuSystem::handleHomeScreen(ButtonEvent event) {
    switch (event) {
        case EVT_START_SHORT:
            startPainting();
            break;
        case EVT_STOP_LONG:
            goToScreen(SCREEN_MAIN_MENU);
            break;
        default:
            break;
    }
}

// ============ Menu główne ============
void MenuSystem::handleMainMenu(ButtonEvent event) {
    switch (event) {
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
                case 0: goToScreen(SCREEN_PAINT_SETTINGS); break;
                case 1: goToScreen(SCREEN_TIME_SETTINGS); break;
                case 2: goToScreen(SCREEN_WIFI_INFO); break;
                case 3: goToScreen(SCREEN_SYSTEM_INFO); break;
            }
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ Ustawienia malowania ============
void MenuSystem::handlePaintSettings(ButtonEvent event) {
    switch (event) {
        case EVT_SELECT_SHORT:
            paintSettingsField = (paintSettingsField + 1) % 2;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CW:
            if (paintSettingsField == 0) {
                g_state.paintSpeed = min(100, g_state.paintSpeed + 5);
            } else {
                g_state.paintPasses = min(99, g_state.paintPasses + 1);
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CCW:
            if (paintSettingsField == 0) {
                g_state.paintSpeed = max(0, g_state.paintSpeed - 5);
            } else {
                g_state.paintPasses = max(1, g_state.paintPasses - 1);
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_MAIN_MENU);
            break;

        default:
            break;
    }
}

// ============ Ustawienia czasu ============
void MenuSystem::handleTimeSettings(ButtonEvent event) {
    switch (event) {
        case EVT_SELECT_SHORT:
            timeSettingsField = (timeSettingsField + 1) % 6;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_ENC_CW:
        case EVT_ENC_CCW: {
            int dir = (event == EVT_ENC_CW) ? 1 : -1;
            DateTime now = rtcModule.now();
            int vals[6] = {now.hour(), now.minute(), now.second(),
                           now.day(), now.month(), (int)now.year()};
            int maxVals[6] = {23, 59, 59, 31, 12, 2099};
            int minVals[6] = {0, 0, 0, 1, 1, 2020};

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

// ============ Info WiFi ============
void MenuSystem::handleWifiInfo(ButtonEvent event) {
    if (event == EVT_STOP_LONG) {
        goToScreen(SCREEN_MAIN_MENU);
    }
}

// ============ Info systemowe ============
void MenuSystem::handleSystemInfo(ButtonEvent event) {
    if (event == EVT_STOP_LONG) {
        goToScreen(SCREEN_MAIN_MENU);
    }
}

// ============ Ekran malowania ============
void MenuSystem::handlePaintingScreen(ButtonEvent event) {
    switch (event) {
        case EVT_START_SHORT:
            if (g_state.machineState == STATE_RUNNING) {
                pausePainting();
            } else if (g_state.machineState == STATE_PAUSED) {
                resumePainting();
            }
            break;

        case EVT_STOP_SHORT:
            stopPainting();
            break;

        case EVT_ENC_CW:
            if (g_state.machineState == STATE_RUNNING || g_state.machineState == STATE_PAUSED) {
                g_state.paintSpeed = min(100, g_state.paintSpeed + 5);
                g_state.displayNeedsUpdate = true;
            }
            break;

        case EVT_ENC_CCW:
            if (g_state.machineState == STATE_RUNNING || g_state.machineState == STATE_PAUSED) {
                g_state.paintSpeed = max(0, g_state.paintSpeed - 5);
                g_state.displayNeedsUpdate = true;
            }
            break;

        default:
            break;
    }
}

// ============ Sterowanie malowaniem ============

void MenuSystem::startPainting() {
    if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
        g_state.machineState = STATE_RUNNING;
        g_state.paintStartTime = millis();
        g_state.paintElapsed = 0;
        g_state.totalPauseTime = 0;
        g_state.currentPass = 1;
        goToScreen(SCREEN_PAINTING);
        Serial.println("[PAINT] Start malowania");
    }
}

void MenuSystem::pausePainting() {
    if (g_state.machineState == STATE_RUNNING) {
        g_state.machineState = STATE_PAUSED;
        g_state.pauseStartTime = millis();
        g_state.displayNeedsUpdate = true;
        Serial.println("[PAINT] Pauza");
    }
}

void MenuSystem::resumePainting() {
    if (g_state.machineState == STATE_PAUSED) {
        g_state.machineState = STATE_RUNNING;
        g_state.totalPauseTime += millis() - g_state.pauseStartTime;
        g_state.displayNeedsUpdate = true;
        Serial.println("[PAINT] Wznowienie");
    }
}

void MenuSystem::stopPainting() {
    if (g_state.machineState == STATE_RUNNING || g_state.machineState == STATE_PAUSED) {
        g_state.machineState = STATE_STOPPED;
        if (g_state.paintStartTime > 0) {
            unsigned long pauseAdj = 0;
            if (g_state.machineState == STATE_PAUSED) {
                pauseAdj = millis() - g_state.pauseStartTime;
            }
            g_state.paintElapsed = millis() - g_state.paintStartTime - g_state.totalPauseTime - pauseAdj;
        }
        goToScreen(SCREEN_HOME);
        Serial.println("[PAINT] Stop");
    }
}
