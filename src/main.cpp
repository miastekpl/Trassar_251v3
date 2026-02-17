// ============================================================
// TrassarV3 - Komputer pokładowy malowarki pasów drogowych
// Firmware v2.7.0
//
// Platforma:    ESP32-S3 N16R8 (dual-core)
// Wyświetlacz:  ILI9341 2.8" 240x320 SPI
// RTC:          DS1307
// Wejścia:      3x BS-33B + enkoder obrotowy
// Wyjścia:      6x przekaźnik (pistolety P1-P6)
// Sieć:         WiFi AP + serwer HTTP (Core 0)
// Krytyczna pętla:  Core 1 (enkoder, pistolety, buzzer)
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
#include "report_logger.h"
#include "buzzer.h"
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>

// Globalny stan systemu
SystemState g_state;

// Stan detekcji anomalii pistoletow
GunAnomalyState gunAnomaly;

// Timery
unsigned long lastDisplayRefresh = 0;
unsigned long lastDynamicUpdate = 0;
unsigned long lastDiagPrint = 0;
unsigned long lastLifetimeSave = 0;
unsigned long lastReportCacheRefresh = 0;
const unsigned long DISPLAY_REFRESH_MS = 100;
const unsigned long DYNAMIC_UPDATE_MS  = 500;
const unsigned long DIAG_PRINT_MS      = 30000;  // Diagnostyka co 30s
const unsigned long LIFETIME_SAVE_MS   = 60000;  // Zapis statystyk co 60s
const unsigned long REPORT_CACHE_MS    = 15000;  // Odswiezanie cache raportow SD co 15s

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
    // WAZNE: Deselect karty SD PRZED inicjalizacja TFT!
    // SD i TFT wspoldziela HSPI - jesli SD_CS jest LOW (floating),
    // karta SD odpowiada na ruch SPI i psuje obraz TFT.
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    Serial.println("[INIT] Wyswietlacz ILI9341...");
    display.begin();

    // 2b. Buzzer
    Serial.println("[INIT] Buzzer...");
    buzzer.begin();

    // 3. Zegar RTC
    Serial.println("[INIT] Zegar RTC DS1307...");
    if (rtcModule.begin()) {
        Serial.printf("[INIT] Czas: %s\n", rtcModule.getDateTimeStr());
    } else {
        Serial.println("[INIT] UWAGA: RTC niedostepny");
        buzzer.play(BUZ_ERROR);
    }

    // 4. Enkoder (pomiar dystansu/predkosci)
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

    // 10. Karta SD (raporty)
    Serial.println("[INIT] Karta SD...");
    if (!reportLogger.begin()) {
        Serial.println("[INIT] UWAGA: Karta SD niedostepna!");
        buzzer.play(BUZ_ERROR);
    }

    // 11. System menu
    Serial.println("[INIT] System menu...");
    menu.begin();

    // 12. WiFi AP + serwer WWW
    Serial.println("[INIT] WiFi AP + serwer WWW...");
    webServer.begin();

    // Ekran powitalny
    delay(1500);

    // Wyrzuc szum enkodera nazbierany podczas inicjalizacji
    encoderDist.resetDistance();

    g_state.currentScreen = SCREEN_HOME;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;

    // Wczytaj prog predkosci maks. z NVS
    float maxSpd = storage.loadMaxSpeed();
    paintEngine.setMaxSpeed(maxSpd);

    // Watchdog timer - 3s timeout, auto-reset przy zawieszeniu
    Serial.println("[INIT] Watchdog timer...");
    esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
    esp_task_wdt_add(NULL);

    Serial.println();
    Serial.println("[INIT] System gotowy!");
    Serial.printf("[INIT] Wzorzec: %s\n", patternMgr.getCurrent().code);
    Serial.printf("[INIT] Max predkosc: %.1f km/h\n", maxSpd);
    Serial.printf("[INIT] WiFi: %s  http://%s\n",
                  WIFI_AP_SSID, webServer.getIPAddress().c_str());
    Serial.println();
}

void loop() {
    unsigned long now = millis();

    // Watchdog reset - jesli loop() sie zawiesi, ESP zresetuje sie po 3s
    esp_task_wdt_reset();

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

    // 4b. Gun keepalive - awaryjne wylaczenie jesli update() nie dziala
    paintEngine.checkGunKeepAlive();

    // 4c. Buzzer - obsluga sekwencji tonow (non-blocking)
    buzzer.update();

    // 5. Dynamiczne odświeżanie ekranu
    if (now - lastDynamicUpdate >= DYNAMIC_UPDATE_MS) {
        lastDynamicUpdate = now;

        if (g_state.currentScreen == SCREEN_HOME ||
            g_state.currentScreen == SCREEN_PAINTING ||
            g_state.currentScreen == SCREEN_CALIBRATION ||
            g_state.currentScreen == SCREEN_DISTANCE_METER ||
            g_state.currentScreen == SCREEN_NOZZLE_CLEAN) {
            g_state.displayNeedsUpdate = true;
        }
    }

    // 6. Renderowanie wyświetlacza
    if (now - lastDisplayRefresh >= DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;
        menu.update();
    }

    // 7. Serwer WWW - obsluga HTTP na Core 0 (osobny task FreeRTOS)
    // webServer.update() jest teraz puste - klienci obslugiwani autonomicznie

    // 8. Okresowy zapis statystyk lifetime (co 60s podczas malowania)
    if (g_state.machineState == STATE_PAINTING) {
        if (now - lastLifetimeSave >= LIFETIME_SAVE_MS) {
            lastLifetimeSave = now;
            stats.saveLifetime();
        }
    } else {
        lastLifetimeSave = now;  // Reset timera gdy nie malujemy
    }

    // 9. Diagnostyka systemowa (co 30s)
    if (now - lastDiagPrint >= DIAG_PRINT_MS) {
        lastDiagPrint = now;
        Serial.printf("[DIAG] Heap: %u/%u B (min: %u)  Frag: %.0f%%  WWW-stack: %u  Core: %d\n",
                      ESP.getFreeHeap(),
                      ESP.getHeapSize(),
                      ESP.getMinFreeHeap(),
                      100.0f * (1.0f - (float)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) /
                                        (float)ESP.getFreeHeap()),
                      webServer.getTaskStackHWM(),
                      xPortGetCoreID());
    }

    // 10. Detekcja anomalii pistoletow (co 10s podczas malowania)
    if (g_state.machineState == STATE_PAINTING) {
        if (now - gunAnomaly.lastCheckMs >= GUN_ANOMALY_CHECK_MS) {
            gunAnomaly.lastCheckMs = now;
            float sessionDist = stats.getSessionDistance();
            if (sessionDist >= GUN_ANOMALY_DISTANCE_M) {
                bool anyAnomaly = false;
                for (int i = 0; i < NUM_GUNS; i++) {
                    GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);
                    if (cfg.mode != GUN_OFF && stats.getGunDistance(i) < 1.0f) {
                        gunAnomaly.alert[i] = true;
                        anyAnomaly = true;
                    } else {
                        gunAnomaly.alert[i] = false;
                    }
                }
                gunAnomaly.detected = anyAnomaly;
                if (anyAnomaly && !gunAnomaly.alerted) {
                    gunAnomaly.alerted = true;
                    buzzer.play(BUZ_GUN_ANOMALY);
                    Serial.print("[ANOMALY] Pistolety bez aktywnosci:");
                    for (int i = 0; i < NUM_GUNS; i++) {
                        if (gunAnomaly.alert[i]) Serial.printf(" P%d", i + 1);
                    }
                    Serial.printf(" (dystans sesji: %.1fm)\n", sessionDist);
                }
            }
        }
    } else {
        // Reset anomalii przy zatrzymaniu
        if (gunAnomaly.detected || gunAnomaly.alerted) {
            gunAnomaly.detected = false;
            gunAnomaly.alerted = false;
            for (int i = 0; i < NUM_GUNS; i++) gunAnomaly.alert[i] = false;
        }
    }

    // 11. Odswiezanie cache listy raportow SD (co 15s, na Core 1 - bezpieczny dostep SPI)
    if (now - lastReportCacheRefresh >= REPORT_CACHE_MS) {
        lastReportCacheRefresh = now;
        reportLogger.refreshReportCache();
    }

    delay(1);
}
