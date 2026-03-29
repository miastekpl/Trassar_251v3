// ============================================================
// TrassarV3 - Komputer pokładowy malowarki pasów drogowych
// Firmware - wersja definiowana w platformio.ini (FW_VERSION)
//
// Platforma:    ESP32-S3 N16R8 (dual-core)
// Wyświetlacz:  ILI9341 2.8" 240x320 SPI
// RTC:          DS1307
// Wejścia:      3x BS-33B + enkoder + joystick KY-023 + 15x MCP23017
// Wyjścia:      6x przekaźnik (pistolety P1-P6)
// Sieć:         WiFi AP + serwer HTTP (Core 0)
// Krytyczna pętla:  Core 1 (enkoder, pistolety, buzzer)
// ============================================================

#include "config.h"
#include "sys_log.h"
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
#include "gps_handler.h"
#include "gps_track.h"
#include "joystick.h"
#include "event_log.h"
#include "nvs_backup.h"
#include "pattern_buttons.h"
#include "paint_consumption.h"
#include "temp_sensor.h"
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <soc/gpio_struct.h>
#include <rom/rtc.h>           // rtc_get_reset_reason() — szczegolowy powod restartu per core

// Globalny stan systemu
SystemState g_state;
portMUX_TYPE g_stateMux = portMUX_INITIALIZER_UNLOCKED;

// Stan detekcji anomalii pistoletow
GunAnomalyState gunAnomaly;

// Stan detekcji zablokowanych przekaznikow
RelayStuckState relayStuck;

// Mutex dostepu do karty SD (SPI wspoldzielone miedzy TFT i SD)
SemaphoreHandle_t g_sdMutex = nullptr;

// Definicje tablic (extern const w config.h — jedna kopia w RAM)
const float GUN_WIDTHS_M[NUM_GUNS] = {
    0.12f,  // P1 - 12cm
    0.12f,  // P2 - 12cm
    0.12f,  // P3 - 12cm
    0.24f,  // P4 - 24cm
    0.12f,  // P5 - 12cm
    0.24f   // P6 - 24cm
};

const uint8_t GUN_PINS[NUM_GUNS] = {
    PIN_RELAY_P1, PIN_RELAY_P2, PIN_RELAY_P3,
    PIN_RELAY_P4, PIN_RELAY_P5, PIN_RELAY_P6
};

// Timery
unsigned long lastDisplayRefresh = 0;
unsigned long lastDynamicUpdate = 0;
unsigned long lastDiagPrint = 0;
unsigned long lastLifetimeSave = 0;
unsigned long lastReportCacheRefresh = 0;
unsigned long lastNvsBackup = 0;
const unsigned long DISPLAY_REFRESH_MS = 100;
const unsigned long DYNAMIC_UPDATE_MS  = 500;
const unsigned long DIAG_PRINT_MS      = 10000;  // Diagnostyka co 10s (debug — normalnie 30s)
const unsigned long LIFETIME_SAVE_MS   = 60000;  // Zapis statystyk co 60s
const unsigned long REPORT_CACHE_MS    = 15000;  // Odswiezanie cache raportow SD co 15s

// ============================================================
// Diagnostyka restartu — logowanie przyczyny po kazdym starcie
// ============================================================
static const char* getResetReasonStr(int reason) {
    switch (reason) {
        case 1:  return "POWERON";
        case 3:  return "SW_RESET (esp_restart)";
        case 4:  return "OWDT_RESET (legacy WDT)";
        case 5:  return "DEEPSLEEP";
        case 6:  return "SDIO_RESET";
        case 7:  return "TG0WDT_SYS (Timer Group 0 WDT)";
        case 8:  return "TG1WDT_SYS (Timer Group 1 WDT)";
        case 9:  return "RTCWDT_SYS (RTC WDT)";
        case 10: return "INTRUSION_RESET";
        case 11: return "TGWDT_CPU (Task WDT)";
        case 12: return "SW_CPU_RESET (software CPU reset)";
        case 13: return "RTCWDT_CPU (RTC WDT CPU)";
        case 14: return "EXT_CPU_RESET (external)";
        case 15: return "RTCWDT_BROWN_OUT (brownout!)";
        case 16: return "RTCWDT_RTC (RTC WDT reset digital)";
        default: return "UNKNOWN";
    }
}

static void logResetDiagnostics() {
    esp_reset_reason_t reason = esp_reset_reason();
    int rtcCore0 = rtc_get_reset_reason(0);
    int rtcCore1 = rtc_get_reset_reason(1);

    DBG_PRINTLN("---------- DIAGNOSTYKA RESTARTU ----------");

    const char* espReasonStr;
    switch (reason) {
        case ESP_RST_POWERON:  espReasonStr = "POWER ON"; break;
        case ESP_RST_SW:       espReasonStr = "SOFTWARE (esp_restart)"; break;
        case ESP_RST_PANIC:    espReasonStr = "PANIC (Guru Meditation!)"; break;
        case ESP_RST_INT_WDT:  espReasonStr = "INTERRUPT WDT"; break;
        case ESP_RST_TASK_WDT: espReasonStr = "TASK WDT (loop/web zawisl!)"; break;
        case ESP_RST_WDT:      espReasonStr = "OTHER WDT"; break;
        case ESP_RST_DEEPSLEEP:espReasonStr = "DEEP SLEEP"; break;
        case ESP_RST_BROWNOUT: espReasonStr = "BROWNOUT (niskie napiecie!)"; break;
        case ESP_RST_SDIO:     espReasonStr = "SDIO"; break;
        default:               espReasonStr = "UNKNOWN"; break;
    }

    DBG_PRINTF("[RESET] Powod: %s (kod: %d)\n", espReasonStr, (int)reason);
    DBG_PRINTF("[RESET] Core 0: %s (kod: %d)\n", getResetReasonStr(rtcCore0), rtcCore0);
    DBG_PRINTF("[RESET] Core 1: %s (kod: %d)\n", getResetReasonStr(rtcCore1), rtcCore1);

    // Krytyczne restarty — dodatkowe ostrzezenie
    if (reason == ESP_RST_PANIC) {
        DBG_PRINTLN("[RESET] !!! PANIC — sprawdz backtrace powyzej (addr2line) !!!");
    } else if (reason == ESP_RST_TASK_WDT) {
        DBG_PRINTLN("[RESET] !!! TASK WDT — ktoras petla (loop/web) zawisla >3s !!!");
    } else if (reason == ESP_RST_INT_WDT) {
        DBG_PRINTLN("[RESET] !!! INTERRUPT WDT — ISR trwal zbyt dlugo !!!");
    } else if (reason == ESP_RST_BROWNOUT) {
        DBG_PRINTLN("[RESET] !!! BROWNOUT — sprawdz zasilanie 5V/3.3V !!!");
    }
    DBG_PRINTLN("-------------------------------------------");

    // Loguj tez do event_log na SD (jesli dostepna pozniej)
    // Zapisujemy do zmiennej globalnej, event_log loguje po inicjalizacji SD
    // (patrz nizej — eventLog.logf po begin())
}

// Zapamietaj powod restartu do zalogowania na SD po inicjalizacji
static esp_reset_reason_t g_lastResetReason;
static int g_lastResetCore0;
static int g_lastResetCore1;

// ============================================================
// Fix #11 (KRYTYCZNE): Wylaczenie pistoletow PRZED resetem WDT
// esp_register_shutdown_handler() wywolywany przez esp_restart()
// oraz przez panic handler — gwarantuje guns OFF przed restartem.
// ============================================================
static void IRAM_ATTR shutdownGunsHandler() {
    // Bezposredni zapis do rejestrow GPIO — bez mutex, bez Serial
    // (kontekst moze byc ISR lub panic handler)
    for (int i = 0; i < NUM_GUNS; i++) {
        uint8_t pin = GUN_PINS[i];
        if (pin < 32) {
            GPIO.out_w1tc = (1UL << pin);
        } else {
            GPIO.out1_w1tc.val = (1UL << (pin - 32));
        }
    }
}

void setup() {
    DBG_BEGIN(115200);
    delay(1000);

    DBG_PRINTLN();
    DBG_PRINTLN("==============================================");
    DBG_PRINTLN("  TrassarV3 - Malowarka pasow drogowych");
    DBG_PRINTF("  Firmware v%s  [%s]\n", FW_VERSION, FW_DATE);
    DBG_PRINTLN("  6 pistoletow, 16 wzorcow, 15 przyciskow, 3 tryby");
    DBG_PRINTLN("==============================================");
    DBG_PRINTLN();

    // DIAGNOSTYKA RESTARTU — pierwsza rzecz po Serial!
    g_lastResetReason = esp_reset_reason();
    g_lastResetCore0 = rtc_get_reset_reason(0);
    g_lastResetCore1 = rtc_get_reset_reason(1);
    logResetDiagnostics();

    // 1. Pamięć trwała (NVS)
    DBG_PRINTLN("[INIT] Pamiec NVS...");
    storage.begin();

    // 2. Wyświetlacz
    // WAZNE: Deselect karty SD PRZED inicjalizacja TFT!
    // SD i TFT wspoldziela HSPI - jesli SD_CS jest LOW (floating),
    // karta SD odpowiada na ruch SPI i psuje obraz TFT.
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    DBG_PRINTLN("[INIT] Wyswietlacz ILI9341...");
    display.begin();

    // 2b. Buzzer
    DBG_PRINTLN("[INIT] Buzzer...");
    buzzer.begin();

    // 3. Zegar RTC
    DBG_PRINTLN("[INIT] Zegar RTC DS1307...");
    if (rtcModule.begin()) {
        DBG_PRINTF("[INIT] Czas: %s%s\n", rtcModule.getDateTimeStr(),
                      rtcModule.isTimeReliable() ? "" : " [FALLBACK: czas kompilacji]");
    } else {
        DBG_PRINTLN("[INIT] UWAGA: RTC niedostepny — timestampy z czasu kompilacji");
        buzzer.play(BUZ_ERROR);
    }

    // 4. Enkoder (pomiar dystansu/predkosci)
    DBG_PRINTLN("[INIT] Enkoder dystansu...");
    encoderDist.begin();

    // 5. Przyciski
    DBG_PRINTLN("[INIT] Przyciski...");
    buttons.begin();

    // 5b. Joystick KY-023
    DBG_PRINTLN("[INIT] Joystick KY-023...");
    joystick.begin();

    // 6. Pistolety (przekaźniki)
    DBG_PRINTLN("[INIT] Pistolety P1-P6...");
    guns.begin();
    guns.beginEmergencyStop();  // Sprzetowy STOP awaryjny (ISR na PIN_BTN_STOP)

    // Fix #11 (KRYTYCZNE): Rejestracja handlera shutdown — pistolety OFF przed resetem
    esp_register_shutdown_handler(shutdownGunsHandler);
    DBG_PRINTLN("[INIT] Shutdown handler (guns OFF) zarejestrowany");

    // 7. Wzorce malowania
    DBG_PRINTLN("[INIT] Wzorce malowania...");
    patternMgr.begin();
    PatternID lastPat = storage.loadLastPattern();
    patternMgr.setPattern(lastPat);

    // 7b. Fizyczne przyciski wzorców (MCP23017 I2C expander)
    DBG_PRINTLN("[INIT] Przyciski wzorcow MCP23017...");
    patternButtons.begin();

    // 8. Silnik malowania
    DBG_PRINTLN("[INIT] Silnik malowania...");
    paintEngine.begin();

    // 9. Statystyki
    DBG_PRINTLN("[INIT] Statystyki...");
    stats.begin();

    // 10. Karta SD (raporty)
    DBG_PRINTLN("[INIT] SD mutex + karta SD...");
    g_sdMutex = xSemaphoreCreateMutex();
    if (!g_sdMutex) {
        DBG_PRINTLN("[INIT] BLAD KRYTYCZNY: Nie mozna utworzyc mutexu SD!");
        buzzer.play(BUZ_ERROR);
        // Kontynuuj bez SD — SD_LOCK() zwroci false dzieki sprawdzeniu nullptr
    }
    if (!reportLogger.begin()) {
        DBG_PRINTLN("[INIT] UWAGA: Karta SD niedostepna!");
        buzzer.play(BUZ_ERROR);
    }

    // 10b. Event log na SD (musi byc po reportLogger)
    DBG_PRINTLN("[INIT] Event log...");
    eventLog.begin();

    // 10c. NVS backup/restore (musi byc po SD + event_log)
    DBG_PRINTLN("[INIT] NVS backup...");
    nvsBackup.begin();

    // 11. GPS (UART2)
    DBG_PRINTLN("[INIT] GPS modul...");
    gpsHandler.begin();

    // 11b. GPS Track recorder (bufor PSRAM + zapis GPX)
    DBG_PRINTLN("[INIT] GPS Track (GPX)...");
    gpsTrack.begin();

    // 11c. Predykcja zuzycia farby
    DBG_PRINTLN("[INIT] Paint consumption...");
    paintConsumption.begin();

    // 11d. Czujnik temperatury DS18B20 (opcjonalny)
    DBG_PRINTLN("[INIT] Czujnik temperatury...");
    tempSensor.begin();

    // 12. System menu
    DBG_PRINTLN("[INIT] System menu...");
    menu.begin();

    // 13. WiFi AP + serwer WWW
    DBG_PRINTLN("[INIT] WiFi AP + serwer WWW...");
    webServer.begin();

    // ======== POST (Power-On Self-Test) ========
    {
        DisplayManager::PostResult post;
        post.sdOk    = reportLogger.isReady();
        post.rtcOk   = rtcModule.isRunning();
        post.gpsOk   = gpsHandler.hasFix();
        post.mcpOk   = patternButtons.isReady();
        post.encOk   = encoderDist.isCalibrated();
        post.tempOk  = tempSensor.isAvailable();
        post.temperature = tempSensor.getTemperature();

        display.clear();
        display.drawPostScreen(post, false);
        delay(800);
        display.drawPostScreen(post, true);

        // Czekaj na START lub timeout 5s
        unsigned long postStart = millis();
        bool postWait = true;
        while (postWait && (millis() - postStart < 5000)) {
            esp_task_wdt_reset();
            buttons.update();
            ButtonEvent pe = buttons.getEvent();
            if (pe == EVT_START_SHORT || pe == EVT_START_LONG) {
                postWait = false;
            }
            delay(10);
        }
        buzzer.beep(2000, 100);
    }

    // ======== QR code WiFi — skanuj smartfonem ========
    {
        display.drawWifiQRScreen(WIFI_AP_SSID,
                                 webServer.getPassword(),
                                 webServer.getIPAddress().c_str());

        // Czekaj na START (bez timeout — operator musi potwierdzic)
        bool qrWait = true;
        while (qrWait) {
            esp_task_wdt_reset();
            buttons.update();
            ButtonEvent qe = buttons.getEvent();
            if (qe == EVT_START_SHORT || qe == EVT_START_LONG) {
                qrWait = false;
            }
            delay(10);
        }
        buzzer.beep(1500, 80);
    }

    // Wyrzuc szum enkodera nazbierany podczas inicjalizacji
    encoderDist.resetDistance();

    g_state.currentScreen = SCREEN_HOME;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;

    // Wczytaj progi predkosci z NVS
    float maxSpd = storage.loadMaxSpeed();
    paintEngine.setMaxSpeed(maxSpd);
    float minSpd = storage.loadMinSpeed();
    paintEngine.setMinSpeed(minSpd);

    // Wczytaj tryb pracy z NVS
    g_state.machineMode = storage.loadMode();

    // Wczytaj tryb przelaczania wzorcow z NVS
    paintEngine.setSmartSwitch(storage.loadSwitchMode());

    // Watchdog timer - 3s timeout, auto-reset przy zawieszeniu.
    // TWDT monitoruje kazdy task niezaleznie (Core 1 loop + Core 0 web).
    // Fix #11: shutdown handler gwarantuje guns OFF przed resetem WDT.
    // Fix #15: Core 0 web task ma wlasny software watchdog (soft recovery
    //          restartuje task zamiast calego ESP — patrz web_server.cpp).
    DBG_PRINTLN("[INIT] Watchdog timer (TWDT per-task)...");
    esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
    esp_task_wdt_add(NULL);  // Dodaj biezacy task (Core 1 loop)

    const char* modeNames[] = {"AUTO", "SEMI-AUTO", "RECZNY"};
    DBG_PRINTLN();
    DBG_PRINTLN("[INIT] System gotowy!");
    DBG_PRINTF("[INIT] Wzorzec: %s\n", patternMgr.getCurrent().code);
    DBG_PRINTF("[INIT] Tryb: %s\n", modeNames[(int)g_state.machineMode]);
    DBG_PRINTF("[INIT] Predkosc: min=%.1f max=%.1f km/h\n", minSpd, maxSpd);
    DBG_PRINTF("[INIT] WiFi: %s  http://%s\n",
                  WIFI_AP_SSID, webServer.getIPAddress().c_str());
    DBG_PRINTLN();

    // Log startu systemu + powod restartu
    eventLog.logf("SYSTEM", "Start v%s | %s | wzorzec=%s tryb=%s min=%.1f max=%.1f",
                  FW_VERSION, rtcModule.getDateTimeStr(),
                  patternMgr.getCurrent().code,
                  modeNames[(int)g_state.machineMode], minSpd, maxSpd);

    // Loguj powod restartu na SD — kluczowe do diagnostyki niestabilnosci
    if (g_lastResetReason != ESP_RST_POWERON) {
        eventLog.logf("RESET", "Powod: %d Core0: %d Core1: %d%s",
                      (int)g_lastResetReason, g_lastResetCore0, g_lastResetCore1,
                      g_lastResetReason == ESP_RST_BROWNOUT ? " BROWNOUT!" :
                      g_lastResetReason == ESP_RST_TASK_WDT ? " TASK_WDT!" :
                      g_lastResetReason == ESP_RST_PANIC    ? " PANIC!" :
                      g_lastResetReason == ESP_RST_INT_WDT  ? " INT_WDT!" : "");
    }

    // Czyszczenie starych logow (>7 dni)
    eventLog.cleanupOldLogs();

    // Pierwszy backup NVS (jesli SD dostepna)
    if (reportLogger.isReady()) {
        nvsBackup.backupToSD();
    }
    lastNvsBackup = millis();
}

// Diagnostyka czasu trwania loop() — detekcja dlugich iteracji
static unsigned long loopStartMs = 0;
static unsigned long loopMaxMs = 0;
static unsigned long loopSlowCount = 0;

void loop() {
    unsigned long now = millis();

    // Diagnostyka: czas trwania poprzedniej iteracji loop()
    if (loopStartMs > 0) {
        unsigned long loopDuration = now - loopStartMs;
        if (loopDuration > loopMaxMs) loopMaxMs = loopDuration;
        if (loopDuration > 500) {
            loopSlowCount++;
            LOG_WARN("LOOP", "Dluga iteracja: %lu ms (max: %lu, slow count: %lu)",
                     loopDuration, loopMaxMs, loopSlowCount);
        }
        if (loopDuration > 2000) {
            LOG_ERROR("LOOP", "KRYTYCZNIE dluga iteracja: %lu ms — ryzyko WDT reset!",
                      loopDuration);
        }
    }
    loopStartMs = now;

    // Watchdog reset - jesli loop() sie zawiesi, ESP zresetuje sie po 3s
    esp_task_wdt_reset();

    // 1. Odczyt przycisków
    buttons.update();
    ButtonEvent event = buttons.getEvent();
    if (event != EVT_NONE) {
        menu.handleEvent(event);
    }

    // 1a2. Zdarzenia z panelu WWW (kolejkowane z Core 0, obslugiwane na Core 1)
    {
        STATE_LOCK();
        uint8_t webEvt = g_state.pendingWebEvent;
        g_state.pendingWebEvent = 0;
        STATE_UNLOCK();
        if (webEvt != 0) {
            menu.handleEvent((ButtonEvent)webEvt);
        }
    }

    // 1b. Odczyt joysticka KY-023
    // requireCenter() w goToScreen() blokuje osie dopoki joystick nie wroci
    // do centrum — eliminuje falszywe zdarzenia z szumu ADC2 (WiFi)
    joystick.update();
    ButtonEvent joyEvent = joystick.getEvent();
    if (joyEvent != EVT_NONE) {
        bool isAxis = joystick.wasAxisEvent();

        // Na ekranach operacyjnych (HOME/PAINTING/SUMMARY) blokuj zdarzenia z osi
        // analogowych — szum ADC moze generowac falszywe EVT_STOP_LONG.
        // Przepuszczamy tylko SW (przycisk).
        bool isOperational = (g_state.currentScreen == SCREEN_HOME ||
                              g_state.currentScreen == SCREEN_PAINTING ||
                              g_state.currentScreen == SCREEN_SUMMARY);

        // Na WSZYSTKICH ekranach: blokuj falszywe EVT_STOP_LONG z osi joysticka
        // krotko po zmianie ekranu (cooldown 600ms). requireCenter() blokuje
        // osie dopoki joystick nie wroci do centrum, ale jesli szum ADC2
        // generuje krotkie skoki wychodzace i wracajace do strefy martwej,
        // moze wygenerowac falszywy event zaraz po powrocie do centrum.
        bool cooldownActive = isAxis &&
            (now - menu.lastScreenChangeMs < 600);

        if (cooldownActive) {
            // Ignoruj — zbyt blisko zmiany ekranu
        } else if (isOperational && isAxis) {
            // Ekrany operacyjne — blokuj wszystkie zdarzenia osi
        } else {
            menu.handleEvent(joyEvent);
        }
    }

    // 1c. Odczyt przycisków wzorców (MCP23017 I2C)
    patternButtons.update();

    // 1d. Sprzetowy STOP awaryjny — synchronizacja stanu po ISR
    if (guns.emergencyStopTriggered) {
        guns.emergencyStopTriggered = false;
        guns.allOff();  // Synchronizuj gunStates[] z fizycznym stanem pinow
        STATE_LOCK();
        MachineState es = g_state.machineState;
        STATE_UNLOCK();
        if (es == STATE_PAINTING || es == STATE_PAUSED) {
            paintEngine.stop();
            menu.goToScreen(SCREEN_HOME);
            eventLog.log("SAFETY", "SPRZETOWY STOP AWARYJNY (ISR) — pistolety wylaczone");
        }
    }

    // 2. Aktualizacja enkodera (prędkość)
    encoderDist.update();

    // 3. Aktualizacja RTC
    rtcModule.update();

    // 3b. Odczyt GPS (UART2)
    gpsHandler.update();

    // 3c. Zapis trasy GPS (co 5s podczas malowania)
    gpsTrack.update();

    // 3d. Czujnik temperatury (odczyt co 5s)
    tempSensor.update();

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
    // Fix #7: Deassert SD CS przed kazda operacja TFT — zapobiega SPI contention.
    // SD i TFT wspoldziela HSPI. Jesli SD CS jest LOW (np. po nieudanej operacji SD),
    // karta SD odpowiada na ruch SPI i psuje rendering TFT.
    if (now - lastDisplayRefresh >= DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;
        digitalWrite(PIN_SD_CS, HIGH);  // Gwarantuj SD CS HIGH przed TFT
        menu.update();
    }

    // Fix #20: WDT reset po renderowaniu TFT — menu.update() moze trwac dlugo
    // (mutex SD do 150ms + SPI rendering do 200ms). W polaczeniu z innymi operacjami
    // w petli (I2C, SD, UART) sumaryczny czas moze przekroczyc 3s WDT timeout.
    esp_task_wdt_reset();

    // 7. Serwer WWW - obsluga HTTP na Core 0 (osobny task FreeRTOS)
    // webServer.update() jest teraz puste - klienci obslugiwani autonomicznie

    // 8. Okresowy zapis statystyk lifetime (co 60s podczas malowania)
    if (g_state.machineState == STATE_PAINTING) {
        if (now - lastLifetimeSave >= LIFETIME_SAVE_MS) {
            lastLifetimeSave = now;
            stats.saveLifetime();
            storage.savePaintLevel(paintConsumption.getCurrentLevel());
        }
    } else {
        lastLifetimeSave = now;  // Reset timera gdy nie malujemy
    }

    // 9. Diagnostyka systemowa (co 30s) + Fix #10: automatyczne dzialanie przy niskim heapie
    if (now - lastDiagPrint >= DIAG_PRINT_MS) {
        lastDiagPrint = now;
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t minFreeHeap = ESP.getMinFreeHeap();
        // Fix #17: Fragmentacja — uzyj MALLOC_CAP_INTERNAL dla obu wartosci.
        // Poprzednio MALLOC_CAP_8BIT zawieralo PSRAM (duzy blok) a freeHeap
        // tylko RAM wewnetrzny, co dawalo -3000% (nonsens).
        uint32_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        float fragPct = (freeHeap > 0) ? 100.0f * (1.0f - (float)largestBlock / (float)freeHeap) : 0;
        uint32_t psramFree = ESP.getFreePsram();
        uint32_t upSec = now / 1000;
        DBG_PRINTF("[DIAG] Up:%lum%lus  Heap:%u/%uB (min:%u) Frag:%.0f%%  PSRAM:%uB  WWW-stk:%u  C1-stk:%u  LoopMax:%lums Slow:%lu\n",
                      upSec / 60, upSec % 60,
                      freeHeap,
                      ESP.getHeapSize(),
                      minFreeHeap,
                      fragPct,
                      psramFree,
                      webServer.getTaskStackHWM(),
                      uxTaskGetStackHighWaterMark(NULL),
                      loopMaxMs,
                      loopSlowCount);
        loopMaxMs = 0;  // Reset max dla nastepnego okresu

        // Fix #10: Automatyczne dzialanie przy niskim heapie
        if (freeHeap < LOW_HEAP_CRITICAL_BYTES) {
            // Krytyczny poziom — wylacz broadcast WebSocket, zatrzymaj malowanie
            DBG_PRINTF("[HEAP] KRYTYCZNY: %u B < %u B — redukcja funkcji!\n",
                          freeHeap, LOW_HEAP_CRITICAL_BYTES);
            eventLog.logf("HEAP", "KRYTYCZNY: %u B wolnego heapa — awaryjne dzialania", freeHeap);

            STATE_LOCK();
            MachineState heapState = g_state.machineState;
            STATE_UNLOCK();
            if (heapState == STATE_PAINTING) {
                paintEngine.stop();
                menu.goToScreen(SCREEN_HOME);
                buzzer.play(BUZ_ERROR);
                eventLog.log("HEAP", "Malowanie zatrzymane — krytycznie niski heap");
            }
        } else if (freeHeap < LOW_HEAP_WARNING_BYTES) {
            DBG_PRINTF("[HEAP] OSTRZEZENIE: %u B < %u B\n",
                          freeHeap, LOW_HEAP_WARNING_BYTES);
        }
    }

    // 10. Detekcja anomalii pistoletow (co 10s podczas malowania)
    //     Caly stan gunAnomaly chroniony STATE_LOCK (czytany z Core 0 przez WebSocket)
    {
        STATE_LOCK();
        MachineState snapState = g_state.machineState;
        unsigned long lastCheck = gunAnomaly.lastCheckMs;
        bool wasAlerted = gunAnomaly.alerted;
        STATE_UNLOCK();

        if (snapState == STATE_PAINTING) {
            if (now - lastCheck >= GUN_ANOMALY_CHECK_MS) {
                // Odczyty stats/patternMgr poza lockiem (maja wlasne mutexy)
                float sessionDist = stats.getSessionDistance();
                bool anyAnomaly = false;
                bool localAlert[NUM_GUNS];

                if (sessionDist >= GUN_ANOMALY_DISTANCE_M) {
                    for (int i = 0; i < NUM_GUNS; i++) {
                        GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);
                        if (cfg.mode != GUN_OFF && stats.getGunDistance(i) < 1.0f) {
                            localAlert[i] = true;
                            anyAnomaly = true;
                        } else {
                            localAlert[i] = false;
                        }
                    }
                } else {
                    for (int i = 0; i < NUM_GUNS; i++) localAlert[i] = false;
                }

                // Atomowa aktualizacja calego stanu anomalii (czytany z Core 0)
                STATE_LOCK();
                gunAnomaly.lastCheckMs = now;
                for (int i = 0; i < NUM_GUNS; i++) gunAnomaly.alert[i] = localAlert[i];
                gunAnomaly.detected = anyAnomaly;
                if (anyAnomaly && !wasAlerted) gunAnomaly.alerted = true;
                STATE_UNLOCK();

                if (anyAnomaly && !wasAlerted) {
                    buzzer.play(BUZ_GUN_ANOMALY);

                    char anomList[32] = "";
                    int pos = 0;
                    for (int i = 0; i < NUM_GUNS; i++) {
                        if (localAlert[i]) {
                            pos += snprintf(anomList + pos, sizeof(anomList) - pos, " P%d", i + 1);
                        }
                    }
                    eventLog.logf("ANOMALY", "Pistolety bez aktywnosci:%s (dist=%.1fm)",
                                  anomList, sessionDist);
                }
            }
        } else {
            // Reset anomalii przy zatrzymaniu
            STATE_LOCK();
            bool needsReset = (gunAnomaly.detected || gunAnomaly.alerted);
            if (needsReset) {
                gunAnomaly.detected = false;
                gunAnomaly.alerted = false;
                for (int i = 0; i < NUM_GUNS; i++) gunAnomaly.alert[i] = false;
            }
            STATE_UNLOCK();
        }
    }

    // 10b. Fix #14: Detekcja zablokowanego przekaznika (co 5s podczas malowania)
    //      Sprawdza czy pistolet DASHED jest ciagly ON dluzej niz GUN_RELAY_MAX_CONT_ON_MS.
    //      Moze wskazywac na mechanicznie zablokowany przekaznik.
    {
        STATE_LOCK();
        MachineState relaySnapState = g_state.machineState;
        STATE_UNLOCK();

        if (relaySnapState == STATE_PAINTING) {
            if (now - relayStuck.lastCheckMs >= GUN_RELAY_STUCK_CHECK_MS) {
                relayStuck.lastCheckMs = now;
                bool anyStuck = false;

                for (int i = 0; i < NUM_GUNS; i++) {
                    bool currentlyOn = guns.getState(i);
                    GunPatternCfg cfg = patternMgr.getGunConfig((GunID)i);

                    if (currentlyOn && !relayStuck.wasOn[i]) {
                        // Wlasnie wlaczony — zapamietaj poczatek
                        relayStuck.contOnStartMs[i] = now;
                    }

                    if (currentlyOn && cfg.mode == GUN_DASHED) {
                        // Pistolet DASHED powinien cyklowac ON/OFF
                        unsigned long onDuration = now - relayStuck.contOnStartMs[i];
                        if (onDuration > GUN_RELAY_MAX_CONT_ON_MS) {
                            relayStuck.suspected[i] = true;
                            anyStuck = true;
                        }
                    } else if (!currentlyOn) {
                        // Reset — pistolet sie wylaczyl (przekaznik dziala)
                        relayStuck.suspected[i] = false;
                        relayStuck.contOnStartMs[i] = 0;
                    }

                    relayStuck.wasOn[i] = currentlyOn;
                }

                if (anyStuck && !relayStuck.alerted) {
                    relayStuck.alerted = true;
                    buzzer.play(BUZ_ERROR);

                    char stuckList[32] = "";
                    int pos = 0;
                    for (int i = 0; i < NUM_GUNS; i++) {
                        if (relayStuck.suspected[i]) {
                            pos += snprintf(stuckList + pos, sizeof(stuckList) - pos, " P%d", i + 1);
                        }
                    }
                    eventLog.logf("RELAY", "Podejrzenie zablokowanego przekaznika:%s (ON > %us)",
                                  stuckList, GUN_RELAY_MAX_CONT_ON_MS / 1000);
                }
            }
        } else {
            // Reset stanu detekcji przy zatrzymaniu
            if (relayStuck.alerted) {
                relayStuck.alerted = false;
                for (int i = 0; i < NUM_GUNS; i++) {
                    relayStuck.suspected[i] = false;
                    relayStuck.contOnStartMs[i] = 0;
                    relayStuck.wasOn[i] = false;
                }
            }
        }
    }

    // 11. Odswiezanie cache listy raportow SD (co 15s, na Core 1 - bezpieczny dostep SPI)
    if (now - lastReportCacheRefresh >= REPORT_CACHE_MS) {
        lastReportCacheRefresh = now;
        reportLogger.refreshReportCache();
        esp_task_wdt_reset();  // Fix #20: WDT reset po operacjach SD
    }

    // 11b. Fix #15+#17: Monitoring zdrowia Core 0 — restart tasku zamiast calego ESP
    // Fix #17: Backoff przy wielokrotnych restartach — zapobiega nieskonczonej
    // petli restart->blokada->restart ktora wyczerpuje zasoby i prowadzi
    // do hardware WDT resetu calego ESP (ekran QR).
    {
        static unsigned long lastCore0Check = 0;
        static unsigned long lastSuccessfulHeartbeat = 0;
        static bool wasAliveLastCheck = true;

        // Dynamiczny timeout: normalnie 10s, po wielu restartach wydluzamy
        unsigned long checkInterval = 5000;
        unsigned long aliveTimeout = 10000;
        if (webServer.restartCount >= TrassarWebServer::MAX_TASK_RESTARTS) {
            // Po MAX restartach — zwolnij sprawdzanie, wydluz timeout
            checkInterval = 30000;
            aliveTimeout = 25000;
        } else if (webServer.restartCount >= 2) {
            // Po 2+ restartach — lekki backoff
            checkInterval = 10000;
            aliveTimeout = 15000;
        }

        if (now - lastCore0Check >= checkInterval) {
            lastCore0Check = now;
            bool alive = webServer.isCore0Alive(now, aliveTimeout);

            if (alive) {
                if (!wasAliveLastCheck && webServer.restartCount > 0) {
                    // Task odzyskal sprawnosc po restartach
                    DBG_PRINTF("[WDT-CORE1] Core 0 task odzyskal sprawnosc (po %u restartach)\n",
                                  webServer.restartCount);
                    webServer.restartCount = 0;
                }
                wasAliveLastCheck = true;
                lastSuccessfulHeartbeat = now;
            } else {
                wasAliveLastCheck = false;
                if (webServer.restartCount < TrassarWebServer::MAX_TASK_RESTARTS) {
                    DBG_PRINTF("[WDT-CORE1] Core 0 web task nie odpowiada — restart #%u!\n",
                                  webServer.restartCount + 1);
                    eventLog.logf("SAFETY", "Core 0 web task restart #%u (bez resetu ESP)",
                                  webServer.restartCount + 1);
                    esp_task_wdt_reset();  // Fix #20: WDT reset przed restartWebTask (moze blokowac na server.stop)
                    webServer.restartWebTask();
                    esp_task_wdt_reset();  // Fix #20: WDT reset po restartWebTask
                } else {
                    // Przekroczono limit restartow — nie restartuj wiecej,
                    // web server jest niedostepny ale ESP dziala stabilnie.
                    // Loguj rzadko (co 5 min) zeby nie zalewac seriala.
                    static unsigned long lastMaxRestartLog = 0;
                    if (now - lastMaxRestartLog >= 300000) {
                        lastMaxRestartLog = now;
                        DBG_PRINTF("[WDT-CORE1] Core 0 task niestabilny (%u restartow) — web server wylaczony\n",
                                      webServer.restartCount);
                        eventLog.logf("SAFETY", "Core 0 task niestabilny (%u restartow) — web wylaczony",
                                      webServer.restartCount);
                    }
                }
            }
        }
    }

    // 12. Okresowy backup NVS na SD (co 30 min) + czyszczenie starych logow
    if (now - lastNvsBackup >= NVS_BACKUP_INTERVAL_MS) {
        lastNvsBackup = now;
        esp_task_wdt_reset();  // Fix #20: WDT reset przed dlugimi operacjami SD
        if (nvsBackup.backupToSD()) {
            eventLog.log("NVS", "Okresowy backup NVS na SD");
        }
        esp_task_wdt_reset();  // Fix #20: WDT reset po backup NVS
        eventLog.cleanupOldLogs();
        esp_task_wdt_reset();  // Fix #20: WDT reset po czyszczeniu logow
    }

    delay(1);
}
