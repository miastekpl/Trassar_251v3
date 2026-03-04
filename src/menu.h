#pragma once
#include "config.h"
#include "button_handler.h"

class MenuSystem {
public:
    void begin();
    void handleEvent(ButtonEvent event);
    void update();   // renderowanie + ciagle sterowanie (np. czyszczenie dysz)
    void goToScreen(ScreenID screen);

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
    void handleCounterReset(ButtonEvent e);
    void handleSummary(ButtonEvent e);
    void handleLifetimeStats(ButtonEvent e);
    void handleCustomPattern(ButtonEvent e);
    void handleStatsExport(ButtonEvent e);

    static const int SERVICE_MENU_ITEMS = 9;  // 6 + lifetime + custom_pat + export

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

    // Podsumowanie etapu (SUMMARY)
    float summaryDist = 0;
    float summaryArea = 0;
    unsigned long summaryTime = 0;
    float summaryAvgSpeed = 0;
    bool summaryHasGps = false;
    float summaryLat = 0;
    float summaryLon = 0;
    char summaryPatCode[16] = {};

    // Edycja wzorca wlasnego
    int custCursor = 0;        // 0=pistolet, 1=tryb, 2=linia, 3=przerwa, 4=zapisz
    int custGunIdx = 0;        // Aktualnie edytowany pistolet (0..5)
    CustomPatternCfg custCfg;  // Edytowany wzorzec

    // Eksport statystyk
    bool exportDone = false;
    bool exportSuccess = false;
};
extern MenuSystem menu;
