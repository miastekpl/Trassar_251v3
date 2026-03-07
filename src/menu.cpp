// ============================================================
// TrassarV3 - System menu - rdzen
// Inicjalizacja, dyspozycja zdarzen, renderowanie
// Handlery per ekran: menu_handlers.cpp
// ============================================================

#include "menu.h"
#include "display_manager.h"
#include "patterns.h"
#include "encoder_distance.h"
#include "painting_engine.h"
#include "statistics.h"
#include "guns.h"
#include "button_handler.h"
#include "report_logger.h"
#include "buzzer.h"

MenuSystem menu;

// ============ Inicjalizacja ============

void MenuSystem::begin() {
    g_state.currentScreen = SCREEN_HOME;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
}

// ============ Przejście między ekranami ============

void MenuSystem::goToScreen(ScreenID screen) {
    // Upewnij sie ze pistolety sa wylaczone przy wyjsciu z czyszczenia
    if (g_state.currentScreen == SCREEN_NOZZLE_CLEAN) {
        guns.allOff();
    }
    g_state.currentScreen = screen;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
}

// ============ Dyspozycja zdarzeń ============

void MenuSystem::handleEvent(ButtonEvent event) {
    if (event == EVT_NONE) return;

    switch (g_state.currentScreen) {
        case SCREEN_HOME:           handleHomeScreen(event);      break;
        case SCREEN_PAINTING:       handlePaintingScreen(event);  break;
        case SCREEN_SERVICE_MENU:   handleServiceMenu(event);     break;
        case SCREEN_CALIBRATION:    handleCalibration(event);     break;
        case SCREEN_DISTANCE_METER: handleDistanceMeter(event);   break;
        case SCREEN_REPORTS:        handleReports(event);         break;
        case SCREEN_NOZZLE_CLEAN:   handleNozzleClean(event);     break;
        case SCREEN_SETUP:          handleSetup(event);           break;
        case SCREEN_SESSION_RESET:  handleSessionReset(event);    break;
        case SCREEN_COUNTER_RESET:  handleCounterReset(event);    break;
        case SCREEN_SUMMARY:        handleSummary(event);         break;
        case SCREEN_LIFETIME_STATS: handleLifetimeStats(event);   break;
        case SCREEN_CUSTOM_PATTERN: handleCustomPattern(event);   break;
        case SCREEN_STATS_EXPORT:   handleStatsExport(event);     break;
        case SCREEN_POST:           handlePost(event);            break;
    }
}

// ============ Renderowanie + logika ciagla ============

void MenuSystem::update() {
    // --- Logika ciagla: pomiar dystansu ---
    if (g_state.currentScreen == SCREEN_DISTANCE_METER && distMeasuring) {
        float current = encoderDist.getDistanceMeters();
        float delta = current - distMeterLast;
        distMeterLast = current;
        if (delta > 0) distMeterValue += delta;
    }

    // --- Logika ciagla: czyszczenie dysz ---
    if (g_state.currentScreen == SCREEN_NOZZLE_CLEAN) {
        bool held = buttons.isStartHeld();
        const PatternDef& pat = patternMgr.getPattern((PatternID)nozzlePatternIdx);
        for (int i = 0; i < NUM_GUNS; i++) {
            bool active = (pat.guns[i].mode != GUN_OFF);
            guns.setGun((GunID)i, held && active);
        }
        // Wymusz odswiezanie zeby pokazac stan pistoletow
        g_state.displayNeedsUpdate = true;
    }

    // --- Renderowanie ---
    if (!g_state.displayNeedsUpdate) return;

    bool fullRedraw = g_state.forceFullRedraw;
    g_state.displayNeedsUpdate = false;
    g_state.forceFullRedraw = false;

    // Pelne czyszczenie tylko przy zmianie ekranu (eliminacja migania)
    if (fullRedraw) {
        display.clear();
    }

    switch (g_state.currentScreen) {

        // ---- Ekran glowny ----
        case SCREEN_HOME: {
            const PatternDef& pat = patternMgr.getCurrent();
            display.drawHomeScreen(
                pat.code,
                pat.name,
                encoderDist.getSpeedKmh(),
                stats.getSessionArea(),
                pat.guns,
                g_state.patternReversed,
                pat.hasReverse
            );
            break;
        }

        // ---- Ekran malowania ----
        case SCREEN_PAINTING: {
            const PatternDef& pat = patternMgr.getCurrent();
            bool gunStates[6];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunStates[i] = guns.getState(i);
            }
            display.drawPaintingScreen(
                g_state.machineState,
                pat.code,
                encoderDist.getSpeedKmh(),
                stats.getSessionArea(),
                pat.guns,
                gunStates,
                g_state.patternReversed,
                paintEngine.isGapStart(),
                paintEngine.isOverspeed(),
                paintEngine.isLowSpeed(),
                stats.getSessionTimeSec(),
                stats.getSessionDistance(),
                paintEngine.getPatternDistance()
            );
            // Ikona SD warning na ekranie malowania
            if (!reportLogger.isReady()) {
                display.drawSdWarningIcon();
            }
            break;
        }

        // ---- Menu serwisowe ----
        case SCREEN_SERVICE_MENU:
            display.drawServiceMenu(g_state.menuIndex);
            break;

        // ---- Kalibracja ----
        case SCREEN_CALIBRATION:
            display.drawCalibrationScreen(
                encoderDist.isCalibrating(),
                encoderDist.getCalibrationPulses(),
                encoderDist.getPulsesPerMeter(),
                encoderDist.isCalibrated()
            );
            break;

        // ---- Pomiar dystansu ----
        case SCREEN_DISTANCE_METER:
            display.drawDistanceMeter(distMeterValue, distMeasuring);
            break;

        // ---- Raporty ----
        case SCREEN_REPORTS: {
            char lastReport[128] = {};
            reportLogger.getLastReport(lastReport, sizeof(lastReport));
            display.drawReportsScreen(
                reportLogger.isReady(),
                reportLogger.getReportCount(),
                lastReport
            );
            break;
        }

        // ---- Czyszczenie dysz ----
        case SCREEN_NOZZLE_CLEAN: {
            const PatternDef& pat = patternMgr.getPattern((PatternID)nozzlePatternIdx);
            bool gunStates[6];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunStates[i] = guns.getState(i);
            }
            display.drawNozzleClean(pat.code, pat.name, pat.guns, gunStates);
            break;
        }

        // ---- Ekran przygotowania (SETUP) ----
        case SCREEN_SETUP:
            display.drawSetupScreen(setupCursor, (MachineMode)setupMode,
                                    setupSmart, setupGapStart);
            break;

        // ---- Reset etapu ----
        case SCREEN_SESSION_RESET:
            display.drawSessionResetScreen(
                stats.getSessionDistance(),
                stats.getSessionArea(),
                stats.getSessionTimeSec()
            );
            break;

        // ---- Reset wszystkich licznikow ----
        case SCREEN_COUNTER_RESET: {
            uint32_t gunShots[NUM_GUNS];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunShots[i] = stats.getGunShotCount(i);
            }
            display.drawCounterResetScreen(
                stats.getLifetimeDistance(),
                stats.getLifetimeArea(),
                stats.getLifetimePaintTimeSec(),
                gunShots
            );
            break;
        }

        // ---- Podsumowanie etapu ----
        case SCREEN_SUMMARY:
            display.drawSummaryScreen(
                summaryPatCode, summaryDist, summaryArea,
                summaryTime, summaryAvgSpeed,
                summaryHasGps, summaryLat, summaryLon
            );
            break;

        // ---- Statystyki lifetime ----
        case SCREEN_LIFETIME_STATS: {
            uint32_t gunShots[NUM_GUNS];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunShots[i] = stats.getGunShotCount(i);
            }
            display.drawLifetimeStatsScreen(
                stats.getLifetimeDistance(),
                stats.getLifetimeArea(),
                stats.getLifetimePaintTimeSec(),
                gunShots,
                stats.getMTHSeconds()
            );
            break;
        }

        // ---- Edycja wzorca wlasnego ----
        case SCREEN_CUSTOM_PATTERN:
            display.drawCustomPatternScreen(custCursor, custGunIdx, custCfg);
            break;

        // ---- Eksport statystyk ----
        case SCREEN_STATS_EXPORT:
            display.drawStatsExportScreen(!exportDone, exportSuccess);
            break;

        // ---- POST (diagnostyka) ----
        case SCREEN_POST:
            // POST jest obslugiwany w setup(), ten case zapobiega warningowi
            break;
    }
}
