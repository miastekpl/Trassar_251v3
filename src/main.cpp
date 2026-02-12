// ============================================================
// TrassarV3 - Komputer pokładowy malowarki pasów drogowych
// Firmware v2.0.0
//
// Platforma:    ESP32-S3 N16R8
// Wyświetlacz:  ILI9341 2.8" 240x320 SPI
// RTC:          DS1307
// Wejścia:      3x BS-33B + enkoder obrotowy
// Wyjścia:      6x przekaźnik (pistolety P1-P6)
// Sieć:         WiFi AP + serwer HTTP
// ============================================================

#include "config.h"
#include "display_manager.h"
#include "button_handler.h"
#include "rtc_handler.h"
#include "web_server.h"
#include "menu.h"
#include "patterns.h"
#include "guns.h"
#include "encoder_distance.h"
#include "painting_engine.h"
#include "statistics.h"
#include "storage.h"

// Globalny stan systemu
SystemState g_state;

// Timery
unsigned long lastDisplayRefresh = 0;
unsigned long lastDynamicUpdate = 0;
const unsigned long DISPLAY_REFRESH_MS = 100;
const unsigned long DYNAMIC_UPDATE_MS  = 500;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("  TrassarV3 - Malowarka pasow drogowych");
    Serial.printf("  Firmware v%s  [%s]\n", FW_VERSION, FW_DATE);
    Serial.println("  6 pistoletow, 15 wzorcow, kalibracja");
    Serial.println("==============================================");
    Serial.println();

    // 1. Pamięć trwała (NVS)
    Serial.println("[INIT] Pamiec NVS...");
    storage.begin();

    // 2. Wyświetlacz
    Serial.println("[INIT] Wyswietlacz ILI9341...");
    display.begin();

    // 3. Zegar RTC
    Serial.println("[INIT] Zegar RTC DS1307...");
    if (rtcModule.begin()) {
        Serial.printf("[INIT] Czas: %s\n", rtcModule.getDateTimeStr());
    } else {
        Serial.println("[INIT] UWAGA: RTC niedostepny");
    }

    // 4. Enkoder (dystans + nawigacja)
    Serial.println("[INIT] Enkoder dystansu...");
    encoderDist.begin();

    // 5. Przyciski
    Serial.println("[INIT] Przyciski...");
    buttons.begin();

    // 6. Pistolety (przekaźniki)
    Serial.println("[INIT] Pistolety P1-P6...");
    guns.begin();

    // 7. Wzorce malowania
    Serial.println("[INIT] Wzorce malowania...");
    patternMgr.begin();
    PatternID lastPat = storage.loadLastPattern();
    patternMgr.setPattern(lastPat);

    // 8. Silnik malowania
    Serial.println("[INIT] Silnik malowania...");
    paintEngine.begin();

    // 9. Statystyki
    Serial.println("[INIT] Statystyki...");
    stats.begin();

    // 10. System menu
    Serial.println("[INIT] System menu...");
    menu.begin();

    // 11. WiFi AP + serwer WWW
    Serial.println("[INIT] WiFi AP + serwer WWW...");
    webServer.begin();

    // Ekran powitalny
    delay(1500);

    g_state.currentScreen = SCREEN_HOME;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;

    Serial.println();
    Serial.println("[INIT] System gotowy!");
    Serial.printf("[INIT] Wzorzec: %s\n", patternMgr.getCurrent().code);
    Serial.printf("[INIT] WiFi: %s  http://%s\n",
                  WIFI_AP_SSID, webServer.getIPAddress().c_str());
    Serial.println();
}

void loop() {
    unsigned long now = millis();

    // 1. Odczyt przycisków
    buttons.update();
    ButtonEvent event = buttons.getEvent();
    if (event != EVT_NONE) {
        menu.handleEvent(event);
    }

    // 2. Aktualizacja enkodera (prędkość)
    encoderDist.update();

    // 3. Aktualizacja RTC
    rtcModule.update();

    // 4. Silnik malowania (sterowanie pistoletami)
    paintEngine.update();

    // 5. Dynamiczne odświeżanie ekranu
    if (now - lastDynamicUpdate >= DYNAMIC_UPDATE_MS) {
        lastDynamicUpdate = now;

        if (g_state.currentScreen == SCREEN_HOME ||
            g_state.currentScreen == SCREEN_PAINTING ||
            g_state.currentScreen == SCREEN_CALIBRATION) {
            g_state.displayNeedsUpdate = true;
        }
    }

    // 6. Renderowanie wyświetlacza
    if (now - lastDisplayRefresh >= DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;
        menu.update();
    }

    // 7. Serwer WWW
    webServer.update();

    delay(1);
}
