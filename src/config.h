#pragma once
// ============================================================
// TrassarV3 - Konfiguracja sprzętowa v2.9.0
// Komputer pokładowy malowarki pasów drogowych
// ============================================================

#include <Arduino.h>

// ============ WERSJA FIRMWARE ============
#define FW_VERSION      "2.9.0"
#define FW_NAME         "TrassarV3"
#define FW_DATE         __DATE__

// ============ WIFI AP ============
#define WIFI_AP_SSID    "TrassarV3"
#define WIFI_AP_PASS    "12345678"
#define WIFI_AP_CHANNEL 6
#define WIFI_AP_MAX_CON 4
#define WEB_SERVER_PORT 80

// ============ ILI9341 Display (SPI) ============
// Piny SPI zdefiniowane w platformio.ini (build_flags TFT_eSPI)
#define PIN_TFT_BL      21

// ============ RTC DS1307 (I2C) ============
#define PIN_RTC_SDA      17
#define PIN_RTC_SCL      18

// ============ Enkoder obrotowy (tylko pomiar dystansu/predkosci) ============
#define PIN_ENC_CLK       5
#define PIN_ENC_DT        6
#define ENC_ISR_DEBOUNCE_US   200    // Minimalny odstep miedzy impulsami ISR [us]

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
#define BTN_LONG_PRESS_MS   1000

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
#define MIN_PAINT_SPEED_KMH  3.0f
#define DEFAULT_MAX_PAINT_SPEED_KMH  15.0f  // Prog alarmu przekroczenia predkosci

// ============ Watchdog ============
#define WDT_TIMEOUT_SEC       3    // Timeout watchdoga [s], auto-reset

// ============ Gun keepalive ============
#define GUN_KEEPALIVE_TIMEOUT_MS  300  // Awaryjne guns.allOff() jesli brak update >300ms

// ============ Kalibracja ============
#define DEFAULT_PULSES_PER_METER  100.0f
#define CALIBRATION_DISTANCE_M     10.0f

// ============ Pomiar prędkości ============
#define SPEED_CALC_INTERVAL_MS    250
#define SPEED_FILTER_ALPHA       0.3f

// ============ Detekcja anomalii pistoletów ============
#define GUN_ANOMALY_DISTANCE_M   50.0f   // Min dystans sesji do uruchomienia detekcji [m]
#define GUN_ANOMALY_CHECK_MS     10000   // Interwał sprawdzania anomalii [ms]

// ============ Kolory UI ============
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
    MODE_MANUAL          // Reczne sterowanie (START = strzelaj)
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
    SCREEN_MODE_SELECT
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
};

extern SystemState g_state;

// ============ Konfiguracja wzorca wlasnego (NVS) ============
struct CustomPatternCfg {
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

// ============ Szerokości pistoletów [m] ============
static const float GUN_WIDTHS_M[NUM_GUNS] = {
    0.12f,  // P1 - 12cm
    0.12f,  // P2 - 12cm
    0.12f,  // P3 - 12cm
    0.24f,  // P4 - 24cm
    0.12f,  // P5 - 12cm
    0.24f   // P6 - 24cm
};

// ============ Piny przekaźników ============
static const uint8_t GUN_PINS[NUM_GUNS] = {
    PIN_RELAY_P1, PIN_RELAY_P2, PIN_RELAY_P3,
    PIN_RELAY_P4, PIN_RELAY_P5, PIN_RELAY_P6
};
