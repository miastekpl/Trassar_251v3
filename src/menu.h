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
    void handleSetup(ButtonEvent e);
    void handleSessionReset(ButtonEvent e);
    void goToScreen(ScreenID screen);

    static const int SERVICE_MENU_ITEMS = 5;

    // Pomiar dystansu
    bool distMeasuring = false;
    float distMeterValue = 0;
    float distMeterLast = 0;

    // Czyszczenie dysz - wybrany wzorzec
    int nozzlePatternIdx = 0;

    // Ekran przygotowania (SETUP)
    int setupCursor = 0;       // 0=tryb, 1=przelaczanie, 2=start
    int setupMode = 0;         // 0=AUTO, 1=SEMI, 2=MANUAL
    bool setupSmart = true;    // true=Smart, false=Instant
    bool setupGapStart = false;// true=Od przerwy, false=Normalny
};
extern MenuSystem menu;
