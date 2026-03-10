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
#include "joystick.h"
#include "report_logger.h"
#include "buzzer.h"
#include "gps_track.h"

MenuSystem menu;

// ============ Inicjalizacja ============

void MenuSystem::begin() {
    STATE_LOCK();
    g_state.currentScreen = SCREEN_HOME;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
    STATE_UNLOCK();
}

// ============ Przejście między ekranami ============

void MenuSystem::goToScreen(ScreenID screen) {
    ScreenID prevScreen;
    STATE_LOCK();
    prevScreen = g_state.currentScreen;
    STATE_UNLOCK();

    // Upewnij sie ze pistolety sa wylaczone przy wyjsciu z czyszczenia
    if (prevScreen == SCREEN_NOZZLE_CLEAN) {
        guns.allOff();
    }

    STATE_LOCK();
    g_state.currentScreen = screen;
    g_state.menuIndex = 0;
    g_state.displayNeedsUpdate = true;
    g_state.forceFullRedraw = true;
    STATE_UNLOCK();

    lastScreenChangeMs = millis();
    // Blokuj osie joysticka dopoki nie wroci do centrum —
    // zapobiega szumowi ADC2 (WiFi) generujacemu falszywe zdarzenia na nowym ekranie
    joystick.requireCenter();
}

// ============ Dyspozycja zdarzeń ============

void MenuSystem::handleEvent(ButtonEvent event) {
    if (event == EVT_NONE) return;

    STATE_LOCK();
    ScreenID screen = g_state.currentScreen;
    STATE_UNLOCK();

    switch (screen) {
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
        case SCREEN_FACTORY_RESET:  handleFactoryReset(event);    break;
        case SCREEN_POST:           handlePost(event);            break;
    }
}

// ============ Renderowanie + logika ciagla ============

void MenuSystem::update() {
    // Odczytaj stan ekranu pod lockiem (Core 0 moze czytac rownoczesnie)
    STATE_LOCK();
    ScreenID curScreen = g_state.currentScreen;
    STATE_UNLOCK();

    // --- Logika ciagla: pomiar dystansu ---
    if (curScreen == SCREEN_DISTANCE_METER && distMeasuring) {
        float current = encoderDist.getDistanceMeters();
        float delta = current - distMeterLast;
        distMeterLast = current;
        if (delta > 0) distMeterValue += delta;
    }

    // --- Logika ciagla: czyszczenie dysz ---
    if (curScreen == SCREEN_NOZZLE_CLEAN) {
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
    STATE_LOCK();
    bool needsUpdate = g_state.displayNeedsUpdate;
    STATE_UNLOCK();
    if (!needsUpdate) return;

    // SPI wspoldzielone: TFT i SD na tej samej magistrali HSPI.
    // Probuj zablokowac SPI z krotkim timeout — jesli SD jest zajete (Core 0),
    // czekaj do TFT_SD_MUTEX_TIMEOUT_MS, potem pomin klatke (redukcja frame drops).
    if (g_sdMutex && xSemaphoreTake(g_sdMutex, pdMS_TO_TICKS(TFT_SD_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return;  // SD zajete, sprobuj w nastepnej klatce
    }
    // Deselect SD przed operacjami TFT
    digitalWrite(PIN_SD_CS, HIGH);

    STATE_LOCK();
    bool fullRedraw = g_state.forceFullRedraw;
    g_state.displayNeedsUpdate = false;
    g_state.forceFullRedraw = false;
    curScreen = g_state.currentScreen;
    STATE_UNLOCK();

    // Pelne czyszczenie tylko przy zmianie ekranu (eliminacja migania)
    if (fullRedraw) {
        display.clear();
    }

    switch (curScreen) {

        // ---- Ekran glowny ----
        case SCREEN_HOME: {
            const PatternDef& pat = patternMgr.getCurrent();
            STATE_LOCK();
            bool reversed = g_state.patternReversed;
            STATE_UNLOCK();
            display.drawHomeScreen(
                pat.code,
                pat.name,
                encoderDist.getSpeedKmh(),
                stats.getSessionArea(),
                pat.guns,
                reversed,
                pat.hasReverse
            );
            break;
        }

        // ---- Ekran malowania ----
        case SCREEN_PAINTING: {
            const PatternDef& pat = patternMgr.getCurrent();
            STATE_LOCK();
            MachineState mState = g_state.machineState;
            bool reversed = g_state.patternReversed;
            STATE_UNLOCK();
            bool gunStates[6];
            for (int i = 0; i < NUM_GUNS; i++) {
                gunStates[i] = guns.getState(i);
            }
            display.drawPaintingScreen(
                mState,
                pat.code,
                encoderDist.getSpeedKmh(),
                stats.getSessionArea(),
                pat.guns,
                gunStates,
                reversed,
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
            // Ikona GPS overflow na ekranie malowania
            if (gpsTrack.isOverflowed()) {
                display.drawGpsOverflowIcon();
            }
            break;
        }

        // ---- Menu serwisowe ----
        case SCREEN_SERVICE_MENU: {
            STATE_LOCK();
            int menuIdx = g_state.menuIndex;
            STATE_UNLOCK();
            display.drawServiceMenu(menuIdx);
            break;
        }

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

        // ---- Factory reset NVS ----
        case SCREEN_FACTORY_RESET:
            display.drawFactoryResetScreen();
            break;

        // ---- POST (diagnostyka) ----
        case SCREEN_POST:
            // POST jest obslugiwany w setup(), ten case zapobiega warningowi
            break;
    }

    // Zwolnij mutex SPI po renderowaniu TFT
    SD_UNLOCK();
}
