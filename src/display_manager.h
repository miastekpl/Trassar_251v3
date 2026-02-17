#pragma once
// ============================================================
// TrassarV3 - Modul wyswietlacza ILI9341  320x240 landscape
// Komputer pokladowy malowarki pasow drogowych  v2.8.0
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
                            float sessionDistM = 0);

    // Menu serwisowe
    void drawServiceMenu(int selectedIndex);
    void drawCalibrationScreen(bool active, float pulses, float ppm, bool calibrated);
    void drawDistanceMeter(float distanceM, bool measuring);
    void drawReportsScreen(bool sdReady, int fileCount, const char* lastReport);
    void drawNozzleClean(const char* patCode, const char* patName,
                         const GunPatternCfg guns[6], const bool gunStates[6]);

    // ---- Elementy pomocnicze ----
    void drawHeader(const char* title);
    void drawGunRects(int y, const GunPatternCfg gunsCfg[6],
                      const bool gunStates[6], bool paused);
    void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color);

    TFT_eSPI& getTFT() { return tft; }

private:
    TFT_eSPI tft;
    const char* stateStr(MachineState s);
    uint16_t stateColor(MachineState s);
    void fmtTime(unsigned long sec, char* buf, size_t len);
    void drawPatternVisualization(int vizX, int vizY, int vizW, int vizH,
                                  const GunPatternCfg gunsCfg[6],
                                  bool reversed, bool gapStart);
};

extern DisplayManager display;
