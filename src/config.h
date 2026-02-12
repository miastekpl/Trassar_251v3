#pragma once
// ============================================================
// TrassarV3 - Konfiguracja sprzętowa
// Wersja: 1.0.1
// ============================================================

#include <Arduino.h>

// ============ WERSJA FIRMWARE ============
#define FW_VERSION      "1.0.1"
#define FW_NAME         "TrassarV3"
#define FW_DATE         __DATE__

// ============ WIFI AP ============
#define WIFI_AP_SSID    "TrassarV3"
#define WIFI_AP_PASS    "12345678"
#define WIFI_AP_CHANNEL 6
#define WIFI_AP_MAX_CON 4
#define WEB_SERVER_PORT 80

// ============ ILI9341 Display (SPI) ============
// Piny zdefiniowane w platformio.ini przez build_flags TFT_eSPI
#define PIN_TFT_BL      21   // Podświetlenie wyświetlacza

// ============ RTC DS1307 (I2C) ============
#define PIN_RTC_SDA      17
#define PIN_RTC_SCL      18

// ============ Enkoder obrotowy ============
#define PIN_ENC_CLK       5   // Encoder A (CLK)
#define PIN_ENC_DT        6   // Encoder B (DT)
#define PIN_ENC_SW        7   // Przycisk enkodera

// ============ Przyciski funkcyjne BS-33B ============
// UWAGA: GPIO 33-37 są zajęte przez Octal PSRAM na module N16R8!
// Używamy GPIO 38-40 które są wolne i bezpieczne.
#define PIN_BTN_START    38   // Start / Pauza
#define PIN_BTN_STOP     39   // Stop
#define PIN_BTN_SELECT   40   // Selektor

// ============ Parametry przycisków ============
#define BTN_DEBOUNCE_MS       50    // Czas debouncingu [ms]
#define BTN_LONG_PRESS_MS   1000    // Czas długiego naciśnięcia [ms]
#define ENC_DEBOUNCE_MS        2    // Debounce enkodera [ms]

// ============ Parametry wyświetlacza ============
#define TFT_SCREEN_W        240
#define TFT_SCREEN_H        320
#define TFT_BACKLIGHT_PWM   200    // Jasność podświetlenia (0-255)
#define TFT_BL_LEDC_CH       0    // Kanał LEDC dla podświetlenia
#define TFT_BL_LEDC_FREQ  5000    // Częstotliwość PWM [Hz]
#define TFT_BL_LEDC_RES      8    // Rozdzielczość PWM [bity]

// ============ Kolory UI ============
#define COLOR_BG          0x0000   // Czarny
#define COLOR_TEXT         0xFFFF   // Biały
#define COLOR_HEADER_BG   0x1A3C   // Ciemnoniebieski
#define COLOR_HEADER_TXT  0xFFFF   // Biały
#define COLOR_ACCENT      0x07E0   // Zielony
#define COLOR_WARNING      0xFBE0   // Żółty
#define COLOR_ERROR        0xF800   // Czerwony
#define COLOR_MENU_SEL    0x2A7D   // Podświetlenie wybranej opcji
#define COLOR_MENU_TXT    0xC618   // Szary tekst
#define COLOR_DIVIDER      0x4208   // Linia podziału
#define COLOR_STATUS_RUN   0x07E0   // Zielony - praca
#define COLOR_STATUS_PAUSE 0xFBE0   // Żółty - pauza
#define COLOR_STATUS_STOP  0xF800   // Czerwony - stop
#define COLOR_STATUS_IDLE  0x4208   // Szary - bezczynność

// ============ Stany maszyny ============
enum MachineState {
    STATE_IDLE = 0,      // Bezczynność - gotowy
    STATE_RUNNING,       // Malowanie w toku
    STATE_PAUSED,        // Pauza
    STATE_STOPPED,       // Zatrzymany
    STATE_ERROR          // Błąd
};

// ============ Ekrany menu ============
enum ScreenID {
    SCREEN_HOME = 0,         // Ekran główny
    SCREEN_MAIN_MENU,        // Menu główne
    SCREEN_PAINT_SETTINGS,   // Ustawienia malowania
    SCREEN_TIME_SETTINGS,    // Ustawienia czasu
    SCREEN_WIFI_INFO,        // Informacje WiFi
    SCREEN_SYSTEM_INFO,      // Informacje systemowe
    SCREEN_PAINTING          // Ekran malowania (aktywny proces)
};

// ============ Struktura globalnego stanu ============
struct SystemState {
    MachineState machineState = STATE_IDLE;
    ScreenID currentScreen = SCREEN_HOME;
    int menuIndex = 0;
    int menuItemCount = 0;

    // Parametry malowania
    int paintSpeed = 50;          // Prędkość malowania [%]
    int paintPasses = 1;          // Liczba przejść
    int currentPass = 0;          // Aktualny przejazd
    unsigned long paintStartTime = 0;
    unsigned long paintElapsed = 0;
    unsigned long pauseStartTime = 0;
    unsigned long totalPauseTime = 0;

    // Flagi odświeżania
    bool displayNeedsUpdate = true;
    bool webNeedsUpdate = true;
};

extern SystemState g_state;
