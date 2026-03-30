#pragma once
// ============================================================
// TrassarV3 - Konfiguracja sprzętowa v2.52.0  [SAFETY PATCH]
// Komputer pokładowy malowarki pasów drogowych
// ============================================================

#include <Arduino.h>

// ============ WERSJA FIRMWARE ============
// FW_VERSION definiowane w platformio.ini (build_flags) — jedno zrodlo prawdy
#ifndef FW_VERSION
  #define FW_VERSION    "2.52.0"
#endif
#define FW_NAME         "TrassarV3"
#define FW_DATE         __DATE__

// ============ WIFI AP ============
#define WIFI_AP_SSID    "TrassarV3"
// Haslo WiFi generowane dynamicznie z MAC adresu ESP32 (patrz web_server.cpp)
#define WIFI_AP_CHANNEL 6
#define WIFI_AP_MAX_CON 4
#define WEB_SERVER_PORT 80
#define WS_PORT         81       // WebSocket port (push status updates)
#define WS_BROADCAST_MS 500      // Interwał broadcastu WebSocket [ms]

// ============ ILI9341 Display (SPI) ============
// Piny SPI zdefiniowane w platformio.ini (build_flags TFT_eSPI)
#define PIN_TFT_BL      21

// ============ RTC DS1307 (I2C) ============
#define PIN_RTC_SDA      17
#define PIN_RTC_SCL      18

// ============ Enkoder obrotowy (tylko pomiar dystansu/predkosci) ============
#define PIN_ENC_CLK       5
#define PIN_ENC_DT        6
#define ENC_ISR_DEBOUNCE_US    50    // Minimalny odstep miedzy impulsami ISR [us]
                                         // 50us = max 20kHz, bezpieczne do ~55 km/h przy 100 imp/m x4

// ============ Przyciski funkcyjne ============
// GPIO 33-37 zajęte przez Octal PSRAM na N16R8!
#define PIN_BTN_START    38   // Start / Pauza
#define PIN_BTN_STOP     39   // Stop
#define PIN_BTN_SELECT   40   // Selektor
#define PIN_BTN_GAP       7   // Start od przerwy (przycisk na enkoderze)

// ============ Przekaźniki pistoletów (6 szt.) ============
#define PIN_RELAY_P1     41   // Pistolet 1 - oś L, 12cm
#define PIN_RELAY_P2     42   // Pistolet 2 - oś C, 12cm
#define PIN_RELAY_P3      1   // Pistolet 3 - oś R, 12cm
#define PIN_RELAY_P4      2   // Pistolet 4 - oś,   24cm
#define PIN_RELAY_P5      3   // Pistolet 5 - krawędź, 12cm
#define PIN_RELAY_P6      4   // Pistolet 6 - krawędź, 24cm

#define NUM_GUNS          6

// ============ Parametry przycisków ============
#define BTN_DEBOUNCE_MS       50
#define BTN_LONG_PRESS_MS   1500

// ============ Parametry wyświetlacza ============
#define TFT_SCREEN_W        320
#define TFT_SCREEN_H        240
#define TFT_BACKLIGHT_PWM   200
#define TFT_BL_LEDC_CH       0
#define TFT_BL_LEDC_FREQ  5000
#define TFT_BL_LEDC_RES      8

// ============ Karta SD (czytnik w module wyświetlacza) ============
#define PIN_SD_CS            16

// ============ Buzzer (pasywny, LEDC PWM) ============
#define PIN_BUZZER            8
#define BUZZER_LEDC_CH        1    // Kanal LEDC (0 = podswietlenie TFT)

// ============ Bezpieczeństwo malowania ============
#define DEFAULT_MIN_PAINT_SPEED_KMH   3.0f  // Prog niskiej predkosci (pistolety OFF ponizej)
#define DEFAULT_MAX_PAINT_SPEED_KMH  15.0f  // Prog alarmu przekroczenia predkosci

// ============ Auto-pauza przy zatrzymaniu (tryb AUTO) ============
#define AUTO_PAUSE_SPEED_KMH   0.5f   // Prog predkosci do auto-pauzy [km/h]
#define AUTO_PAUSE_DELAY_MS   1500     // Opoznienie przed auto-pauza [ms]
#define AUTO_PAUSE_ZERO_PULSE_MS  5000 // Auto-pauza gdy 0 impulsow z enkodera (5s) [ms]
#define ENCODER_ZERO_SPEED_THRESHOLD  2  // Ile cykli zerowych predkosci = pewne zatrzymanie

// ============ Motogodziny (MTH) ============
#define MTH_SAVE_INTERVAL_MS  300000UL  // Zapis MTH co 5 min

// ============ Watchdog ============
#define WDT_TIMEOUT_SEC       3    // Timeout watchdoga [s], auto-reset

// ============ Gun keepalive ============
#define GUN_KEEPALIVE_TIMEOUT_MS  300  // Awaryjne guns.allOff() jesli brak update >300ms

// ============ Bezpieczenstwo — overspeed wylacza pistolety ============
#define OVERSPEED_GUN_DISABLE    true  // true = pistolety OFF przy przekroczeniu maxSpeedKmh

// ============ Bezpieczenstwo — niski heap ============
#define LOW_HEAP_CRITICAL_BYTES  32768  // <32KB = redukcja funkcji (wylacz WS broadcast)
#define LOW_HEAP_WARNING_BYTES   65536  // <64KB = ostrzezenie w logu

// ============ Auto-resume — cooldown po wznowieniu ============
#define AUTO_RESUME_COOLDOWN_MS  2000   // Min czas malowania po auto-resume przed ponowna auto-pauza

// ============ Detekcja zablokowanego przekaznika ============
#define GUN_RELAY_STUCK_CHECK_MS   5000   // Interwał sprawdzania zablokowanych przekaznikow [ms]
#define GUN_RELAY_MAX_CONT_ON_MS  60000   // Max ciagly czas ON bez cyklowania = podejrzenie zablokowania [ms]

// ============ Farba — ostrzezenie o niskim poziomie ============
#define LOW_PAINT_WARNING_PCT     15     // Ostrzezenie (zolty) ponizej 15% zbiornika
#define LOW_PAINT_CRITICAL_PCT     5     // Alarm (czerwony) ponizej 5% zbiornika
#define PAINT_LEVEL_SAVE_MS    30000UL   // Zapis poziomu farby do NVS co 30s malowania

// ============ Alarmy predkosci — interwaly buzzera ============
#define LOW_SPEED_BUZZ_REPEAT_MS  3000   // Powtarzaj alarm niskiej predkosci co 3s
#define OVERSPEED_BUZZ_REPEAT_MS  2000   // Powtarzaj alarm przekroczenia co 2s

// ============ Buzzer — tony potwierdzenia (ad-hoc beepy) ============
#define BUZ_CONFIRM_FREQ          1500   // Czestotliwosc potwierdzenia [Hz]
#define BUZ_CONFIRM_DURATION_MS     80   // Czas trwania potwierdzenia [ms]
#define BUZ_SEMI_LINE_FREQ        1000   // Czestotliwosc sygnalu konca linii semi-auto [Hz]
#define BUZ_SEMI_LINE_DURATION_MS   50   // Czas trwania sygnalu konca linii [ms]

// ============ Kalibracja ============
#define DEFAULT_PULSES_PER_METER  100.0f
#define CALIBRATION_DISTANCE_M     10.0f

// ============ Joystick analogowy KY-023 ============
#define PIN_JOY_VRX      19   // Os pozioma (lewo/prawo) — ADC2_CH8
#define PIN_JOY_VRY      20   // Os pionowa (gora/dol)   — ADC2_CH9
#define PIN_JOY_SW       46   // Przycisk wciskany — INPUT_PULLUP (strap pin: nie wciskac przy wlaczaniu!)

#define JOY_DEAD_ZONE        500   // Strefa martwa ±500 z centrum (12-bit ADC, centrum=2048)
#define JOY_HYSTERESIS       150   // Histereza ADC — wejscie w kierunek wymaga DEAD_ZONE+HYSTERESIS,
                                   // powrot do centrum wymaga < DEAD_ZONE (redukcja szumu ADC2/WiFi)
#define JOY_INITIAL_DELAY_MS 400   // Opoznienie przed auto-repeat [ms]
#define JOY_REPEAT_MS        200   // Interwał auto-repeat [ms]

// ============ Expander MCP23017 (I2C — przyciski wzorców) ============
// MCP23017 na tej samej magistrali I2C co RTC DS1307 (SDA=17, SCL=18)
#define MCP23017_I2C_ADDR     0x20   // Adres I2C (A0=A1=A2=GND)
#define MCP23017_NUM_BUTTONS    15   // 15 przycisków = 15 wzorców predefiniowanych
#define MCP23017_SCAN_MS        20   // Interwał skanowania przycisków [ms]
#define MCP23017_BUTTON_MASK  0x7FFF // Bity 0..14 (GPA0-7 + GPB0-6)

// ============ GPS NEO-6M (UART2) ============
#define PIN_GPS_RX       47   // ESP32 RX <- GPS TX
#define PIN_GPS_TX       48   // ESP32 TX -> GPS RX
#define GPS_BAUD       9600   // Default NEO-6M baud rate

// ============ Pomiar prędkości ============
#define SPEED_CALC_INTERVAL_MS    250
#define SPEED_FILTER_ALPHA       0.3f

// ============ Zapis trasy GPS (GPX) ============
#define GPX_RECORD_INTERVAL_MS  5000     // Interwał zapisu punktu GPS [ms]
#define GPX_MAX_POINTS          4320     // Max punktów w buforze PSRAM (~6h przy 5s)
                                         // 4320 * 32B = ~135 KB w PSRAM

// ============ Detekcja anomalii pistoletów ============
#define GUN_ANOMALY_DISTANCE_M   50.0f   // Min dystans sesji do uruchomienia detekcji [m]
#define GUN_ANOMALY_CHECK_MS     10000   // Interwał sprawdzania anomalii [ms]

// ============ Kolory UI - tryb dzienny ============
#define COLOR_BG          0x0000
#define COLOR_TEXT         0xFFFF
#define COLOR_HEADER_BG   0x1A3C
#define COLOR_HEADER_TXT  0xFFFF
#define COLOR_ACCENT      0x07E0
#define COLOR_WARNING     0xFBE0
#define COLOR_ERROR       0xF800
#define COLOR_MENU_SEL    0x2A7D
#define COLOR_MENU_TXT    0xC618
#define COLOR_DIVIDER     0x4208
#define COLOR_GUN_ON      0x07E0
#define COLOR_GUN_OFF     0x4208

// ============ Kolory UI - tryb nocny (amber/dark) ============
#define NIGHT_COLOR_BG          0x0000
#define NIGHT_COLOR_TEXT        0xFCC0   // Cieply amber
#define NIGHT_COLOR_HEADER_BG   0x2100   // Ciemny braz
#define NIGHT_COLOR_HEADER_TXT  0xFCC0
#define NIGHT_COLOR_ACCENT      0xFC00   // Pomaranczowy
#define NIGHT_COLOR_WARNING     0xFB00   // Ciemny zolty
#define NIGHT_COLOR_ERROR       0xC000   // Ciemny czerwony
#define NIGHT_COLOR_MENU_SEL    0x4200   // Ciemny amber podswietlenie
#define NIGHT_COLOR_MENU_TXT    0x9B40   // Przygaszony amber
#define NIGHT_COLOR_DIVIDER     0x3180   // Ciemny separator
#define NIGHT_COLOR_GUN_ON      0xFC00   // Pomaranczowy
#define NIGHT_COLOR_GUN_OFF     0x3180   // Ciemny szary-amber

// ============ Stany maszyny ============
enum MachineState : uint8_t {
    STATE_IDLE = 0,
    STATE_PAINTING,
    STATE_PAUSED,
    STATE_STOPPED
};

// ============ Tryby pracy malowania ============
enum MachineMode : uint8_t {
    MODE_AUTO = 0,       // Pelna automatyka (dystans steruje pistoletami)
    MODE_SEMI_AUTO,      // Automatyczna linia, reczna przerwa
    MODE_MANUAL,         // Reczne sterowanie (START = strzelaj)
    MODE_DEMO            // Tryb nauki operatora (bez pistoletow, wizualizacja)
};

// ============ Ekrany ============
enum ScreenID : uint8_t {
    SCREEN_HOME = 0,
    SCREEN_PAINTING,
    SCREEN_SERVICE_MENU,
    SCREEN_CALIBRATION,
    SCREEN_DISTANCE_METER,
    SCREEN_REPORTS,
    SCREEN_NOZZLE_CLEAN,
    SCREEN_SETUP,
    SCREEN_SESSION_RESET,
    SCREEN_COUNTER_RESET,   // Reset wszystkich licznikow (oprocz kalibracji)
    SCREEN_SUMMARY,         // Podsumowanie etapu po STOP
    SCREEN_LIFETIME_STATS,  // Statystyki lifetime
    SCREEN_CUSTOM_PATTERN,  // Edycja wzorca wlasnego
    SCREEN_STATS_EXPORT,    // Eksport statystyk na SD
    SCREEN_FACTORY_RESET,   // Factory reset NVS z ekranu serwisowego
    SCREEN_TANKOWANIE,      // Tankowanie farby (uzupelnianie zbiornika)
    SCREEN_POST             // Power-On Self-Test (diagnostyka startowa)
};

// ============ Identyfikatory wzorców ============
enum PatternID : uint8_t {
    PAT_P1A = 0,   // P-1a Przerywana długa
    PAT_P1B,        // P-1b Przerywana krótka
    PAT_P1C,        // P-1c Wydzielająca
    PAT_P1D,        // P-1d Prowadząca wąska
    PAT_P1E,        // P-1e Prowadząca szeroka
    PAT_P2A,        // P-2a Ciągła wąska
    PAT_P2B,        // P-2b Ciągła szeroka
    PAT_P3A,        // P-3a Przekraczalna długa
    PAT_P3B,        // P-3b Przekraczalna krótka
    PAT_P4,         // P-4  Podwójna ciągła
    PAT_P6,         // P-6  Ostrzegawcza
    PAT_P7A,        // P-7a Krawędziowa przeryw. szer.
    PAT_P7B,        // P-7b Krawędziowa ciągła szer.
    PAT_P7C,        // P-7c Krawędziowa przeryw. wąska
    PAT_P7D,        // P-7d Krawędziowa ciągła wąska
    PAT_CUSTOM,     // Wzorzec wlasny uzytkownika
    PAT_COUNT
};

// ============ Identyfikatory pistoletów ============
enum GunID : uint8_t {
    GUN_P1 = 0,    // Oś jezdni lewy,   12cm
    GUN_P2,         // Oś jezdni środek,  12cm
    GUN_P3,         // Oś jezdni prawy,   12cm
    GUN_P4,         // Oś jezdni szeroki, 24cm
    GUN_P5,         // Krawędź wąska,     12cm
    GUN_P6          // Krawędź szeroka,   24cm
};

// ============ Tryby pracy pistoletu ============
enum GunMode : uint8_t {
    GUN_OFF = 0,
    GUN_CONTINUOUS,
    GUN_DASHED
};

// ============ Konfiguracja pistoletu w wzorcu ============
struct GunPatternCfg {
    GunMode mode;
    float lineLen;    // [m] długość linii (dla DASHED)
    float gapLen;     // [m] długość przerwy (dla DASHED)
};

// ============ Definicja wzorca ============
struct PatternDef {
    const char* code;       // "P-1a"
    const char* name;       // "Przerywana dluga"
    float nominalWidth_cm;  // Nominalna szerokość wzorca
    bool hasReverse;        // Czy obsługuje odwracanie (P-3a, P-3b)
    GunPatternCfg guns[NUM_GUNS];
};

// ============ Globalny stan systemu ============
struct SystemState {
    MachineState machineState = STATE_IDLE;
    MachineMode machineMode = MODE_AUTO;
    ScreenID currentScreen = SCREEN_HOME;
    PatternID currentPattern = PAT_P1A;
    bool patternReversed = false;

    int menuIndex = 0;
    bool displayNeedsUpdate = true;
    bool forceFullRedraw = true;

    bool nightMode = false;           // Tryb nocny (amber UI)
    bool sdCardWarningShown = false;  // Flaga jednorazowego ostrzezenia SD
    bool gpsBufferWarningShown = false; // Flaga ostrzezenia GPS overflow (raz na sesje)

    uint8_t pendingWebEvent = 0;     // Zdarzenie z panelu WWW (Core 0 -> Core 1)
    bool qrDismissed = false;        // Flaga zamkniecia ekranu QR z panelu WWW
};

// ============ Pre-alokowany bufor SD (wspoldzielony) ============
#define SD_BUF_SIZE       512  // Rozmiar bufora operacji SD [bajtow]

// ============ Pre-alokowany bufor SD (unikniecie alokacji na stosie) ============
extern uint8_t g_sdBuf[SD_BUF_SIZE];

extern SystemState g_state;

// ============ Mutex dostepu do g_state (Core 0 ↔ Core 1) ============
extern portMUX_TYPE g_stateMux;

// Makra bezpiecznego dostepu
#define STATE_LOCK()   taskENTER_CRITICAL(&g_stateMux)
#define STATE_UNLOCK() taskEXIT_CRITICAL(&g_stateMux)

// ============ Mutex dostepu do karty SD (SPI wspoldzielone) ============
extern SemaphoreHandle_t g_sdMutex;

// Makra bezpiecznego dostepu do SD (timeout 2s — musi byc < WDT_TIMEOUT_SEC!)
#define SD_LOCK()   (g_sdMutex && xSemaphoreTake(g_sdMutex, pdMS_TO_TICKS(2000)))
#define SD_UNLOCK() do { if (g_sdMutex) xSemaphoreGive(g_sdMutex); } while(0)

// ============ Wersja formatu danych NVS ============
#define NVS_DATA_VERSION  5  // Inkrementuj przy zmianie struktur NVS (v5: versioned CustomPatternCfg)

// ============ Timeout TFT/SD contention [ms] ============
#define TFT_SD_MUTEX_TIMEOUT_MS  50  // Timeout oczekiwania na mutex SD przy renderowaniu TFT
#define TFT_SD_MUTEX_RETRIES      2  // Ile razy ponowic probe zdobycia mutexu SD dla TFT

// ============ Sloty wzorcow wlasnych ============
#define NUM_CUSTOM_SLOTS  3

// ============ Konfiguracja wzorca wlasnego (NVS) ============
#define CUSTOM_PAT_STRUCT_VER  1  // Wersja layoutu struktury CustomPatternCfg

struct CustomPatternCfg {
    uint8_t structVersion;       // Wersja layoutu struktury (migracja NVS)
    uint8_t gunModes[NUM_GUNS];  // GunMode per gun (OFF/CONT/DASHED)
    float lineLen[NUM_GUNS];     // Dlugosc linii [m] per gun (dla DASHED)
    float gapLen[NUM_GUNS];      // Dlugosc przerwy [m] per gun (dla DASHED)
    bool valid;                  // Czy wzorzec jest zdefiniowany
};

// ============ Stan detekcji anomalii pistoletów ============
struct GunAnomalyState {
    bool detected = false;           // Czy wykryto anomalie
    bool alert[NUM_GUNS] = {};       // Ktory pistolet jest anomalny
    unsigned long lastCheckMs = 0;   // Timestamp ostatniego sprawdzenia
    bool alerted = false;            // Czy buzzer juz zagral (raz na wykrycie)
};

extern GunAnomalyState gunAnomaly;

// ============ Stan detekcji zablokowanych przekaznikow ============
struct RelayStuckState {
    bool suspected[NUM_GUNS] = {};           // Podejrzenie zablokowania per pistolet
    unsigned long contOnStartMs[NUM_GUNS] = {};  // Poczatek ciaglego ON
    bool wasOn[NUM_GUNS] = {};               // Poprzedni stan (do detekcji cyklowania)
    unsigned long lastCheckMs = 0;
    bool alerted = false;                    // Czy buzzer juz zagral
};

extern RelayStuckState relayStuck;

// ============ Szerokości pistoletów [m] ============
extern const float GUN_WIDTHS_M[NUM_GUNS];

// ============ Piny przekaźników ============
extern const uint8_t GUN_PINS[NUM_GUNS];
