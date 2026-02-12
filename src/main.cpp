// ============================================================
// TrassarV3 - Sterownik maszyny malarskiej
// Firmware v1.0.1
//
// Platforma: ESP32-S3 N16R8
// Wyświetlacz: ILI9341 2.8" 240x320 SPI
// RTC: DS1307
// Wejścia: 3x BS-33B + enkoder obrotowy
// Sieć: WiFi AP + serwer HTTP
// ============================================================

#include "config.h"
#include "display_manager.h"
#include "button_handler.h"
#include "rtc_handler.h"
#include "web_server.h"
#include "menu.h"

// Globalny stan systemu
SystemState g_state;

// Timery
unsigned long lastDisplayRefresh = 0;
unsigned long lastTimeUpdate = 0;
const unsigned long DISPLAY_REFRESH_MS = 100;   // Odświeżanie wyświetlacza co 100ms
const unsigned long TIME_UPDATE_MS = 1000;      // Aktualizacja czasu co 1s

void setup() {
    // UART0 na domyślnych pinach (TX=43, RX=44) - COM port
    Serial.begin(115200);
    delay(1000);  // Daj czas na stabilizację UART i zasilania

    Serial.println();
    Serial.println("========================================");
    Serial.println("  TrassarV3 - Sterownik maszyny malarskiej");
    Serial.print("  Firmware v");
    Serial.println(FW_VERSION);
    Serial.println("========================================");
    Serial.println();

    // 1. Inicjalizacja wyświetlacza
    Serial.println("[INIT] Wyswietlacz ILI9341...");
    display.begin();

    // 2. Inicjalizacja RTC
    Serial.println("[INIT] Zegar RTC DS1307...");
    if (rtcModule.begin()) {
        Serial.print("[INIT] Czas: ");
        Serial.println(rtcModule.getDateTimeStr());
    } else {
        Serial.println("[INIT] UWAGA: RTC niedostepny - uzywam czasu wewnetrznego");
    }

    // 3. Inicjalizacja przycisków i enkodera
    Serial.println("[INIT] Przyciski i enkoder...");
    buttons.begin();

    // 4. Inicjalizacja systemu menu
    Serial.println("[INIT] System menu...");
    menu.begin();

    // 5. Uruchomienie WiFi AP i serwera WWW
    Serial.println("[INIT] WiFi Access Point...");
    webServer.begin();

    // Ekran powitalny - krótka pauza
    delay(1500);

    // Przejście do ekranu głównego
    g_state.currentScreen = SCREEN_HOME;
    g_state.displayNeedsUpdate = true;

    Serial.println();
    Serial.println("[INIT] System gotowy!");
    Serial.print("[INIT] Polacz sie z WiFi: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("[INIT] Otworz: http://");
    Serial.println(webServer.getIPAddress());
    Serial.println();
}

void loop() {
    unsigned long now = millis();

    // 1. Obsługa przycisków
    buttons.update();
    ButtonEvent event = buttons.getEvent();
    if (event != EVT_NONE) {
        menu.handleEvent(event);
    }

    // 2. Aktualizacja RTC
    rtcModule.update();

    // 3. Aktualizacja wyświetlacza (cykliczna dla ekranów dynamicznych)
    if (now - lastTimeUpdate >= TIME_UPDATE_MS) {
        lastTimeUpdate = now;

        // Ekran główny - odśwież czas
        if (g_state.currentScreen == SCREEN_HOME) {
            g_state.displayNeedsUpdate = true;
        }
        // Ekran malowania - odśwież czas trwania
        if (g_state.currentScreen == SCREEN_PAINTING &&
            (g_state.machineState == STATE_RUNNING || g_state.machineState == STATE_PAUSED)) {
            g_state.displayNeedsUpdate = true;
        }
    }

    // 4. Renderowanie menu (tylko gdy potrzeba)
    if (now - lastDisplayRefresh >= DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;
        menu.update();
    }

    // 5. Obsługa serwera WWW
    webServer.update();

    // Krótkie opóźnienie, aby nie obciążać procesora
    delay(1);
}
