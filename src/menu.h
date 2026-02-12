#pragma once
#include "config.h"
#include "button_handler.h"

class MenuSystem {
public:
    void begin();
    void handleEvent(ButtonEvent event);
    void update();
private:
    void handleHomeScreen(ButtonEvent e);
    void handlePaintingScreen(ButtonEvent e);
    void handleMainMenu(ButtonEvent e);
    void handlePatternSelect(ButtonEvent e);
    void handleCalibration(ButtonEvent e);
    void handleStatistics(ButtonEvent e);
    void handleWifiInfo(ButtonEvent e);
    void handleSystemInfo(ButtonEvent e);
    void handleTimeSettings(ButtonEvent e);
    void goToScreen(ScreenID screen);

    static const int MAIN_MENU_ITEMS = 6;
    int timeSettingsField = 0;
};
extern MenuSystem menu;
