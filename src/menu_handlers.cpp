// ============================================================
// TrassarV3 - Handlery zdarzen per ekran
// Wydzielone z menu.cpp dla czytelnosci
// ============================================================

#include "menu.h"
#include "display_manager.h"
#include "rtc_handler.h"
#include "patterns.h"
#include "encoder_distance.h"
#include "painting_engine.h"
#include "statistics.h"
#include "guns.h"
#include "button_handler.h"
#include "report_logger.h"
#include "storage.h"
#include "buzzer.h"
#include "gps_handler.h"
#include "event_log.h"
#include <SD.h>

// ============ SCREEN_HOME ============

void MenuSystem::handleHomeScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            // Alarm braku SD przy starcie malowania (jednorazowy)
            if (!reportLogger.isReady() && !g_state.sdCardWarningShown) {
                g_state.sdCardWarningShown = true;
                buzzer.play(BUZ_SD_WARNING);
                eventLog.log("MENU", "Ostrzezenie: start malowania bez karty SD");
            }
            paintEngine.start();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_START_LONG:
            // Dlugie przytrzymanie START na HOME = ekran przygotowania (SETUP)
            if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
                setupCursor = 0;
                setupMode = (int)g_state.machineMode;
                setupSmart = paintEngine.isSmartSwitch();
                setupGapStart = false;
                goToScreen(SCREEN_SETUP);
            }
            break;

        case EVT_GAP_START:
            // Alarm braku SD
            if (!reportLogger.isReady() && !g_state.sdCardWarningShown) {
                g_state.sdCardWarningShown = true;
                buzzer.play(BUZ_SD_WARNING);
            }
            paintEngine.startFromGap();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        case EVT_SELECT_SHORT:
            // Odwracanie wzorca (tylko P-3a, P-3b)
            if (patternMgr.getCurrent().hasReverse) {
                patternMgr.toggleReverse();
                g_state.displayNeedsUpdate = true;
            }
            break;

        default:
            break;
    }
}

// ============ SCREEN_PAINTING ============

void MenuSystem::handlePaintingScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (g_state.machineMode == MODE_SEMI_AUTO &&
                g_state.machineState == STATE_PAINTING &&
                paintEngine.isSemiLineComplete()) {
                // Semi-auto: START wyzwala kolejna linie
                paintEngine.semiNextLine();
                g_state.displayNeedsUpdate = true;
            } else if (g_state.machineMode == MODE_MANUAL) {
                // Manual: ignoruj krotkie START (trzymanie = strzal w update)
                // Ale jesli na pauzie, wznow
                if (g_state.machineState == STATE_PAUSED) {
                    paintEngine.resume();
                }
            } else {
                // Auto / Semi (nie czeka na linie): pauza/wznowienie
                if (g_state.machineState == STATE_PAINTING) {
                    paintEngine.pause();
                } else if (g_state.machineState == STATE_PAUSED) {
                    paintEngine.resume();
                }
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT: {
            // Zachowaj dane podsumowania PRZED zatrzymaniem
            summaryDist = stats.getSessionDistance();
            summaryArea = stats.getSessionArea();
            summaryTime = stats.getSessionTimeSec();
            if (summaryTime > 0) {
                float distKm = summaryDist / 1000.0f;
                float timeH = (float)summaryTime / 3600.0f;
                summaryAvgSpeed = (timeH > 0) ? (distKm / timeH) : 0;
            } else {
                summaryAvgSpeed = 0;
            }
            strncpy(summaryPatCode, patternMgr.getCurrent().code, sizeof(summaryPatCode) - 1);
            summaryHasGps = gpsHandler.hasFix();
            summaryLat = summaryHasGps ? gpsHandler.getLat() : 0;
            summaryLon = summaryHasGps ? gpsHandler.getLng() : 0;

            paintEngine.stop();
            goToScreen(SCREEN_SUMMARY);
            break;
        }

        case EVT_SELECT_SHORT:
            // Odwracanie wzorca (tylko P-3a, P-3b)
            if (patternMgr.getCurrent().hasReverse) {
                paintEngine.toggleReverse();
                g_state.displayNeedsUpdate = true;
            }
            break;

        default:
            break;
    }
}

// ============ SCREEN_SETUP (PRZYGOTOWANIE) ============

void MenuSystem::handleSetup(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            setupCursor++;
            if (setupCursor > 2) setupCursor = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            setupCursor--;
            if (setupCursor < 0) setupCursor = 2;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            switch (setupCursor) {
                case 0:  // Tryb pracy: AUTO -> SEMI -> RECZNY -> AUTO
                    setupMode++;
                    if (setupMode > 2) setupMode = 0;
                    break;
                case 1:  // Przelaczanie: Smart <-> Instant
                    setupSmart = !setupSmart;
                    break;
                case 2:  // Start: Normalny <-> Od przerwy
                    setupGapStart = !setupGapStart;
                    break;
            }
            buzzer.beep(1500, 60);
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_START_SHORT:
        case EVT_START_LONG: {
            // Zapisz ustawienia i rozpocznij malowanie
            MachineMode newMode = (MachineMode)setupMode;
            if (newMode != g_state.machineMode) {
                g_state.machineMode = newMode;
                storage.saveMode(newMode);
            }
            if (setupSmart != paintEngine.isSmartSwitch()) {
                paintEngine.setSmartSwitch(setupSmart);
                storage.saveSwitchMode(setupSmart);
            }

            const char* modeNames[] = {"AUTO", "SEMI-AUTO", "RECZNY"};
            Serial.printf("[MENU] SETUP -> Tryb: %s, Smart: %s, Start: %s\n",
                          modeNames[setupMode],
                          setupSmart ? "TAK" : "NIE",
                          setupGapStart ? "OD PRZERWY" : "NORMALNY");

            // Alarm braku SD
            if (!reportLogger.isReady() && !g_state.sdCardWarningShown) {
                g_state.sdCardWarningShown = true;
                buzzer.play(BUZ_SD_WARNING);
            }

            // Uruchom malowanie
            if (setupGapStart) {
                paintEngine.startFromGap();
            } else {
                paintEngine.start();
            }
            buzzer.beep(2000, 150);
            goToScreen(SCREEN_PAINTING);
            break;
        }

        case EVT_STOP_LONG:
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SERVICE_MENU ============

void MenuSystem::handleServiceMenu(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            g_state.menuIndex++;
            if (g_state.menuIndex >= SERVICE_MENU_ITEMS) g_state.menuIndex = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            g_state.menuIndex--;
            if (g_state.menuIndex < 0) g_state.menuIndex = SERVICE_MENU_ITEMS - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            switch (g_state.menuIndex) {
                case 0: goToScreen(SCREEN_CALIBRATION);    break;
                case 1: goToScreen(SCREEN_DISTANCE_METER); break;
                case 2: goToScreen(SCREEN_REPORTS);         break;
                case 3:
                    nozzlePatternIdx = (int)g_state.currentPattern;
                    if (nozzlePatternIdx >= PatternManager::PREDEFINED_PAT_COUNT)
                        nozzlePatternIdx = 0;
                    goToScreen(SCREEN_NOZZLE_CLEAN);
                    break;
                case 4:
                    goToScreen(SCREEN_LIFETIME_STATS);
                    break;
                case 5: {
                    custCfg = patternMgr.loadSlot(patternMgr.getActiveSlot());
                    if (!custCfg.valid) {
                        memset(&custCfg, 0, sizeof(custCfg));
                        custCfg.valid = true;
                        for (int i = 0; i < NUM_GUNS; i++) {
                            custCfg.lineLen[i] = 2.0f;
                            custCfg.gapLen[i] = 2.0f;
                        }
                    }
                    custCursor = 0;
                    custGunIdx = 0;
                    goToScreen(SCREEN_CUSTOM_PATTERN);
                    break;
                }
                case 6: {
                    exportDone = false;
                    exportSuccess = false;
                    goToScreen(SCREEN_STATS_EXPORT);
                    break;
                }
                case 7:
                    goToScreen(SCREEN_SESSION_RESET);
                    break;
                case 8:
                    goToScreen(SCREEN_COUNTER_RESET);
                    break;
            }
            break;

        case EVT_START_SHORT:
        case EVT_START_LONG:
            // Toggle trybu nocnego z poziomu menu serwisowego
            g_state.nightMode = !g_state.nightMode;
            display.applyNightMode(g_state.nightMode);
            storage.saveNightMode(g_state.nightMode);
            buzzer.beep(1500, 60);
            g_state.forceFullRedraw = true;
            g_state.displayNeedsUpdate = true;
            Serial.printf("[MENU] Tryb nocny: %s\n", g_state.nightMode ? "ON" : "OFF");
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ SCREEN_CALIBRATION ============

void MenuSystem::handleCalibration(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (!encoderDist.isCalibrating()) {
                encoderDist.startCalibration();
            } else {
                encoderDist.finishCalibration();
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            encoderDist.cancelCalibration();
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_DISTANCE_METER ============

void MenuSystem::handleDistanceMeter(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            if (!distMeasuring) {
                distMeasuring = true;
                distMeterLast = encoderDist.getDistanceMeters();
            } else {
                distMeasuring = false;
            }
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            distMeasuring = false;
            distMeterValue = 0;
            distMeterLast = encoderDist.getDistanceMeters();
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            distMeasuring = false;
            distMeterValue = 0;
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_REPORTS ============

void MenuSystem::handleReports(ButtonEvent e) {
    if (e == EVT_STOP_LONG) {
        goToScreen(SCREEN_SERVICE_MENU);
    }
}

// ============ SCREEN_NOZZLE_CLEAN ============

void MenuSystem::handleNozzleClean(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            nozzlePatternIdx++;
            if (nozzlePatternIdx >= PatternManager::PREDEFINED_PAT_COUNT)
                nozzlePatternIdx = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            nozzlePatternIdx--;
            if (nozzlePatternIdx < 0)
                nozzlePatternIdx = PatternManager::PREDEFINED_PAT_COUNT - 1;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            guns.allOff();
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SESSION_RESET ============

void MenuSystem::handleSessionReset(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
        case EVT_START_LONG: {
            stats.resetSession();
            encoderDist.resetDistance();
            g_state.machineState = STATE_IDLE;
            buzzer.beep(2000, 150);
            Serial.println("[MENU] Reset etapu - liczniki wyzerowane");
            goToScreen(SCREEN_HOME);
            break;
        }

        case EVT_STOP_SHORT:
        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_COUNTER_RESET ============

void MenuSystem::handleCounterReset(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
        case EVT_START_LONG: {
            stats.resetAll();
            encoderDist.resetDistance();
            storage.resetAllExceptCalibration();
            g_state.machineState = STATE_IDLE;
            buzzer.beep(1500, 300);
            Serial.println("[MENU] Reset wszystkich licznikow (kalibracja zachowana)");
            goToScreen(SCREEN_HOME);
            break;
        }

        case EVT_STOP_SHORT:
        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SUMMARY ============

void MenuSystem::handleSummary(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
        case EVT_START_LONG:
            paintEngine.start();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_STOP_SHORT: {
            stats.resetSession();
            encoderDist.resetDistance();
            g_state.machineState = STATE_IDLE;
            buzzer.beep(2000, 100);
            Serial.println("[MENU] Podsumowanie -> Nowy etap (reset sesji)");
            goToScreen(SCREEN_HOME);
            break;
        }

        case EVT_STOP_LONG:
            goToScreen(SCREEN_HOME);
            break;

        default:
            break;
    }
}

// ============ SCREEN_LIFETIME_STATS ============

void MenuSystem::handleLifetimeStats(ButtonEvent e) {
    if (e == EVT_STOP_LONG) {
        goToScreen(SCREEN_SERVICE_MENU);
    }
}

// ============ SCREEN_CUSTOM_PATTERN ============

void MenuSystem::handleCustomPattern(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            custCursor++;
            if (custCursor > 4) custCursor = 0;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_SHORT:
            custCursor--;
            if (custCursor < 0) custCursor = 4;
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_SELECT_LONG:
            switch (custCursor) {
                case 0:
                    custGunIdx++;
                    if (custGunIdx >= NUM_GUNS) custGunIdx = 0;
                    break;
                case 1: {
                    uint8_t m = custCfg.gunModes[custGunIdx];
                    m++;
                    if (m > GUN_DASHED) m = GUN_OFF;
                    custCfg.gunModes[custGunIdx] = m;
                    break;
                }
                case 2:
                    custCfg.lineLen[custGunIdx] += 0.5f;
                    if (custCfg.lineLen[custGunIdx] > 20.0f) custCfg.lineLen[custGunIdx] = 0.5f;
                    break;
                case 3:
                    custCfg.gapLen[custGunIdx] += 0.5f;
                    if (custCfg.gapLen[custGunIdx] > 20.0f) custCfg.gapLen[custGunIdx] = 0.5f;
                    break;
                case 4: {
                    custCfg.valid = true;
                    int slot = patternMgr.getActiveSlot();
                    patternMgr.saveSlot(slot, custCfg);
                    patternMgr.activateSlot(slot);
                    buzzer.beep(2000, 150);
                    Serial.printf("[MENU] Wzorzec wlasny zapisany (slot %d)\n", slot);
                    eventLog.logf("MENU", "Wzorzec wlasny zapisany (slot %d)", slot);
                    goToScreen(SCREEN_SERVICE_MENU);
                    return;
                }
            }
            buzzer.beep(1500, 60);
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_START_SHORT:
            switch (custCursor) {
                case 0:
                    custGunIdx--;
                    if (custGunIdx < 0) custGunIdx = NUM_GUNS - 1;
                    break;
                case 1: {
                    int m = (int)custCfg.gunModes[custGunIdx] - 1;
                    if (m < 0) m = GUN_DASHED;
                    custCfg.gunModes[custGunIdx] = (uint8_t)m;
                    break;
                }
                case 2:
                    custCfg.lineLen[custGunIdx] -= 0.5f;
                    if (custCfg.lineLen[custGunIdx] < 0.5f) custCfg.lineLen[custGunIdx] = 20.0f;
                    break;
                case 3:
                    custCfg.gapLen[custGunIdx] -= 0.5f;
                    if (custCfg.gapLen[custGunIdx] < 0.5f) custCfg.gapLen[custGunIdx] = 20.0f;
                    break;
            }
            buzzer.beep(1500, 40);
            g_state.displayNeedsUpdate = true;
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_STATS_EXPORT ============

void MenuSystem::handleStatsExport(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
        case EVT_START_LONG: {
            if (exportDone) break;

            if (!reportLogger.isReady()) {
                exportDone = true;
                exportSuccess = false;
                buzzer.play(BUZ_ERROR);
                g_state.displayNeedsUpdate = true;
                break;
            }

            if (!SD.exists("/stats")) {
                SD.mkdir("/stats");
            }
            File f = SD.open("/stats/lifetime_stats.csv", FILE_WRITE);
            if (f) {
                f.println("parametr,wartosc");
                char buf[64];
                snprintf(buf, sizeof(buf), "dystans_m,%.1f", stats.getLifetimeDistance());
                f.println(buf);
                snprintf(buf, sizeof(buf), "powierzchnia_m2,%.2f", stats.getLifetimeArea());
                f.println(buf);
                snprintf(buf, sizeof(buf), "czas_malowania_s,%u", stats.getLifetimePaintTimeSec());
                f.println(buf);
                snprintf(buf, sizeof(buf), "motogodziny_s,%u", stats.getMTHSeconds());
                f.println(buf);
                for (int i = 0; i < NUM_GUNS; i++) {
                    snprintf(buf, sizeof(buf), "strzaly_P%d,%u", i + 1, stats.getGunShotCount(i));
                    f.println(buf);
                }
                snprintf(buf, sizeof(buf), "data_eksportu,%s", rtcModule.getDateTimeStr());
                f.println(buf);
                f.close();
                exportDone = true;
                exportSuccess = true;
                buzzer.beep(2000, 150);
                eventLog.log("MENU", "Eksport statystyk na SD: /stats/lifetime_stats.csv");
            } else {
                exportDone = true;
                exportSuccess = false;
                buzzer.play(BUZ_ERROR);
            }
            g_state.forceFullRedraw = true;
            g_state.displayNeedsUpdate = true;
            break;
        }

        case EVT_STOP_SHORT:
        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}
