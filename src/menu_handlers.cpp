#include "sys_log.h"
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
#include "paint_consumption.h"
#include <SD.h>

// ============ SCREEN_HOME ============

void MenuSystem::handleHomeScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT:
            // Alarm braku SD przy starcie malowania (jednorazowy)
            {
                STATE_LOCK();
                bool warned = g_state.sdCardWarningShown;
                STATE_UNLOCK();
                if (!reportLogger.isReady() && !warned) {
                    STATE_LOCK();
                    g_state.sdCardWarningShown = true;
                    STATE_UNLOCK();
                    buzzer.play(BUZ_SD_WARNING);
                    eventLog.log("MENU", "Ostrzezenie: start malowania bez karty SD");
                }
            }
            paintEngine.start();
            goToScreen(SCREEN_PAINTING);
            break;

        case EVT_START_LONG: {
            // Dlugie przytrzymanie START na HOME = ekran przygotowania (SETUP)
            STATE_LOCK();
            MachineState ms = g_state.machineState;
            STATE_UNLOCK();
            if (ms == STATE_IDLE || ms == STATE_STOPPED) {
                setupCursor = 0;
                STATE_LOCK();
                setupMode = (int)g_state.machineMode;
                STATE_UNLOCK();
                setupSmart = paintEngine.isSmartSwitch();
                setupGapStart = false;
                goToScreen(SCREEN_SETUP);
            }
            break;
        }

        case EVT_GAP_START: {
            // Alarm braku SD
            STATE_LOCK();
            bool warnShown = g_state.sdCardWarningShown;
            STATE_UNLOCK();
            if (!reportLogger.isReady() && !warnShown) {
                STATE_LOCK();
                g_state.sdCardWarningShown = true;
                STATE_UNLOCK();
                buzzer.play(BUZ_SD_WARNING);
            }
            paintEngine.startFromGap();
            goToScreen(SCREEN_PAINTING);
            break;
        }

        case EVT_STOP_LONG:
        case EVT_START_STOP_COMBO:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        case EVT_SELECT_SHORT:
            // Odwracanie wzorca (tylko P-3a, P-3b)
            if (patternMgr.getCurrent().hasReverse) {
                patternMgr.toggleReverse();
                STATE_LOCK();
                g_state.displayNeedsUpdate = true;
                STATE_UNLOCK();
            }
            break;

        default:
            break;
    }
}

// ============ SCREEN_PAINTING ============

void MenuSystem::handlePaintingScreen(ButtonEvent e) {
    switch (e) {
        case EVT_START_SHORT: {
            STATE_LOCK();
            MachineMode curMode = g_state.machineMode;
            MachineState curState = g_state.machineState;
            STATE_UNLOCK();
            if (curMode == MODE_SEMI_AUTO &&
                curState == STATE_PAINTING &&
                paintEngine.isSemiLineComplete()) {
                // Semi-auto: START wyzwala kolejna linie
                paintEngine.semiNextLine();
                STATE_LOCK();
                g_state.displayNeedsUpdate = true;
                STATE_UNLOCK();
            } else if (curMode == MODE_MANUAL) {
                // Manual: ignoruj krotkie START (trzymanie = strzal w update)
                // Ale jesli na pauzie, wznow
                if (curState == STATE_PAUSED) {
                    paintEngine.resume();
                }
            } else {
                // Auto / Semi (nie czeka na linie): pauza/wznowienie
                if (curState == STATE_PAINTING) {
                    paintEngine.pause();
                } else if (curState == STATE_PAUSED) {
                    paintEngine.resume();
                }
            }
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;
        }

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

        case EVT_START_STOP_COMBO:
            // Combo START+STOP = stop + serwis
            paintEngine.stop();
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        case EVT_SELECT_SHORT:
            // Odwracanie wzorca (tylko P-3a, P-3b)
            if (patternMgr.getCurrent().hasReverse) {
                paintEngine.toggleReverse();
                STATE_LOCK();
                g_state.displayNeedsUpdate = true;
                STATE_UNLOCK();
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
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_STOP_SHORT:
            setupCursor--;
            if (setupCursor < 0) setupCursor = 2;
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_SELECT_LONG:
            switch (setupCursor) {
                case 0:  // Tryb pracy: AUTO -> SEMI -> RECZNY -> DEMO -> AUTO
                    setupMode++;
                    if (setupMode > 3) setupMode = 0;
                    break;
                case 1:  // Przelaczanie: Smart <-> Instant
                    setupSmart = !setupSmart;
                    break;
                case 2:  // Start: Normalny <-> Od przerwy
                    setupGapStart = !setupGapStart;
                    break;
            }
            buzzer.beep(1500, 60);
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_START_SHORT:
        case EVT_START_LONG: {
            // Zapisz ustawienia i rozpocznij malowanie
            MachineMode newMode = (MachineMode)setupMode;
            STATE_LOCK();
            MachineMode oldMode = g_state.machineMode;
            if (newMode != oldMode) {
                g_state.machineMode = newMode;
            }
            STATE_UNLOCK();
            if (newMode != oldMode) {
                storage.saveMode(newMode);
            }
            if (setupSmart != paintEngine.isSmartSwitch()) {
                paintEngine.setSmartSwitch(setupSmart);
                storage.saveSwitchMode(setupSmart);
            }

            const char* modeNames[] = {"AUTO", "SEMI-AUTO", "RECZNY"};
            DBG_PRINTF("[MENU] SETUP -> Tryb: %s, Smart: %s, Start: %s\n",
                          modeNames[setupMode],
                          setupSmart ? "TAK" : "NIE",
                          setupGapStart ? "OD PRZERWY" : "NORMALNY");

            // Alarm braku SD
            {
                STATE_LOCK();
                bool warnShown = g_state.sdCardWarningShown;
                STATE_UNLOCK();
                if (!reportLogger.isReady() && !warnShown) {
                    STATE_LOCK();
                    g_state.sdCardWarningShown = true;
                    STATE_UNLOCK();
                    buzzer.play(BUZ_SD_WARNING);
                }
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

        case EVT_START_STOP_COMBO:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_SERVICE_MENU ============

void MenuSystem::handleServiceMenu(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            STATE_LOCK();
            g_state.menuIndex++;
            if (g_state.menuIndex >= SERVICE_MENU_ITEMS) g_state.menuIndex = 0;
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_STOP_SHORT:
            STATE_LOCK();
            g_state.menuIndex--;
            if (g_state.menuIndex < 0) g_state.menuIndex = SERVICE_MENU_ITEMS - 1;
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_SELECT_LONG: {
            STATE_LOCK();
            int menuIdx = g_state.menuIndex;
            STATE_UNLOCK();
            switch (menuIdx) {
                case 0: goToScreen(SCREEN_CALIBRATION);    break;
                case 1: goToScreen(SCREEN_DISTANCE_METER); break;
                case 2: goToScreen(SCREEN_REPORTS);         break;
                case 3: {
                    STATE_LOCK();
                    PatternID cp = g_state.currentPattern;
                    STATE_UNLOCK();
                    nozzlePatternIdx = (int)cp;
                    if (nozzlePatternIdx >= PatternManager::PREDEFINED_PAT_COUNT)
                        nozzlePatternIdx = 0;
                    goToScreen(SCREEN_NOZZLE_CLEAN);
                    break;
                }
                case 4:
                    goToScreen(SCREEN_LIFETIME_STATS);
                    break;
                case 5: {
                    custCfg = patternMgr.loadSlot(patternMgr.getActiveSlot());
                    if (!custCfg.valid) {
                        memset(&custCfg, 0, sizeof(custCfg));
                        custCfg.structVersion = CUSTOM_PAT_STRUCT_VER;
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
                case 9: {
                    tankRefuelAmount = 50.0f;
                    tankRefuelDone = false;
                    goToScreen(SCREEN_TANKOWANIE);
                    break;
                }
                case 10:
                    goToScreen(SCREEN_FACTORY_RESET);
                    break;
            }
            break;
        }

        case EVT_START_SHORT:
        case EVT_START_LONG: {
            // Toggle trybu nocnego z poziomu menu serwisowego
            STATE_LOCK();
            g_state.nightMode = !g_state.nightMode;
            bool nm = g_state.nightMode;
            g_state.forceFullRedraw = true;
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            display.applyNightMode(nm);
            storage.saveNightMode(nm);
            buzzer.beep(1500, 60);
            DBG_PRINTF("[MENU] Tryb nocny: %s\n", nm ? "ON" : "OFF");
            break;
        }

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
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
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
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_STOP_SHORT:
            distMeasuring = false;
            distMeterValue = 0;
            distMeterLast = encoderDist.getDistanceMeters();
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
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
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_SELECT_LONG:
            nozzlePatternIdx--;
            if (nozzlePatternIdx < 0)
                nozzlePatternIdx = PatternManager::PREDEFINED_PAT_COUNT - 1;
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
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
            STATE_LOCK();
            g_state.machineState = STATE_IDLE;
            STATE_UNLOCK();
            buzzer.beep(2000, 150);
            DBG_PRINTLN("[MENU] Reset etapu - liczniki wyzerowane");
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
            STATE_LOCK();
            g_state.machineState = STATE_IDLE;
            STATE_UNLOCK();
            buzzer.beep(1500, 300);
            DBG_PRINTLN("[MENU] Reset wszystkich licznikow (kalibracja zachowana)");
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
            STATE_LOCK();
            g_state.machineState = STATE_IDLE;
            STATE_UNLOCK();
            buzzer.beep(2000, 100);
            DBG_PRINTLN("[MENU] Podsumowanie -> Nowy etap (reset sesji)");
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
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_STOP_SHORT:
            custCursor--;
            if (custCursor < 0) custCursor = 4;
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
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
                    DBG_PRINTF("[MENU] Wzorzec wlasny zapisany (slot %d)\n", slot);
                    eventLog.logf("MENU", "Wzorzec wlasny zapisany (slot %d)", slot);
                    goToScreen(SCREEN_SERVICE_MENU);
                    return;
                }
            }
            buzzer.beep(1500, 60);
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
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
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_TANKOWANIE ============

void MenuSystem::handleTankowanie(ButtonEvent e) {
    switch (e) {
        case EVT_SELECT_SHORT:
            // Zwieksz ilosc farby (+10 L)
            tankRefuelAmount += 10.0f;
            if (tankRefuelAmount > paintConsumption.getTankCapacity())
                tankRefuelAmount = 10.0f;
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_STOP_SHORT:
            // Zmniejsz ilosc farby (-10 L)
            tankRefuelAmount -= 10.0f;
            if (tankRefuelAmount < 10.0f)
                tankRefuelAmount = paintConsumption.getTankCapacity();
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_SELECT_LONG:
            // Krok dokladny (+5 L)
            tankRefuelAmount += 5.0f;
            if (tankRefuelAmount > paintConsumption.getTankCapacity())
                tankRefuelAmount = 5.0f;
            buzzer.beep(1500, 40);
            STATE_LOCK();
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;

        case EVT_START_SHORT:
        case EVT_START_LONG: {
            if (tankRefuelDone) break;
            // Potwierdz tankowanie
            paintConsumption.refuel(tankRefuelAmount);
            tankRefuelDone = true;
            buzzer.beep(2000, 150);
            DBG_PRINTF("[MENU] Tankowanie: +%.0f L, poziom: %.1f L\n",
                          tankRefuelAmount, paintConsumption.getCurrentLevel());
            eventLog.logf("MENU", "Tankowanie: +%.0f L, poziom: %.1f L",
                          tankRefuelAmount, paintConsumption.getCurrentLevel());
            STATE_LOCK();
            g_state.forceFullRedraw = true;
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
            break;
        }

        case EVT_STOP_LONG:
            goToScreen(SCREEN_SERVICE_MENU);
            break;

        default:
            break;
    }
}

// ============ SCREEN_POST ============

void MenuSystem::handlePost(ButtonEvent e) {
    // POST jest obslugiwany w setup(), ale gdyby uzytkownik tu trafil:
    if (e == EVT_START_SHORT || e == EVT_START_LONG || e == EVT_STOP_LONG) {
        goToScreen(SCREEN_HOME);
    }
}

// ============ SCREEN_FACTORY_RESET ============

void MenuSystem::handleFactoryReset(ButtonEvent e) {
    switch (e) {
        case EVT_START_LONG: {
            // Dlugie przytrzymanie START (3s) = potwierdzenie factory reset
            storage.factoryReset();
            buzzer.beep(500, 1000);  // Dlugi niski sygnal
            eventLog.log("MENU", "FACTORY RESET wykonany");
            DBG_PRINTLN("[MENU] FACTORY RESET — restart za 2s...");
            delay(2000);
            ESP.restart();
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
                STATE_LOCK();
                g_state.displayNeedsUpdate = true;
                STATE_UNLOCK();
                break;
            }

            if (!SD_LOCK()) {
                exportDone = true;
                exportSuccess = false;
                buzzer.play(BUZ_ERROR);
                STATE_LOCK();
                g_state.displayNeedsUpdate = true;
                STATE_UNLOCK();
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
                SD_UNLOCK();
                exportDone = true;
                exportSuccess = true;
                buzzer.beep(2000, 150);
                eventLog.log("MENU", "Eksport statystyk na SD: /stats/lifetime_stats.csv");
            } else {
                SD_UNLOCK();
                exportDone = true;
                exportSuccess = false;
                buzzer.play(BUZ_ERROR);
            }
            STATE_LOCK();
            g_state.forceFullRedraw = true;
            g_state.displayNeedsUpdate = true;
            STATE_UNLOCK();
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
