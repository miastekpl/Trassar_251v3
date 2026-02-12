#pragma once
// ============================================================
// TrassarV3 - Modul wyswietlacza ILI9341  240x320
// Komputer pokladowy malowarki pasow drogowych  v2.0.0
// ============================================================

#include <TFT_eSPI.h>
#include "config.h"

class DisplayManager {
public:
    void begin();
    void setBacklight(uint8_t brightness);
    void clear();

    // ---- Ekrany ----
    void drawHomeScreen(const char* timeStr, const char* dateStr, const char* patCode, const char* patName,
                        float speedKmh, float distanceM, bool calibrated, bool reversed);
    void drawPaintingScreen(MachineState state, const char* patCode, float speedKmh, float distM,
                            float areaM2, unsigned long elapsedSec, const bool gunStates[6], bool reversed);
    void drawMainMenu(int selectedIndex);
    void drawPatternSelect(int selectedIndex);
    void drawCalibrationScreen(bool active, float pulses, float ppm, bool calibrated);
    void drawStatisticsScreen(float sessDist, float sessArea, unsigned long sessTime,
                              float ltDist, float ltArea, uint32_t ltTime);
    void drawWifiInfo(const char* ssid, const char* ip, int clients);
    void drawSystemInfo(const char* fwVer, uint32_t freeHeap, uint32_t uptime);
    void drawTimeSettings(const char* timeStr, const char* dateStr, int selectedField);

    // ---- Elementy pomocnicze ----
    void drawHeader(const char* title);
    void drawStatusBar(MachineState state, const char* timeStr);
    void drawGunIndicators(int y, const bool gunStates[6]);
    void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color);

    TFT_eSPI& getTFT() { return tft; }

private:
    TFT_eSPI tft;
    const char* stateStr(MachineState s);
    uint16_t stateColor(MachineState s);
    void fmtTime(unsigned long sec, char* buf, size_t len);
};

extern DisplayManager display;
