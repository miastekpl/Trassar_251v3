#pragma once
#include "config.h"
#include "button_handler.h"

class MenuSystem {
public:
    void begin();
    void handleEvent(ButtonEvent event);
    void update();   // renderowanie + ciagle sterowanie (np. czyszczenie dysz)

private:
    void handleHomeScreen(ButtonEvent e);
    void handlePaintingScreen(ButtonEvent e);
    void handleServiceMenu(ButtonEvent e);
    void handleCalibration(ButtonEvent e);
    void handleDistanceMeter(ButtonEvent e);
    void handleReports(ButtonEvent e);
    void handleNozzleClean(ButtonEvent e);
    void goToScreen(ScreenID screen);

    static const int SERVICE_MENU_ITEMS = 4;

    // Pomiar dystansu
    bool distMeasuring = false;
    float distMeterValue = 0;
    float distMeterLast = 0;

    // Czyszczenie dysz - wybrany wzorzec
    int nozzlePatternIdx = 0;
};
extern MenuSystem menu;
