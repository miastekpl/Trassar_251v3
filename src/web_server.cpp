#include "sys_log.h"
// ============================================================
// TrassarV3 - Implementacja serwera WWW (WiFi AP) + WebSocket
// v2.20.0 - WebSocket push, GeoJSON, GPS tracks API, WDT Core 0
// ============================================================

#include "web_server.h"
#include "web_html.h"
#include <cmath>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <LittleFS.h>
#include "painting_engine.h"
#include "menu.h"
#include "encoder_distance.h"
#include "statistics.h"
#include "guns.h"
#include "patterns.h"
#include "rtc_handler.h"
#include "storage.h"
#include "report_logger.h"
#include "gps_handler.h"
#include "gps_track.h"
#include "paint_consumption.h"
#include "event_log.h"
#include <SD.h>

TrassarWebServer webServer;

// ============================================================
// Inicjalizacja WiFi AP i serwera HTTP
// ============================================================
void TrassarWebServer::begin() {
    // Mount LittleFS (web UI assets)
    // Fix #21: Przy uszkodzonym FS formatuj i remontuj — zapobiega blokowaniu
    // na skorumpowanych operacjach plikowych w handleRoot/handleStaticFile
    if (LittleFS.begin(false)) {
        littleFsReady = true;
        DBG_PRINTLN("[LittleFS] Zamontowano pomyslnie");
        // Sprawdz czy index.html istnieje
        if (LittleFS.exists("/index.html")) {
            File f = LittleFS.open("/index.html", "r");
            if (f) {
                DBG_PRINTF("[LittleFS] index.html: %u bajtow\n", (unsigned)f.size());
                f.close();
            }
        } else {
            DBG_PRINTLN("[LittleFS] UWAGA: brak /index.html — fallback PROGMEM");
            littleFsReady = false;
        }
    } else {
        DBG_PRINTLN("[LittleFS] Blad montowania — proba formatowania...");
        if (LittleFS.begin(true)) {  // true = formatuj przy bledzie
            DBG_PRINTLN("[LittleFS] Sformatowano i zamontowano (pusty FS)");
            // Pusty FS — nie ma index.html, uzywamy PROGMEM
        } else {
            DBG_PRINTLN("[LittleFS] Formatowanie nieudane — FS wylaczony");
        }
        littleFsReady = false;
    }

    generatePassword();
    DBG_PRINTF("[WiFi] SSID: %s  Haslo: %s\n", WIFI_AP_SSID, wifiPassword);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, wifiPassword, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CON);

    // Fix #16: Monitoruj polaczenia/rozlaczenia stacji WiFi
    // Przy rozlaczeniu klienta WiFi proaktywnie zamknij WS — zapobiega
    // blokowaniu broadcastTXT() na martwych TCP socketach (crash Core 0)
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
            wifiStationConnected = (WiFi.softAPgetStationNum() > 0);
            DBG_PRINTF("[WiFi] Stacja rozlaczona (pozostalo: %d)\n", WiFi.softAPgetStationNum());
            // Fix #18: Nie wolac disconnect() z kontekstu WiFi tasku —
            // wsServer nie jest thread-safe. Ustawiamy flage, task Core 0
            // obsluzy rozlaczenie w swojej petli (wsServer.loop()).
            wsDisconnectRequested = true;
        } else if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
            wifiStationConnected = true;
            DBG_PRINTF("[WiFi] Nowa stacja polaczona (lacznie: %d)\n", WiFi.softAPgetStationNum());
        }
    });

    delay(100);
    DBG_PRINT("[WiFi] AP uruchomiony. IP: ");
    DBG_PRINTLN(WiFi.softAPIP());

    // Fix #23: Routy rejestrujemy tu (tylko ustawia tablice callback'ow w pamieci,
    // nie tworzy socketow TCP — bezpieczne z dowolnego rdzenia).
    // server.begin() i wsServer.begin() przeniesione do webTaskFunc() —
    // musza byc wywolane z Core 0 (ten sam rdzen co handleClient/loop).
    // Poprzednio begin() z Core 1 powodowalo blokowanie handleClient()
    // na Core 0, co wyzwalalo falszywy alarm WDT "Core 0 nie odpowiada".
    setupRoutes();

    // Uruchom task WWW na Core 0 (Arduino loop() dziala na Core 1)
    // Fix #26: Priorytet 5 (bylo 1). Przy priorytecie 1 task byl zagladzany
    // przez WiFi/LWIP (priorytet 18-23) — core0AliveMs nie aktualizowane,
    // falszywy alarm WDT, selfRepair rozlaczal WS → przeladarka reconnect
    // → wiecej WiFi activity → vicious cycle → ESP.restart().
    xTaskCreatePinnedToCore(
        webTaskFunc,        // Funkcja tasku
        "WebServer",        // Nazwa (debug)
        20480,              // Stack size [bytes] (Fix #19: zwiekszone 16K->20K dla JSON+WS)
        this,               // Parametr -> wskaznik na obiekt
        5,                  // Priorytet (5 = ponad idle, ponizej WiFi/LWIP)
        &webTaskHandle,     // Uchwyt tasku
        0                   // Core 0
    );
    DBG_PRINTLN("[WWW] Task WWW uruchomiony na Core 0");
}

// Task FreeRTOS na Core 0 - obsluga HTTP + WebSocket + watchdog
// Fix #15: Cala petla owinięta w try/catch-like recovery — crash Core 0
// NIE resetuje calego ESP. Task restartuje sie sam, pistolety chroni
// shutdown handler + keepalive z Core 1.
void TrassarWebServer::webTaskFunc(void* param) {
    TrassarWebServer* self = static_cast<TrassarWebServer*>(param);

    // Fix #23: Inicjalizacja serwerow HTTP/WS Z WEWNATRZ Core 0.
    // Poprzednio begin() bylo w begin() na Core 1, a handleClient()/loop()
    // na Core 0 — to powodowalo blokowanie pierwszego handleClient()
    // i falszywy alarm WDT "Core 0 nie odpowiada" ~10s po starcie.
    // selfRepairServers() juz to robilO poprawnie (begin z Core 0) —
    // teraz pierwsza inicjalizacja tez jest na wlasciwym rdzeniu.
    self->core0AliveMs = millis();

    self->server.begin();
    // Fix #25: Timeout HTTP kontrolowany przez -DHTTP_MAX_DATA_WAIT=3000
    // w platformio.ini (domyslnie WebServer czeka 5000ms na dane klienta).
    DBG_PRINTLN("[WWW] Serwer HTTP uruchomiony na porcie 80 (Core 0)");

    self->wsServer.begin();
    self->wsServer.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (type == WStype_CONNECTED) {
            DBG_PRINTF("[WS] Klient #%u polaczony\n", num);
        } else if (type == WStype_DISCONNECTED) {
            DBG_PRINTF("[WS] Klient #%u rozlaczony\n", num);
        }
    });
    DBG_PRINTF("[WWW] WebSocket na porcie %d (Core 0)\n", WS_PORT);

    self->core0AliveMs = millis();

    // Core 0 NIE jest dodawany do hardware TWDT.
    // Task WebServer jest blokowany przez operacje TCP (handleClient, sendTXT,
    // wsServer.loop) oraz WiFi/LWIP stack (priorytet 18-23 vs task priorytet 5).
    // Sumaryczny czas blokady moze przekroczyc WDT_TIMEOUT_SEC (5s), co powodowalo
    // falszywe triggery TWDT i restart ESP. Monitoring Core 0 jest realizowany
    // przez software watchdog z Core 1 (checkInterval=10s, selfRepair, decay)
    // ktory lepiej toleruje przejsciowe zawieszenia TCP/WiFi.
    DBG_PRINTLN("[WDT] Core 0 WebServer: monitoring software (z Core 1), bez hardware TWDT");

    for (;;) {
        // Aktualizuj timestamp aktywnosci (monitorowane z Core 1 software watchdog)
        self->core0AliveMs = millis();

        // Diagnostyka — mierzymy czas kazdej sekcji petli
        unsigned long _secStart = millis();
        unsigned long _secEnd;

        // Samonaprawa zazadana z Core 1 — reinicjalizacja serwerow
        // z wewnatrz Core 0 (thread-safe, w przeciwienstwie do vTaskDelete)
        if (self->selfRepairRequested) {
            self->selfRepairRequested = false;
            self->selfRepairServers();
        }

        // Obsluz rozlaczenie WS PRZED handleClient/broadcast —
        // zapobiega blokowaniu na martwych TCP socketach
        if (self->wsDisconnectRequested) {
            self->wsDisconnectRequested = false;
            self->disconnectAllWsClients();
        }

        self->server.handleClient();
        _secEnd = millis();
        if (_secEnd - _secStart > 1000) {
            DBG_PRINTF("[WDT-DIAG] handleClient() zablokowany %lu ms!\n", _secEnd - _secStart);
        }
        self->core0AliveMs = millis();

        _secStart = millis();
        self->wsServer.loop();
        _secEnd = millis();
        if (_secEnd - _secStart > 1000) {
            DBG_PRINTF("[WDT-DIAG] wsServer.loop() zablokowany %lu ms!\n", _secEnd - _secStart);
        }
        self->core0AliveMs = millis();

        // Broadcast statusu do klientow WebSocket co WS_BROADCAST_MS
        unsigned long now = millis();
        if (now - self->lastWsBroadcast >= WS_BROADCAST_MS) {
            self->lastWsBroadcast = now;
            if (self->wifiStationConnected && self->wsServer.connectedClients() > 0) {
                uint32_t freeHeap = ESP.getFreeHeap();
                if (freeHeap >= LOW_HEAP_CRITICAL_BYTES) {
                    _secStart = millis();
                    String json = self->getStateJson();
                    _secEnd = millis();
                    if (_secEnd - _secStart > 500) {
                        DBG_PRINTF("[WDT-DIAG] getStateJson() trwalo %lu ms!\n", _secEnd - _secStart);
                    }
                    // Wysylaj do kazdego klienta osobno.
                    // Klienty ktore blokowaly >2s sa oznaczane jako "slow"
                    // i rozlaczane — zapobiega kaskadowemu blokowaniu broadcastu
                    static uint8_t slowClientStrikes[WEBSOCKETS_SERVER_CLIENT_MAX] = {};
                    for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
                        if (self->wsServer.clientIsConnected(i)) {
                            if (slowClientStrikes[i] >= 2) {
                                DBG_PRINTF("[WS] Klient #%u rozlaczony (slow — %u strike)\n",
                                           i, slowClientStrikes[i]);
                                self->wsServer.disconnect(i);
                                slowClientStrikes[i] = 0;
                                continue;
                            }
                            _secStart = millis();
                            self->wsServer.sendTXT(i, json);
                            _secEnd = millis();
                            unsigned long sendDuration = _secEnd - _secStart;
                            if (sendDuration > 2000) {
                                slowClientStrikes[i]++;
                                DBG_PRINTF("[WDT-DIAG] sendTXT(#%u) zablokowany %lu ms (strike %u)\n",
                                           i, sendDuration, slowClientStrikes[i]);
                            } else {
                                slowClientStrikes[i] = 0;
                            }
                            self->core0AliveMs = millis();
                        } else {
                            slowClientStrikes[i] = 0;
                        }
                    }
                }
            }
        }

        // Gun keepalive z Core 0 — ochrona przed zawieszeniem Core 1.
        // Jesli Core 1 (loop) zawiesi sie, WDT zresetuje po 3s. Ale w tym
        // czasie pistolety moglyby pozostac otwarte. Core 0 sprawdza
        // niezaleznie czy update() bylo wywolywane i awaryjnie wylacza.
        {
            // Fix #25/#30: Trylock — jesli mutex zajety, pomin keepalive w tym cyklu
            // (nastepny cykl za 2ms sprawdzi ponownie). 200→50ms — keepalive sprawdzany
            // co 2ms, nie warto czekac dlugo na mutex.
            MachineState snapState = STATE_IDLE;
            if (STATE_TRYLOCK(50)) {
                snapState = g_state.machineState;
                STATE_UNLOCK();
            }
            if (snapState == STATE_PAINTING) {
                // Fix #24: Uzyj swiezego millis() — zmienna 'now' z linii 192
                // moze byc przestarzala po broadcast/self-repair (~100ms+).
                // Jesli lastUpdate jest nowsze niz stale 'now' (bo start()
                // ustawil lastGunUpdateMs po przechwyceniu 'now'), roznica
                // unsigned wraca do ~4294967291 (0xFFFFFFFB) = falszywy alarm.
                unsigned long freshNow = millis();
                unsigned long lastUpdate = paintEngine.getLastGunUpdateMs();
                unsigned long elapsed = freshNow - lastUpdate;
                // Guard: jesli elapsed > polowa zakresu uint32 — to underflow, nie prawdziwy timeout
                if (elapsed < 0x80000000UL && elapsed > GUN_KEEPALIVE_TIMEOUT_MS) {
                    guns.allOff();
                    DBG_PRINTF("[WDT-CORE0] KEEPALIVE: awaryjne guns.allOff() (brak update %lu ms)\n",
                                  elapsed);
                    eventLog.logf("SAFETY", "Core 0 keepalive: guns OFF (brak update %lu ms)", elapsed);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));  // 2ms yield - nie blokuj innych taskow
    }
}

void TrassarWebServer::update() {
    // Puste - obsluga HTTP przeniesiona do tasku na Core 0
    // Metoda zachowana dla kompatybilnosci wstecznej
}

// Fix #16: Rozlacz wszystkie klienty WebSocket (martwe TCP sockety)
#ifndef WEBSOCKETS_SERVER_CLIENT_MAX
  #define WEBSOCKETS_SERVER_CLIENT_MAX 5
#endif
void TrassarWebServer::disconnectAllWsClients() {
    for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
        core0AliveMs = millis();
        wsServer.disconnect(i);
    }
    DBG_PRINTLN("[WS] Wszystkie klienty WS rozlaczone (cleanup)");
}

// Fix #22: Samonaprawa serwerow HTTP/WS — wykonywana Z WEWNATRZ tasku Core 0.
// Poprzedni mechanizm (vTaskDelete z Core 1 + reinit) powodowal crash
// LoadProhibited bo WebServer/WebSocketsServer nie sa thread-safe,
// a ich wewnetrzny stan TCP byl korumpowany przez operacje z innego rdzenia.
void TrassarWebServer::selfRepairServers() {
    DBG_PRINTLN("[WWW] Self-repair: reinicjalizacja serwerow z Core 0...");
    unsigned long _start = millis();

    // Rozlacz WS klienty (martwe TCP sockety)
    disconnectAllWsClients();

    // Stop + begin z tego samego Core co handleClient() — bezpieczne
    server.stop();
    wsServer.close();

    // LWIP potrzebuje czasu na cleanup TCP socketow (FIN_WAIT/TIME_WAIT).
    vTaskDelay(pdMS_TO_TICKS(500));
    core0AliveMs = millis();

    server.begin();
    wsServer.begin();
    wsServer.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (type == WStype_CONNECTED) {
            DBG_PRINTF("[WS] Klient #%u polaczony\n", num);
        } else if (type == WStype_DISCONNECTED) {
            DBG_PRINTF("[WS] Klient #%u rozlaczony\n", num);
        }
    });

    core0AliveMs = millis();
    lastRepairMs = millis();  // Fix #26: cooldown — nie sprawdzaj przez 60s po naprawie

    // Fix #28: Reset hangCount po udanej naprawie.
    // Bez tego hangCount zostawal na 3+ po cooldownie (wasAliveLastCheck=true
    // blokowal przejscie dead→alive), i jeden miss po cooldownie eskalowal
    // od razu do #4, #5, #6 → restart. Teraz: reset do 1 (pamiec o problemie,
    // ale nie natychmiastowa eskalacja).
    hangCount = 1;
    totalSelfRepairs++;
    DBG_PRINTF("[WWW] Self-repair #%u ukonczony w %lu ms (hangCount reset 1)\n",
               totalSelfRepairs, millis() - _start);
}

uint32_t TrassarWebServer::getTaskStackHWM() const {
    if (webTaskHandle) {
        return uxTaskGetStackHighWaterMark(webTaskHandle);
    }
    return 0;
}

String TrassarWebServer::getIPAddress() {
    return WiFi.softAPIP().toString();
}

int TrassarWebServer::getConnectedClients() {
    return WiFi.softAPgetStationNum();
}

// ============================================================
// Generowanie unikalnego hasla WiFi z MAC adresu ESP32
// Format: 8 znakow hex (ostatnie 4 bajty MAC) — unikalne per urzadzenie
// ============================================================
void TrassarWebServer::generatePassword() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(wifiPassword, sizeof(wifiPassword), "%02X%02X%02X%02X",
             mac[2], mac[3], mac[4], mac[5]);
}

// ============================================================
// Routing
// ============================================================
void TrassarWebServer::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/stats", HTTP_GET, [this]() { handleStats(); });
    server.on("/api/reports", HTTP_GET, [this]() { handleReports(); });
    server.on("/api/reports/download", HTTP_GET, [this]() { handleReportDownload(); });
    server.on("/api/reports/geojson", HTTP_GET, [this]() { handleGeoJson(); });
    server.on("/api/tracks", HTTP_GET, [this]() { handleTrackList(); });
    server.on("/api/tracks/download", HTTP_GET, [this]() { handleTrackDownload(); });
    server.on("/api/control", HTTP_POST, [this]() { handleControl(); });
    server.on("/api/html_reports", HTTP_GET, [this]() { handleHtmlReports(); });
    server.on("/api/html_reports/download", HTTP_GET, [this]() { handleHtmlReportDownload(); });
    // Obsluga plikow statycznych z LittleFS (CSS, JS, ikony itp.)
    server.onNotFound([this]() {
        if (littleFsReady && handleStaticFile(server.uri())) {
            return;
        }
        handleNotFound();
    });
}

// ============================================================
// GET /api/status - JSON ze stanem maszyny
// ============================================================
void TrassarWebServer::handleStatus() {
    server.send(200, "application/json", getStateJson());
}

// ============================================================
// POST /api/control - Sterowanie maszyna
// ============================================================
void TrassarWebServer::handleControl() {
    core0AliveMs = millis();

    if (!server.hasArg("action")) {
        server.send(400, "application/json", "{\"error\":\"brak parametru action\"}");
        return;
    }

    String action = server.arg("action");
    String result = "ok";

    // Atomowy snapshot stanu (wymagany do decyzji o akcji)
    // Fix #25/#30: Trylock — jesli Core 1 trzyma mutex, zwroc blad zamiast blokowac.
    // Fix #30: 1000→200ms — krotszy timeout zapobiega zagłodzeniu Core 0 WDT
    // podczas ciezkich operacji Core 1 (NVS write w stop(), display update).
    if (!STATE_TRYLOCK(200)) {
        server.send(503, "application/json", "{\"error\":\"serwer zajety — sprobuj ponownie\"}");
        return;
    }
    MachineState snapState = g_state.machineState;
    ScreenID snapScreen = g_state.currentScreen;
    STATE_UNLOCK();

    if (action == "start") {
        // Jesli ekran QR startowy jest aktywny — zamknij go zamiast startowac malowanie
        if (snapScreen == SCREEN_POST) {
            if (STATE_TRYLOCK(500)) { g_state.qrDismissed = true; STATE_UNLOCK(); }
        } else if (snapState == STATE_PAUSED) {
            paintEngine.resume();
        } else if (snapState == STATE_IDLE || snapState == STATE_STOPPED) {
            paintEngine.start();
        }
    } else if (action == "start_from_gap") {
        paintEngine.startFromGap();
    } else if (action == "pause") {
        paintEngine.pause();
    } else if (action == "stop") {
        // Fix #30: Nie wywoluj stop() na Core 0 — ciężkie I/O (NVS/SD, 800ms-2.5s)
        // blokowalo WDT. Zamiast tego requestStop() natychmiast wylacza pistolety
        // i zmienia stan, a Core 1 wykonuje zapis danych w nastepnym update().
        paintEngine.requestStop();
    } else if (action == "set_pattern") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val >= 0 && val < PAT_COUNT) {
                paintEngine.setPattern((PatternID)val);
            } else {
                result = "nieprawidlowy wzorzec";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "toggle_reverse") {
        paintEngine.toggleReverse();
    } else if (action == "cal_start") {
        encoderDist.startCalibration();
    } else if (action == "cal_finish") {
        encoderDist.finishCalibration();
    } else if (action == "set_max_speed") {
        if (server.hasArg("value")) {
            float val = server.arg("value").toFloat();
            // Fix #8: walidacja NaN/Inf + cross-check z minSpeed
            if (isnan(val) || isinf(val)) {
                result = "nieprawidlowa wartosc";
            } else if (val >= 5.0f && val <= 30.0f) {
                if (val <= paintEngine.getMinSpeed()) {
                    result = "maxSpeed musi byc > minSpeed";
                } else {
                    paintEngine.setMaxSpeed(val);
                    storage.saveMaxSpeed(val);
                }
            } else {
                result = "zakres 5-30 km/h";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "set_min_speed") {
        if (server.hasArg("value")) {
            float val = server.arg("value").toFloat();
            // Fix #8: walidacja NaN/Inf + cross-check z maxSpeed
            if (isnan(val) || isinf(val)) {
                result = "nieprawidlowa wartosc";
            } else if (val >= 0.0f && val <= 10.0f) {
                if (val >= paintEngine.getMaxSpeed()) {
                    result = "minSpeed musi byc < maxSpeed";
                } else {
                    paintEngine.setMinSpeed(val);
                    storage.saveMinSpeed(val);
                }
            } else {
                result = "zakres 0-10 km/h";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "set_mode") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val >= 0 && val <= 3) {
                // Nie zmieniaj trybu podczas malowania — niebezpieczne
                if (STATE_TRYLOCK(200)) {
                    MachineState modeState = g_state.machineState;
                    if (modeState == STATE_IDLE || modeState == STATE_STOPPED) {
                        MachineMode newMode = (MachineMode)val;
                        g_state.machineMode = newMode;
                        STATE_UNLOCK();
                        storage.saveMode(newMode);
                        DBG_PRINTF("[WWW] Tryb pracy: %d\n", val);
                    } else {
                        STATE_UNLOCK();
                        result = "nie mozna zmienic trybu podczas malowania";
                    }
                } else {
                    result = "serwer zajety — sprobuj ponownie";
                }
            } else {
                result = "nieprawidlowy tryb (0-3)";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "save_custom_pattern") {
        // Parametry: g0..g5, ln0..ln5, gp0..gp5, slot (0-2)
        CustomPatternCfg cfg = {};
        cfg.structVersion = CUSTOM_PAT_STRUCT_VER;
        cfg.valid = true;
        bool validationError = false;
        for (int i = 0; i < NUM_GUNS; i++) {
            String gKey = "g" + String(i);
            String lKey = "ln" + String(i);
            String pKey = "gp" + String(i);
            if (server.hasArg(gKey)) {
                int gm = server.arg(gKey).toInt();
                if (gm < 0 || gm > 2) gm = 0;
                cfg.gunModes[i] = (uint8_t)gm;
            }
            float ln = 4.0f, gp = 8.0f;
            if (server.hasArg(lKey)) ln = server.arg(lKey).toFloat();
            if (server.hasArg(pKey)) gp = server.arg(pKey).toFloat();
            // Walidacja: odrzuc NaN/Inf i wartosci spoza zakresu
            if (isnan(ln) || isinf(ln) || isnan(gp) || isinf(gp)) {
                validationError = true;
                break;
            }
            if (ln < 0.1f) ln = 0.1f;
            if (ln > 50.0f) ln = 50.0f;
            if (gp < 0.1f) gp = 0.1f;
            if (gp > 50.0f) gp = 50.0f;
            cfg.lineLen[i] = ln;
            cfg.gapLen[i] = gp;
        }
        if (validationError) {
            server.send(400, "application/json", "{\"error\":\"nieprawidlowe wartosci lineLen/gapLen\"}");
            return;
        }
        int slot = 0;
        if (server.hasArg("slot")) {
            slot = server.arg("slot").toInt();
            if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
        }
        patternMgr.saveSlot(slot, cfg);
        patternMgr.activateSlot(slot);
        DBG_PRINTF("[WWW] Wzorzec wlasny slot %d zapisany\n", slot);
    } else if (action == "activate_slot") {
        if (server.hasArg("value")) {
            int slot = server.arg("value").toInt();
            if (slot >= 0 && slot < NUM_CUSTOM_SLOTS && patternMgr.isSlotValid(slot)) {
                patternMgr.activateSlot(slot);
            } else {
                result = "slot pusty lub nieprawidlowy";
            }
        }
    } else if (action == "get_slot_config") {
        int slot = 0;
        if (server.hasArg("slot")) {
            slot = server.arg("slot").toInt();
            if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
        }
        CustomPatternCfg cfg = patternMgr.loadSlot(slot);
        JsonDocument slotDoc;
        JsonArray guns = slotDoc["guns"].to<JsonArray>();
        for (int i = 0; i < NUM_GUNS; i++) {
            JsonObject g = guns.add<JsonObject>();
            g["mode"] = cfg.gunModes[i];
            g["ln"]   = serialized(String(cfg.lineLen[i], 1));
            g["gp"]   = serialized(String(cfg.gapLen[i], 1));
        }
        slotDoc["valid"] = cfg.valid;
        String resp;
        serializeJson(slotDoc, resp);
        server.send(200, "application/json", resp);
        return;
    } else if (action == "semi_next_line") {
        paintEngine.semiNextLine();
    } else if (action == "send_event") {
        // Wirtualne przyciski z panelu www — kolejkowanie zdarzenia do Core 1.
        // NIE wywoluj menu.handleEvent() bezposrednio z Core 0 — race condition
        // z obsluga przyciskow/joysticka w loop() na Core 1.
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val > 0 && val <= (int)EVT_GAP_START) {
                if (STATE_TRYLOCK(500)) {
                    g_state.pendingWebEvent = (ButtonEvent)val;
                    STATE_UNLOCK();
                }
                DBG_PRINTF("[WWW] Event kolejkowany: %d\n", val);
            } else {
                result = "nieprawidlowy event";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "set_screen") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val >= 0 && val <= (int)SCREEN_STATS_EXPORT) {
                menu.goToScreen((ScreenID)val);
                DBG_PRINTF("[WWW] Ekran: %d\n", val);
            } else {
                result = "nieprawidlowy ekran";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "set_switch_mode") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            bool smart = (val == 0);  // 0=smart, 1=instant
            paintEngine.setSmartSwitch(smart);
            storage.saveSwitchMode(smart);
            DBG_PRINTF("[WWW] Tryb przelaczania: %s\n", smart ? "SMART" : "INSTANT");
        }
    } else if (action == "set_tank_capacity") {
        if (server.hasArg("value")) {
            float val = server.arg("value").toFloat();
            if (val >= 1.0f && val <= 1000.0f) {
                paintConsumption.setTankCapacity(val);
                storage.saveTankCapacity(val);
            } else {
                result = "zakres 1-1000 litrow";
            }
        }
    } else if (action == "set_paint_rate") {
        if (server.hasArg("value")) {
            float val = server.arg("value").toFloat();
            if (val >= 0.1f && val <= 5.0f) {
                paintConsumption.setConsumptionRate(val);
                storage.saveConsumptionRate(val);
            } else {
                result = "zakres 0.1-5.0 l/m2";
            }
        }
    } else if (action == "set_auto_resume") {
        if (server.hasArg("value")) {
            bool en = (server.arg("value").toInt() != 0);
            paintEngine.setAutoResumeEnabled(en);
            storage.saveAutoResume(en);
        }
    } else if (action == "refuel") {
        if (server.hasArg("value")) {
            float val = server.arg("value").toFloat();
            if (val >= 1.0f && val <= 1000.0f) {
                paintConsumption.refuel(val);
                DBG_PRINTF("[WWW] Tankowanie: +%.0f L, poziom: %.1f L\n",
                              val, paintConsumption.getCurrentLevel());
            } else {
                result = "zakres 1-1000 litrow";
            }
        }
    } else {
        result = "nieznana akcja";
    }

    if (STATE_TRYLOCK(500)) { g_state.displayNeedsUpdate = true; STATE_UNLOCK(); }
    server.send(200, "application/json", "{\"result\":\"" + result + "\"}");
}

// ============================================================
// GET /api/html_reports - Lista raportow HTML na karcie SD
// ============================================================
void TrassarWebServer::handleHtmlReports() {
    if (!SD_LOCK()) {
        server.send(503, "application/json", "{\"error\":\"SD zajeta\"}");
        return;
    }
    String json = "[";
    File dir = SD.open("/html_reports");
    if (dir && dir.isDirectory()) {
        bool first = true;
        File f = dir.openNextFile();
        while (f) {
            if (!f.isDirectory()) {
                if (!first) json += ",";
                json += "\"";
                json += f.name();
                json += "\"";
                first = false;
            }
            f = dir.openNextFile();
        }
        dir.close();
    }
    SD_UNLOCK();
    json += "]";
    server.send(200, "application/json", json);
}

// ============================================================
// GET /api/html_reports/download?file=FILENAME - Pobierz raport HTML
// ============================================================
void TrassarWebServer::handleHtmlReportDownload() {
    if (!server.hasArg("file")) {
        server.send(400, "text/plain", "Brak parametru file");
        return;
    }
    String fname = server.arg("file");
    // Zabezpieczenie: tylko litery, cyfry, kropka, podkreslenie, myslnik
    for (unsigned int i = 0; i < fname.length(); i++) {
        char c = fname.charAt(i);
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            server.send(400, "text/plain", "Nieprawidlowa nazwa pliku");
            return;
        }
    }

    String path = "/html_reports/" + fname;
    if (!SD_LOCK()) {
        server.send(503, "text/plain", "SD zajeta");
        return;
    }

    if (!SD.exists(path.c_str())) {
        SD_UNLOCK();
        server.send(404, "text/plain", "Plik nie znaleziony");
        return;
    }

    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) {
        SD_UNLOCK();
        server.send(500, "text/plain", "Blad otwarcia pliku");
        return;
    }

    server.sendHeader("Content-Disposition", "inline; filename=\"" + fname + "\"");
    server.setContentLength(f.size());
    server.send(200, "text/html", "");

    uint8_t buf[512];
    while (f.available()) {
        esp_task_wdt_reset();   // Fix #19: WDT reset w petli streamowania
        core0AliveMs = millis();
        int r = f.read(buf, sizeof(buf));
        if (r > 0) server.sendContent((const char*)buf, r);
    }
    f.close();
    SD_UNLOCK();
    DBG_PRINTF("[WWW] Pobranie raportu HTML: %s\n", fname.c_str());
}

// ============================================================
// Serwowanie plikow statycznych z LittleFS
// ============================================================
bool TrassarWebServer::handleStaticFile(const String& path) {
    String filePath = path;
    if (filePath.endsWith("/")) filePath += "index.html";

    // Content type na podstawie rozszerzenia
    const char* ct = "application/octet-stream";
    if (filePath.endsWith(".html"))      ct = "text/html";
    else if (filePath.endsWith(".css"))   ct = "text/css";
    else if (filePath.endsWith(".js"))    ct = "application/javascript";
    else if (filePath.endsWith(".json"))  ct = "application/json";
    else if (filePath.endsWith(".png"))   ct = "image/png";
    else if (filePath.endsWith(".ico"))   ct = "image/x-icon";
    else if (filePath.endsWith(".svg"))   ct = "image/svg+xml";

    if (!LittleFS.exists(filePath)) return false;

    File f = LittleFS.open(filePath, "r");
    if (!f) return false;

    server.setContentLength(f.size());
    server.send(200, ct, "");
    uint8_t buf[512];
    while (f.available()) {
        esp_task_wdt_reset();   // Fix #19: WDT reset w petli streamowania
        core0AliveMs = millis();
        int r = f.read(buf, sizeof(buf));
        if (r > 0) server.sendContent((const char*)buf, r);
    }
    f.close();
    return true;
}

// ============================================================
// 404
// ============================================================
void TrassarWebServer::handleNotFound() {
    // Fix #30: Loguj URI 404 zeby zidentyfikowac nieznane requesty (co ~60s w logach)
    DBG_PRINTF("[WWW] 404: %s %s\n",
               server.method() == HTTP_GET ? "GET" : "POST",
               server.uri().c_str());
    server.send(404, "text/plain", "404 - Nie znaleziono");
}

// ============================================================
// JSON - stan maszyny
// ============================================================
String TrassarWebServer::getStateJson() {
    JsonDocument doc;

    // --- Atomowy snapshot g_state (bezpieczny odczyt z Core 0) ---
    // Fix #25/#30: Trylock z timeoutem — jesli Core 1 trzyma mutex, nie blokuj
    // broadcastu WS na nieskonczonosc. Zwroc pusty JSON zamiast zawieszac Core 0.
    // Fix #30: 500→200ms — krotszy timeout zmniejsza ryzyko kumulacji blokad
    // (handleClient + getStateJson) ktore prowadzily do WDT timeout.
    if (!STATE_TRYLOCK(200)) {
        DBG_PRINTLN("[WDT-DIAG] getStateJson(): STATE_TRYLOCK timeout — pomijam broadcast");
        return "{}";
    }
    MachineState  snapState   = g_state.machineState;
    MachineMode   snapMode    = g_state.machineMode;
    PatternID     snapPattern = g_state.currentPattern;
    bool          snapReversed = g_state.patternReversed;
    ScreenID      snapScreen  = g_state.currentScreen;
    int           snapMenuIdx = g_state.menuIndex;
    STATE_UNLOCK();

    // Stan maszyny
    const char* stateStr;
    switch (snapState) {
        case STATE_IDLE:     stateStr = "idle";     break;
        case STATE_PAINTING: stateStr = "painting"; break;
        case STATE_PAUSED:   stateStr = "paused";   break;
        case STATE_STOPPED:  stateStr = "stopped";  break;
        default:             stateStr = "unknown";   break;
    }
    doc["state"] = stateStr;

    // Aktualny ekran TFT
    doc["screen"] = (int)snapScreen;
    doc["menuIndex"] = snapMenuIdx;

    // Tryb pracy
    const char* modeStr;
    switch (snapMode) {
        case MODE_AUTO:      modeStr = "auto";      break;
        case MODE_SEMI_AUTO: modeStr = "semi";       break;
        case MODE_MANUAL:    modeStr = "manual";     break;
        case MODE_DEMO:      modeStr = "demo";       break;
        default:             modeStr = "auto";        break;
    }
    doc["mode"] = modeStr;
    doc["semiLineComplete"] = paintEngine.isSemiLineComplete();

    // Wzorzec
    const PatternDef& pat = patternMgr.getCurrent();
    doc["pattern"] = pat.code;
    doc["patternName"] = pat.name;
    doc["patternIdx"] = (int)snapPattern;
    doc["reversed"] = snapReversed;
    doc["gapStart"] = paintEngine.isGapStart();
    doc["customValid"] = patternMgr.isCustomValid();
    doc["activeSlot"] = patternMgr.getActiveSlot();
    JsonArray slotsArr = doc["slotsValid"].to<JsonArray>();
    for (int s = 0; s < NUM_CUSTOM_SLOTS; s++) slotsArr.add(patternMgr.isSlotValid(s));

    // Wzorzec wlasny — konfiguracja pistoletow do podgladu w web UI
    // Fix #21: Cache loadSlot — NVS read co 500ms (broadcast WS) jest zbyt kosztowny.
    // Odczytujemy z NVS max co 5s, miedzy tym uzywamy zbuforowanej wartosci.
    if (patternMgr.isCustomValid()) {
        static CustomPatternCfg cachedCpCfg;
        static unsigned long lastSlotLoadMs = 0;
        static int8_t lastSlot = -1;
        int8_t curSlot = patternMgr.getActiveSlot();
        unsigned long nowSlot = millis();
        if (curSlot != lastSlot || (nowSlot - lastSlotLoadMs) >= 5000) {
            cachedCpCfg = patternMgr.loadSlot(curSlot);
            lastSlotLoadMs = nowSlot;
            lastSlot = curSlot;
        }
        JsonArray cpArr = doc["customGuns"].to<JsonArray>();
        for (int i = 0; i < NUM_GUNS; i++) {
            if (cachedCpCfg.gunModes[i] == GUN_OFF) continue;
            JsonArray g = cpArr.add<JsonArray>();
            g.add(String("P") + String(i + 1));
            g.add((int)(GUN_WIDTHS_M[i] * 100.0f));
            g.add(cachedCpCfg.gunModes[i] == GUN_CONTINUOUS ? 0 : cachedCpCfg.lineLen[i]);
            g.add(cachedCpCfg.gunModes[i] == GUN_CONTINUOUS ? 0 : cachedCpCfg.gapLen[i]);
        }
    }

    // Predkosc i dystans
    doc["speed"] = serialized(String(encoderDist.getSpeedKmh(), 1));
    doc["distance"] = serialized(String(stats.getSessionDistance(), 1));
    doc["area"] = serialized(String(stats.getSessionArea(), 2));
    doc["elapsed"] = stats.getSessionTimeSec();

    // System
    doc["firmware"] = FW_VERSION;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["minFreeHeap"] = ESP.getMinFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["clients"] = WiFi.softAPgetStationNum();
    doc["webStackHWM"] = webServer.getTaskStackHWM();
    doc["littleFs"] = webServer.isLittleFsReady();

    // Kalibracja
    doc["calibrated"] = encoderDist.isCalibrated();
    doc["ppm"] = serialized(String(encoderDist.getPulsesPerMeter(), 1));
    doc["calibrating"] = encoderDist.isCalibrating();
    doc["calPulses"] = serialized(String(encoderDist.getCalibrationPulses(), 0));

    // Alarmy predkosci
    doc["maxSpeed"] = serialized(String(paintEngine.getMaxSpeed(), 1));
    doc["minSpeed"] = serialized(String(paintEngine.getMinSpeed(), 1));
    doc["overspeed"] = paintEngine.isOverspeed();
    doc["lowSpeed"] = paintEngine.isLowSpeed();

    // Pistolety
    JsonArray gunsArr = doc["guns"].to<JsonArray>();
    for (int i = 0; i < NUM_GUNS; i++) {
        gunsArr.add(guns.getState(i));
    }

    // Auto-pauza / auto-resume
    doc["autoPaused"] = paintEngine.isAutoPaused();
    doc["autoResumeEnabled"] = paintEngine.isAutoResumeEnabled();
    doc["semiSegment"] = paintEngine.getSemiSegmentNum();

    // Przelaczanie wzorcow
    doc["smartSwitch"] = paintEngine.isSmartSwitch();
    doc["patternPending"] = paintEngine.isPatternChangePending();
    if (paintEngine.isPatternChangePending()) {
        doc["pendingPattern"] = patternMgr.getPattern(paintEngine.getPendingPattern()).code;
    }

    // GPS
    doc["gpsFix"] = gpsHandler.hasFix();
    doc["gpsLat"] = serialized(String(gpsHandler.getLat(), 6));
    doc["gpsLng"] = serialized(String(gpsHandler.getLng(), 6));
    doc["gpsSat"] = gpsHandler.getSatellites();
    doc["gpsSpeed"] = serialized(String(gpsHandler.getGpsSpeed(), 1));
    doc["gpsHdop"] = serialized(String(gpsHandler.getHdop(), 1));
    doc["gpxRec"] = gpsTrack.isRecording();
    doc["gpxPts"] = gpsTrack.getPointCount();

    // Poziom farby w zbiorniku (widoczny na zywo w panelu WWW)
    doc["paintLevelL"] = serialized(String(paintConsumption.getCurrentLevel(), 1));
    {
        float tankCap = paintConsumption.getTankCapacity();
        int paintPct = (tankCap > 0) ? (int)(paintConsumption.getCurrentLevel() * 100.0f / tankCap) : 0;
        if (paintPct > 100) paintPct = 100;
        if (paintPct < 0) paintPct = 0;
        doc["paintLevelPct"] = paintPct;
    }

    // Anomalia pistoletow (odczyt pod lockiem — modyfikowane z Core 1)
    // Fix #25: Trylock — jesli mutex zajety, pokaz brak anomalii (bezpieczne default)
    bool snapAnomalyDetected = false;
    bool snapAnomalyAlert[NUM_GUNS] = {};
    if (STATE_TRYLOCK(200)) {
        snapAnomalyDetected = gunAnomaly.detected;
        for (int i = 0; i < NUM_GUNS; i++) snapAnomalyAlert[i] = gunAnomaly.alert[i];
        STATE_UNLOCK();
    }

    doc["gunAnomalyDetected"] = snapAnomalyDetected;
    JsonArray anomArr = doc["gunAnomaly"].to<JsonArray>();
    for (int i = 0; i < NUM_GUNS; i++) {
        anomArr.add(snapAnomalyAlert[i]);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================
// HTML - zawartosc przeniesiona do web_html.h (PROGMEM)
// ============================================================
// ============================================================
// GET / - Strona HTML
// LittleFS: strumieniowe wysylanie z pliku /index.html
// Fallback: chunked transfer z PROGMEM (web_html.h)
// ============================================================
void TrassarWebServer::handleRoot() {
    bool served = false;

    if (littleFsReady) {
        // --- LittleFS: strumieniowe wysylanie z pliku ---
        File f = LittleFS.open("/index.html", "r");
        if (f) {
            server.setContentLength(CONTENT_LENGTH_UNKNOWN);
            server.send(200, "text/html", "");

            // Wysylaj plik chunkami, podmieniajac {{FW_VERSION}} na biezaco.
            // Uzywa statycznego char bufora zamiast String — brak alokacji heap.
            static const char MARKER[] = "{{FW_VERSION}}";
            static const int MARKER_LEN = 14;  // strlen("{{FW_VERSION}}")
            const size_t BUF_SZ = 512;
            char buf[BUF_SZ];
            char leftover[MARKER_LEN];  // Max rozmiar = dlugosc markera - 1
            int leftoverLen = 0;

            while (f.available()) {
                // Fix #19: Reset WDT w petli streamowania — duzy plik moze
                // trwac kilka sekund (100+ chunkow x TCP latency)
                esp_task_wdt_reset();
                core0AliveMs = millis();

                int r = f.readBytes(buf, BUF_SZ - MARKER_LEN - 1);
                buf[r] = '\0';

                // Polacz leftover z nowym buforem (w miejscu — bez alokacji)
                char combined[BUF_SZ + MARKER_LEN];
                if (leftoverLen > 0) {
                    memcpy(combined, leftover, leftoverLen);
                    memcpy(combined + leftoverLen, buf, r + 1);  // +1 dla '\0'
                    r += leftoverLen;
                    leftoverLen = 0;
                } else {
                    memcpy(combined, buf, r + 1);
                }

                // Szukaj markera w combined
                char* markerPos = strstr(combined, MARKER);
                if (markerPos) {
                    int pos = markerPos - combined;
                    // Wyslij czesc przed markerem
                    if (pos > 0) server.sendContent(combined, pos);
                    server.sendContent(FW_VERSION);
                    // Wyslij czesc po markerze
                    int afterPos = pos + MARKER_LEN;
                    int remaining = r - afterPos;
                    if (remaining > 0) server.sendContent(combined + afterPos, remaining);
                } else {
                    // Zachowaj koniec bufora jako leftover jesli moze zawierac poczatek markera
                    if (f.available() && r >= MARKER_LEN) {
                        // Sprawdz czy koniec bufora zaczyna marker
                        int safeEnd = r;
                        for (int i = MARKER_LEN - 1; i >= 1; i--) {
                            if (r >= i && memcmp(combined + r - i, MARKER, i) == 0) {
                                safeEnd = r - i;
                                memcpy(leftover, combined + safeEnd, i);
                                leftoverLen = i;
                                break;
                            }
                        }
                        if (safeEnd > 0) server.sendContent(combined, safeEnd);
                    } else {
                        server.sendContent(combined, r);
                    }
                }
            }
            // Flush leftover jesli zostal
            if (leftoverLen > 0) {
                server.sendContent(leftover, leftoverLen);
            }
            f.close();
            server.sendContent("");  // End chunked
            served = true;
        }
    }

    if (!served) {
        // --- Fallback PROGMEM (web_html.h) ---
        // Fix #24: Wysylamy PROGMEM chunkami z resetem WDT + core0AliveMs.
        // Poprzednio sendContent_P() blokowala Core 0 na 10-15s (45KB przez TCP)
        // co wyzwalalo falszywy alarm WDT "Core 0 nie odpowiada".
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");

        auto sendProgmemChunked = [this](const char* progmemData) {
            size_t totalLen = strlen_P(progmemData);
            const size_t CHUNK = 512;
            char buf[CHUNK];
            size_t offset = 0;
            while (offset < totalLen) {
                esp_task_wdt_reset();
                core0AliveMs = millis();
                size_t remaining = totalLen - offset;
                size_t toSend = (remaining < CHUNK) ? remaining : CHUNK;
                memcpy_P(buf, progmemData + offset, toSend);
                server.sendContent(buf, toSend);
                offset += toSend;
            }
        };

        sendProgmemChunked(HTML_PART1);
        server.sendContent(FW_VERSION);
        sendProgmemChunked(HTML_PART2);
        server.sendContent("");
    }
}

// ============================================================
// GET /api/stats - Statystyki lifetime + sesja + per-gun
// ============================================================
void TrassarWebServer::handleStats() {
    server.send(200, "application/json", getStatsJson());
}

String TrassarWebServer::getStatsJson() {
    JsonDocument doc;

    // Lifetime
    doc["lifetimeDistanceM"] = serialized(String(stats.getLifetimeDistance(), 1));
    doc["lifetimeAreaM2"] = serialized(String(stats.getLifetimeArea(), 2));
    doc["lifetimePaintTimeSec"] = stats.getLifetimePaintTimeSec();

    // Sesja biezaca
    doc["sessionDistanceM"] = serialized(String(stats.getSessionDistance(), 1));
    doc["sessionAreaM2"] = serialized(String(stats.getSessionArea(), 2));
    doc["sessionTimeSec"] = stats.getSessionTimeSec();

    // Dystans per pistolet (sesja)
    JsonArray gunDist = doc["gunDistances"].to<JsonArray>();
    for (int i = 0; i < NUM_GUNS; i++) {
        gunDist.add(serialized(String(stats.getGunDistance(i), 1)));
    }

    // Licznik strzalow pistoletow (lifetime)
    JsonArray gunShots = doc["gunShotCounts"].to<JsonArray>();
    for (int i = 0; i < NUM_GUNS; i++) {
        gunShots.add(stats.getGunShotCount(i));
    }

    // Raporty SD
    doc["sdReady"] = reportLogger.isReady();
    doc["reportCount"] = reportLogger.getReportCount();

    // Predykcja zuzycia farby
    float totalArea = stats.getLifetimeArea() + stats.getSessionArea();
    doc["paintUsedL"] = serialized(String(paintConsumption.getUsedLiters(stats.getSessionArea()), 1));
    doc["paintRemainingL"] = serialized(String(paintConsumption.getRemainingLiters(totalArea), 1));
    doc["paintTankL"] = serialized(String(paintConsumption.getTankCapacity(), 0));
    doc["paintUsedPct"] = paintConsumption.getUsedPercent(totalArea);
    doc["paintCurrentLevelL"] = serialized(String(paintConsumption.getCurrentLevel(), 1));
    doc["refuelCount"] = paintConsumption.getTotalRefuelCount();
    doc["totalRefueledL"] = serialized(String(paintConsumption.getTotalRefueledL(), 1));

    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================
// GET /api/reports - Lista raportow z karty SD (z cache)
// ============================================================
void TrassarWebServer::handleReports() {
    server.send(200, "application/json", getReportsJson());
}

String TrassarWebServer::getReportsJson() {
    return reportLogger.getReportListJson();
}

// ============================================================
// GET /api/reports/download?file=FILENAME - Pobierz plik CSV
// Strumieniowe wysylanie pliku z karty SD (bez bufora w RAM)
// ============================================================
void TrassarWebServer::handleReportDownload() {
    if (!server.hasArg("file")) {
        server.send(400, "text/plain", "Brak parametru file");
        return;
    }
    String fname = server.arg("file");
    // Zabezpieczenie: tylko litery, cyfry, kropka, podkreslenie
    for (unsigned int i = 0; i < fname.length(); i++) {
        char c = fname.charAt(i);
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            server.send(400, "text/plain", "Nieprawidlowa nazwa pliku");
            return;
        }
    }

    String path = "/reports/" + fname;
    if (!reportLogger.isReady()) {
        server.send(404, "text/plain", "Plik nie znaleziony");
        return;
    }

    if (!SD_LOCK()) {
        server.send(503, "text/plain", "SD zajeta");
        return;
    }

    if (!SD.exists(path.c_str())) {
        SD_UNLOCK();
        server.send(404, "text/plain", "Plik nie znaleziony");
        return;
    }

    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) {
        SD_UNLOCK();
        server.send(500, "text/plain", "Blad otwarcia pliku");
        return;
    }

    // Wyslij naglowki
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
    server.setContentLength(f.size());
    server.send(200, "text/csv", "");

    // Wyslij plik chunkami po 512 bajtow
    uint8_t buf[512];
    while (f.available()) {
        esp_task_wdt_reset();   // Fix #19: WDT reset w petli streamowania SD
        core0AliveMs = millis();
        int r = f.read(buf, sizeof(buf));
        if (r > 0) server.sendContent((const char*)buf, r);
    }
    f.close();
    SD_UNLOCK();
    DBG_PRINTF("[WWW] Pobranie raportu: %s\n", fname.c_str());
}

// ============================================================
// GET /api/reports/geojson?file=RRRRMMDD.csv - CSV -> GeoJSON
// Konwersja raportow sesji do GeoJSON FeatureCollection (Points)
// Strumieniowe wysylanie (chunked) — niskie zuzycie RAM
// ============================================================
void TrassarWebServer::handleGeoJson() {
    if (!server.hasArg("file")) {
        server.send(400, "application/json", "{\"error\":\"brak parametru file\"}");
        return;
    }
    String fname = server.arg("file");
    for (unsigned int i = 0; i < fname.length(); i++) {
        char c = fname.charAt(i);
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            server.send(400, "application/json", "{\"error\":\"nieprawidlowa nazwa\"}");
            return;
        }
    }

    String path = "/reports/" + fname;
    if (!reportLogger.isReady()) {
        server.send(404, "application/json", "{\"error\":\"plik nie znaleziony\"}");
        return;
    }

    if (!SD_LOCK()) {
        server.send(503, "application/json", "{\"error\":\"SD zajeta\"}");
        return;
    }

    if (!SD.exists(path.c_str())) {
        SD_UNLOCK();
        server.send(404, "application/json", "{\"error\":\"plik nie znaleziony\"}");
        return;
    }

    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) {
        SD_UNLOCK();
        server.send(500, "application/json", "{\"error\":\"blad otwarcia\"}");
        return;
    }

    // Chunked GeoJSON FeatureCollection
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/geo+json", "");
    server.sendContent("{\"type\":\"FeatureCollection\",\"features\":[");

    // Pomin naglowek CSV
    char line[200];
    if (f.available()) {
        f.readBytesUntil('\n', line, sizeof(line) - 1);
    }

    bool first = true;
    while (f.available()) {
        esp_task_wdt_reset();   // Fix #19: WDT reset w petli parsowania GeoJSON
        core0AliveMs = millis();
        int r = f.readBytesUntil('\n', line, sizeof(line) - 1);
        line[r] = 0;

        // Format: data,godzina,wzorzec,dystans_m,powierzchnia_m2,lat,lon
        char date[16], timeStr[16], pat[16];
        float dist, area;
        double lat, lon;

        if (sscanf(line, "%15[^,],%15[^,],%15[^,],%f,%f,%lf,%lf",
                   date, timeStr, pat, &dist, &area, &lat, &lon) >= 7) {
            if (lat != 0 || lon != 0) {
                char feat[400];
                snprintf(feat, sizeof(feat),
                    "%s{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
                    "\"coordinates\":[%.7f,%.7f]},\"properties\":{"
                    "\"date\":\"%s\",\"time\":\"%s\",\"pattern\":\"%s\","
                    "\"distance_m\":%.1f,\"area_m2\":%.2f}}",
                    first ? "" : ",",
                    lon, lat, date, timeStr, pat, dist, area);
                server.sendContent(feat);
                first = false;
            }
        }
    }

    f.close();
    SD_UNLOCK();
    server.sendContent("]}");
    server.sendContent("");  // End chunked
    DBG_PRINTF("[WWW] GeoJSON: %s\n", fname.c_str());
}

// ============================================================
// GET /api/tracks - Lista plikow GPS track (GPX + GeoJSON)
// ============================================================
void TrassarWebServer::handleTrackList() {
    if (!reportLogger.isReady()) {
        server.send(503, "application/json", "{\"error\":\"SD niedostepna\"}");
        return;
    }

    if (!SD_LOCK()) {
        server.send(503, "application/json", "{\"error\":\"SD zajeta\"}");
        return;
    }

    if (!SD.exists("/tracks")) {
        SD_UNLOCK();
        server.send(200, "application/json", "[]");
        return;
    }

    File dir = SD.open("/tracks");
    if (!dir) {
        SD_UNLOCK();
        server.send(200, "application/json", "[]");
        return;
    }

    String json = "[";
    bool first = true;
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
            if (!first) json += ",";
            json += "{\"file\":\"";
            json += entry.name();
            json += "\",\"size\":";
            json += String(entry.size());
            json += "}";
            first = false;
        }
        entry.close();
    }
    dir.close();
    SD_UNLOCK();
    json += "]";

    server.send(200, "application/json", json);
}

// ============================================================
// GET /api/tracks/download?file=FILENAME - Pobierz GPX/GeoJSON
// ============================================================
void TrassarWebServer::handleTrackDownload() {
    if (!server.hasArg("file")) {
        server.send(400, "text/plain", "Brak parametru file");
        return;
    }
    String fname = server.arg("file");
    for (unsigned int i = 0; i < fname.length(); i++) {
        char c = fname.charAt(i);
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            server.send(400, "text/plain", "Nieprawidlowa nazwa pliku");
            return;
        }
    }

    String path = "/tracks/" + fname;
    if (!reportLogger.isReady()) {
        server.send(404, "text/plain", "Plik nie znaleziony");
        return;
    }

    if (!SD_LOCK()) {
        server.send(503, "text/plain", "SD zajeta");
        return;
    }

    if (!SD.exists(path.c_str())) {
        SD_UNLOCK();
        server.send(404, "text/plain", "Plik nie znaleziony");
        return;
    }

    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) {
        SD_UNLOCK();
        server.send(500, "text/plain", "Blad otwarcia pliku");
        return;
    }

    // Content type wedlug rozszerzenia
    const char* ct = "application/octet-stream";
    if (fname.endsWith(".gpx")) ct = "application/gpx+xml";
    else if (fname.endsWith(".geojson")) ct = "application/geo+json";

    server.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
    server.setContentLength(f.size());
    server.send(200, ct, "");

    uint8_t buf[512];
    while (f.available()) {
        esp_task_wdt_reset();   // Fix #19: WDT reset w petli streamowania
        core0AliveMs = millis();
        int r = f.read(buf, sizeof(buf));
        if (r > 0) server.sendContent((const char*)buf, r);
    }
    f.close();
    SD_UNLOCK();
    DBG_PRINTF("[WWW] Track download: %s\n", fname.c_str());
}

// buildHtmlPage() - nie uzywane, HTML wysylany chunkami z handleRoot()
String TrassarWebServer::buildHtmlPage() {
    return String();
}
