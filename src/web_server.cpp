// ============================================================
// TrassarV3 - Implementacja serwera WWW (WiFi AP) + WebSocket
// v2.20.0 - WebSocket push, GeoJSON, GPS tracks API, WDT Core 0
// ============================================================

#include "web_server.h"
#include "web_html.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
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
#include <SD.h>

TrassarWebServer webServer;

// ============================================================
// Inicjalizacja WiFi AP i serwera HTTP
// ============================================================
void TrassarWebServer::begin() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CON);

    delay(100);
    Serial.print("[WiFi] AP uruchomiony. IP: ");
    Serial.println(WiFi.softAPIP());

    setupRoutes();
    server.begin();
    Serial.println("[WWW] Serwer HTTP uruchomiony na porcie 80");

    // WebSocket server na porcie 81 (push status updates)
    wsServer.begin();
    wsServer.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (type == WStype_CONNECTED) {
            Serial.printf("[WS] Klient #%u polaczony\n", num);
        } else if (type == WStype_DISCONNECTED) {
            Serial.printf("[WS] Klient #%u rozlaczony\n", num);
        }
    });
    Serial.printf("[WWW] WebSocket na porcie %d\n", WS_PORT);

    // Uruchom task WWW na Core 0 (Arduino loop() dziala na Core 1)
    xTaskCreatePinnedToCore(
        webTaskFunc,        // Funkcja tasku
        "WebServer",        // Nazwa (debug)
        16384,              // Stack size [bytes] (zwiekszone: WS + GeoJSON)
        this,               // Parametr -> wskaznik na obiekt
        1,                  // Priorytet (1 = niski, nie blokuje krytycznych taskow)
        &webTaskHandle,     // Uchwyt tasku
        0                   // Core 0
    );
    Serial.println("[WWW] Task WWW uruchomiony na Core 0");
}

// Task FreeRTOS na Core 0 - obsluga HTTP + WebSocket + watchdog
void TrassarWebServer::webTaskFunc(void* param) {
    TrassarWebServer* self = static_cast<TrassarWebServer*>(param);

    // Poczekaj az setup() zainicjalizuje WDT, potem dodaj ten task
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_task_wdt_add(NULL);
    Serial.println("[WDT] Core 0 WebServer task dodany do watchdoga");

    for (;;) {
        esp_task_wdt_reset();  // Podwojny watchdog: Core 0

        self->server.handleClient();
        self->wsServer.loop();

        // Broadcast statusu do klientow WebSocket co WS_BROADCAST_MS
        unsigned long now = millis();
        if (now - self->lastWsBroadcast >= WS_BROADCAST_MS) {
            self->lastWsBroadcast = now;
            if (self->wsServer.connectedClients() > 0) {
                String json = self->getStateJson();
                self->wsServer.broadcastTXT(json);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));  // 2ms yield - nie blokuj innych taskow
    }
}

void TrassarWebServer::update() {
    // Puste - obsluga HTTP przeniesiona do tasku na Core 0
    // Metoda zachowana dla kompatybilnosci wstecznej
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
    server.onNotFound([this]() { handleNotFound(); });
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
    if (!server.hasArg("action")) {
        server.send(400, "application/json", "{\"error\":\"brak parametru action\"}");
        return;
    }

    String action = server.arg("action");
    String result = "ok";

    // Atomowy snapshot stanu (wymagany do decyzji o akcji)
    STATE_LOCK();
    MachineState snapState = g_state.machineState;
    STATE_UNLOCK();

    if (action == "start") {
        // start/resume/pause/stop maja wewnetrzne locki i sprawdzaja stan ponownie
        if (snapState == STATE_PAUSED) {
            paintEngine.resume();
        } else if (snapState == STATE_IDLE || snapState == STATE_STOPPED) {
            paintEngine.start();
        }
    } else if (action == "start_from_gap") {
        paintEngine.startFromGap();
    } else if (action == "pause") {
        paintEngine.pause();
    } else if (action == "stop") {
        paintEngine.stop();
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
            if (val >= 5.0f && val <= 30.0f) {
                paintEngine.setMaxSpeed(val);
                storage.saveMaxSpeed(val);
            } else {
                result = "zakres 5-30 km/h";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "set_min_speed") {
        if (server.hasArg("value")) {
            float val = server.arg("value").toFloat();
            if (val >= 0.0f && val <= 10.0f) {
                paintEngine.setMinSpeed(val);
                storage.saveMinSpeed(val);
            } else {
                result = "zakres 0-10 km/h";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "set_mode") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val >= 0 && val <= 2) {
                MachineMode newMode = (MachineMode)val;
                STATE_LOCK();
                g_state.machineMode = newMode;
                STATE_UNLOCK();
                storage.saveMode(newMode);
                Serial.printf("[WWW] Tryb pracy: %d\n", val);
            } else {
                result = "nieprawidlowy tryb (0-2)";
            }
        } else {
            result = "brak parametru value";
        }
    } else if (action == "save_custom_pattern") {
        // Parametry: g0..g5, ln0..ln5, gp0..gp5, slot (0-2)
        CustomPatternCfg cfg = {};
        cfg.valid = true;
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
            if (ln < 0.1f) ln = 0.1f;
            if (ln > 50.0f) ln = 50.0f;
            if (gp < 0.1f) gp = 0.1f;
            if (gp > 50.0f) gp = 50.0f;
            cfg.lineLen[i] = ln;
            cfg.gapLen[i] = gp;
        }
        int slot = 0;
        if (server.hasArg("slot")) {
            slot = server.arg("slot").toInt();
            if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
        }
        patternMgr.saveSlot(slot, cfg);
        patternMgr.activateSlot(slot);
        Serial.printf("[WWW] Wzorzec wlasny slot %d zapisany\n", slot);
    } else if (action == "activate_slot") {
        if (server.hasArg("value")) {
            int slot = server.arg("value").toInt();
            if (slot >= 0 && slot < NUM_CUSTOM_SLOTS && patternMgr.isSlotValid(slot)) {
                patternMgr.activateSlot(slot);
            } else {
                result = "slot pusty lub nieprawidlowy";
            }
        }
    } else if (action == "semi_next_line") {
        paintEngine.semiNextLine();
    } else if (action == "send_event") {
        // Wirtualne przyciski z panelu www — wstrzykniecie zdarzenia do menu
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val > 0 && val <= (int)EVT_GAP_START) {
                menu.handleEvent((ButtonEvent)val);
                Serial.printf("[WWW] Event: %d\n", val);
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
                Serial.printf("[WWW] Ekran: %d\n", val);
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
            Serial.printf("[WWW] Tryb przelaczania: %s\n", smart ? "SMART" : "INSTANT");
        }
    } else {
        result = "nieznana akcja";
    }

    STATE_LOCK();
    g_state.displayNeedsUpdate = true;
    STATE_UNLOCK();
    server.send(200, "application/json", "{\"result\":\"" + result + "\"}");
}

// ============================================================
// 404
// ============================================================
void TrassarWebServer::handleNotFound() {
    server.send(404, "text/plain", "404 - Nie znaleziono");
}

// ============================================================
// JSON - stan maszyny
// ============================================================
String TrassarWebServer::getStateJson() {
    JsonDocument doc;

    // --- Atomowy snapshot g_state (bezpieczny odczyt z Core 0) ---
    STATE_LOCK();
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

    // Anomalia pistoletow (odczyt pod lockiem — modyfikowane z Core 1)
    STATE_LOCK();
    bool snapAnomalyDetected = gunAnomaly.detected;
    bool snapAnomalyAlert[NUM_GUNS];
    for (int i = 0; i < NUM_GUNS; i++) snapAnomalyAlert[i] = gunAnomaly.alert[i];
    STATE_UNLOCK();

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
// GET / - Strona HTML (chunked transfer z PROGMEM)
// Nie alokuje calej strony w RAM - wysyla fragmentami z flash
// ============================================================
void TrassarWebServer::handleRoot() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(HTML_PART1);
    server.sendContent(FW_VERSION);
    server.sendContent_P(HTML_PART2);
    server.sendContent("");  // koniec chunked
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
        int r = f.read(buf, sizeof(buf));
        if (r > 0) server.sendContent((const char*)buf, r);
    }
    f.close();
    SD_UNLOCK();
    Serial.printf("[WWW] Pobranie raportu: %s\n", fname.c_str());
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
    Serial.printf("[WWW] GeoJSON: %s\n", fname.c_str());
}

// ============================================================
// GET /api/tracks - Lista plikow GPS track (GPX + GeoJSON)
// ============================================================
void TrassarWebServer::handleTrackList() {
    if (!reportLogger.isReady()) {
        server.send(200, "application/json", "[]");
        return;
    }

    if (!SD_LOCK()) {
        server.send(200, "application/json", "[]");
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
        int r = f.read(buf, sizeof(buf));
        if (r > 0) server.sendContent((const char*)buf, r);
    }
    f.close();
    SD_UNLOCK();
    Serial.printf("[WWW] Track download: %s\n", fname.c_str());
}

// buildHtmlPage() - nie uzywane, HTML wysylany chunkami z handleRoot()
String TrassarWebServer::buildHtmlPage() {
    return String();
}
