#pragma once
// ============================================================
// TrassarV3 - Modul wyswietlacza ILI9341  320x240 landscape
// Komputer pokladowy malowarki pasow drogowych  v2.12.0
// ============================================================

#include <TFT_eSPI.h>
#include "config.h"

class DisplayManager {
public:
    void begin();
    void setBacklight(uint8_t brightness);
    void clear();

    // ---- Ekrany glowne ----
    void drawHomeScreen(const char* patCode, const char* patName,
                        float speedKmh, float areaM2,
                        const GunPatternCfg gunsCfg[6],
                        bool reversed, bool hasReverse);
    void drawPaintingScreen(MachineState state, const char* patCode,
                            float speedKmh, float areaM2,
                            const GunPatternCfg gunsCfg[6],
                            const bool gunStates[6],
                            bool reversed, bool gapStart = false,
                            bool overspeed = false, bool lowSpeed = false,
                            unsigned long sessionTimeSec = 0,
                            float sessionDistM = 0,
                            float patternPosM = -1.0f,
                            bool waitingForMovement = false);

    // Menu serwisowe
    void drawServiceMenu(int selectedIndex);
    void drawCalibrationScreen(bool active, float pulses, float ppm, bool calibrated);
    void drawDistanceMeter(float distanceM, bool measuring);
    void drawReportsScreen(bool sdReady, int fileCount, const char* lastReport);
    void drawNozzleClean(const char* patCode, const char* patName,
                         const GunPatternCfg guns[6], const bool gunStates[6]);

    // Ekran przygotowania (SETUP)
    void drawSetupScreen(int cursor, MachineMode mode, bool smartSwitch, bool gapStart);

    // Reset etapu (potwierdzenie)
    void drawSessionResetScreen(float distM, float areaM2, unsigned long timeSec);

    // Reset wszystkich licznikow (potwierdzenie)
    void drawCounterResetScreen(float ltDistM, float ltAreaM2, uint32_t ltTimeSec,
                                const uint32_t gunShots[6]);

    // Podsumowanie etapu (po STOP)
    void drawSummaryScreen(const char* patCode, float distM, float areaM2,
                           unsigned long timeSec, float speedAvg,
                           bool hasGps, float lat, float lon);

    // Statystyki lifetime
    void drawLifetimeStatsScreen(float ltDistM, float ltAreaM2, uint32_t ltTimeSec,
                                 const uint32_t gunShots[6], uint32_t mthSec);

    // Edycja wzorca wlasnego
    void drawCustomPatternScreen(int cursor, int gunIdx, const CustomPatternCfg& cfg);

    // Eksport statystyk na SD
    void drawStatsExportScreen(bool exporting, bool success);

    // Factory reset NVS (potwierdzenie)
    void drawFactoryResetScreen();

    // Tankowanie farby (uzupelnianie zbiornika)
    void drawTankowanieScreen(float refuelAmount, bool done);

    // Ostrzezenie GPS buffer overflow na ekranie malowania
    void drawGpsOverflowIcon();

    // POST (Power-On Self-Test)
    struct PostResult {
        bool sdOk;
        bool rtcOk;
        bool gpsOk;
        bool mcpOk;
        bool encOk;
        bool tempOk;
        float temperature;
    };
    void drawPostScreen(const PostResult& result, bool done);

    // Ikona ostrzezenia SD na ekranie malowania
    void drawSdWarningIcon();

    // Ekran QR code WiFi (startowy)
    void drawWifiQRScreen(const char* ssid, const char* password, const char* ip);

    // ---- Elementy pomocnicze ----
    void drawHeader(const char* title);
    void drawGunRects(int y, const GunPatternCfg gunsCfg[6],
                      const bool gunStates[6], bool paused,
                      bool waitingForMovement = false);
    void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color);

    // Tryb nocny
    void applyNightMode(bool night);

    TFT_eSPI& getTFT() { return tft; }

    // Kolory dynamiczne (zmieniane przez tryb nocny)
    uint16_t cBg          = COLOR_BG;
    uint16_t cText        = COLOR_TEXT;
    uint16_t cHeaderBg    = COLOR_HEADER_BG;
    uint16_t cHeaderTxt   = COLOR_HEADER_TXT;
    uint16_t cAccent      = COLOR_ACCENT;
    uint16_t cWarning     = COLOR_WARNING;
    uint16_t cError       = COLOR_ERROR;
    uint16_t cMenuSel     = COLOR_MENU_SEL;
    uint16_t cMenuTxt     = COLOR_MENU_TXT;
    uint16_t cDivider     = COLOR_DIVIDER;
    uint16_t cGunOn       = COLOR_GUN_ON;
    uint16_t cGunOff      = COLOR_GUN_OFF;

private:
    TFT_eSPI tft;
    const char* stateStr(MachineState s);
    uint16_t stateColor(MachineState s);
    const char* modeStr(MachineMode m);
    void fmtTime(unsigned long sec, char* buf, size_t len);
    void fmtDist(float meters, char* buf, size_t len);
    void fmtMTH(uint32_t sec, char* buf, size_t len);
    void drawStatRow(const char* label, const char* value, int y);
    void drawPatternVisualization(int vizX, int vizY, int vizW, int vizH,
                                  const GunPatternCfg gunsCfg[6],
                                  bool reversed, bool gapStart,
                                  float positionM = -1.0f);
};

extern DisplayManager display;
