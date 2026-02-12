#pragma once
// ============================================================
// TrassarV3 - System menu
// ============================================================

#include "config.h"
#include "button_handler.h"

class MenuSystem {
public:
    void begin();
    void handleEvent(ButtonEvent event);
    void update();

    ScreenID getCurrentScreen() const { return g_state.currentScreen; }

private:
    void handleHomeScreen(ButtonEvent event);
    void handleMainMenu(ButtonEvent event);
    void handlePaintSettings(ButtonEvent event);
    void handleTimeSettings(ButtonEvent event);
    void handleWifiInfo(ButtonEvent event);
    void handleSystemInfo(ButtonEvent event);
    void handlePaintingScreen(ButtonEvent event);

    void goToScreen(ScreenID screen);
    void startPainting();
    void pausePainting();
    void resumePainting();
    void stopPainting();

    // Menu główne
    static const int MAIN_MENU_ITEMS = 4;
    const char* mainMenuLabels[MAIN_MENU_ITEMS] = {
        "Ustawienia malowania",
        "Czas i data",
        "Informacje WiFi",
        "Informacje systemowe"
    };

    // Ustawienia malowania - pola
    int paintSettingsField = 0;  // 0=speed, 1=passes

    // Ustawienia czasu - pola
    int timeSettingsField = 0;   // 0=hour, 1=min, 2=sec, 3=day, 4=month, 5=year
    int timeEditValues[6];       // Wartości edytowane
    bool timeEditMode = false;
};

extern MenuSystem menu;
