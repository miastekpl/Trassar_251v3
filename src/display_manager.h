#pragma once
// ============================================================
// TrassarV3 - Moduł wyświetlacza ILI9341
// ============================================================

#include <TFT_eSPI.h>
#include "config.h"

class DisplayManager {
public:
    void begin();
    void setBacklight(uint8_t brightness);
    void clear();

    // Ekrany
    void drawHomeScreen(const char* timeStr, const char* dateStr);
    void drawMainMenu(int selectedIndex);
    void drawPaintSettings(int selectedField, int speed, int passes);
    void drawTimeSettings(const char* timeStr, const char* dateStr, int selectedField);
    void drawWifiInfo(const char* ssid, const char* ip, int clients);
    void drawSystemInfo(const char* fwVer, const char* fwDate, uint32_t freeHeap, uint32_t uptime);
    void drawPaintingScreen(MachineState state, int speed, int currentPass, int totalPasses, unsigned long elapsed);

    // Elementy pomocnicze
    void drawHeader(const char* title);
    void drawStatusBar(MachineState state, const char* timeStr);
    void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color);
    void drawMenuItem(int y, const char* label, bool selected);

    TFT_eSPI& getTFT() { return tft; }

private:
    TFT_eSPI tft;
    const char* stateToString(MachineState state);
    uint16_t stateToColor(MachineState state);
    void formatElapsed(unsigned long ms, char* buf, size_t len);
};

extern DisplayManager display;
