// ============================================================
// TrassarV3 - Implementacja serwera WWW (WiFi AP)
// Komputer pokladowy malowarki pasow drogowych
// ============================================================

#include "web_server.h"
#include <ArduinoJson.h>
#include "painting_engine.h"
#include "encoder_distance.h"
#include "statistics.h"
#include "guns.h"
#include "patterns.h"
#include "rtc_handler.h"
#include "storage.h"
#include "report_logger.h"

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

    // Uruchom task WWW na Core 0 (Arduino loop() dziala na Core 1)
    xTaskCreatePinnedToCore(
        webTaskFunc,        // Funkcja tasku
        "WebServer",        // Nazwa (debug)
        12288,              // Stack size [bytes]
        this,               // Parametr -> wskaznik na obiekt
        1,                  // Priorytet (1 = niski, nie blokuje krytycznych taskow)
        &webTaskHandle,     // Uchwyt tasku
        0                   // Core 0
    );
    Serial.println("[WWW] Task WWW uruchomiony na Core 0");
}

// Task FreeRTOS na Core 0 - obsluga klientow HTTP
void TrassarWebServer::webTaskFunc(void* param) {
    TrassarWebServer* self = static_cast<TrassarWebServer*>(param);
    for (;;) {
        self->server.handleClient();
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

    if (action == "start") {
        if (g_state.machineState == STATE_PAUSED) {
            paintEngine.resume();
        } else if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
            paintEngine.start();
        }
    } else if (action == "start_from_gap") {
        if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
            paintEngine.startFromGap();
        }
    } else if (action == "pause") {
        if (g_state.machineState == STATE_PAINTING) {
            paintEngine.pause();
        }
    } else if (action == "stop") {
        if (g_state.machineState == STATE_PAINTING || g_state.machineState == STATE_PAUSED) {
            paintEngine.stop();
        }
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

    // Anomalia pistoletow
    doc["gunAnomalyDetected"] = gunAnomaly.detected;
    JsonArray anomArr = doc["gunAnomaly"].to<JsonArray>();
    for (int i = 0; i < NUM_GUNS; i++) {
        anomArr.add(gunAnomaly.alert[i]);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================
// HTML - strona panelu sterowania
// Podzielona na 2 czesci PROGMEM (punkt podzialu: FW_VERSION w stopce)
// Wysylana chunkami - nie alokuje ~7KB String w RAM
// ============================================================

// --- HTML PART 1: od poczatku do FW_VERSION ---
static const char HTML_PART1[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>TRASSAR V3 - Panel sterowania</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{
    font-family:'Segoe UI',system-ui,-apple-system,Arial,sans-serif;
    background:#0a0e17;
    color:#e0e6f0;
    min-height:100vh;
    -webkit-tap-highlight-color:transparent;
}
.header{
    background:linear-gradient(135deg,#1a2332,#0d1520);
    border-bottom:2px solid #2a7d4f;
    padding:14px 16px;
    text-align:center;
    position:sticky;top:0;z-index:100;
}
.header h1{font-size:20px;color:#2ae67a;letter-spacing:2px;}
.header .sub{font-size:11px;color:#6b7d9a;margin-top:3px;}
.container{max-width:520px;margin:0 auto;padding:10px;}
.card{
    background:#111927;
    border-radius:12px;
    border:1px solid #1e2d42;
    padding:16px;
    margin-bottom:10px;
}
.card h3{
    font-size:12px;color:#6b7d9a;
    text-transform:uppercase;letter-spacing:1px;
    margin-bottom:10px;
    padding-bottom:6px;
    border-bottom:1px solid #1e2d42;
}

/* ---------- STATUS ---------- */
.st-wrap{text-align:center;margin-bottom:8px;}
.st-dot{
    display:inline-block;width:14px;height:14px;
    border-radius:50%;margin-right:8px;vertical-align:middle;
}
.st-dot.pulse{animation:pulse 1.5s infinite;}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:.4;}}
.st-label{font-size:13px;vertical-align:middle;}
.st-big{font-size:26px;font-weight:bold;margin:6px 0;}

.c-idle .st-dot{background:#6b7d9a;} .c-idle .st-big{color:#6b7d9a;}
.c-painting .st-dot{background:#2ae67a;} .c-painting .st-big{color:#2ae67a;}
.c-paused .st-dot{background:#f0c040;} .c-paused .st-big{color:#f0c040;}
.c-stopped .st-dot{background:#e64040;} .c-stopped .st-big{color:#e64040;}

.info-grid{
    display:grid;grid-template-columns:1fr 1fr 1fr;
    gap:6px;margin-top:10px;
}
.info-grid.two{grid-template-columns:1fr 1fr;}
.info-item{
    background:#0d1520;border-radius:8px;
    padding:10px 6px;text-align:center;
}
.info-item .lbl{font-size:10px;color:#6b7d9a;text-transform:uppercase;letter-spacing:.5px;}
.info-item .val{font-size:17px;font-weight:bold;color:#2ae67a;margin-top:3px;}
.info-item .val.warn{color:#f0c040;}

/* ---------- CONTROLS ---------- */
.controls{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-bottom:8px;}
.controls2{display:grid;grid-template-columns:1fr;gap:8px;margin-bottom:10px;}
.btn-gap{background:linear-gradient(135deg,#6a4a10,#f0c040);color:#0a0e17;}
.btn{
    padding:14px 6px;border:none;border-radius:10px;
    font-size:14px;font-weight:bold;cursor:pointer;
    transition:all .15s;text-transform:uppercase;letter-spacing:1px;
}
.btn:active{transform:scale(.95);}
.btn:disabled{opacity:.3;cursor:not-allowed;transform:none;}
.btn-start{background:linear-gradient(135deg,#1a8a4a,#2ae67a);color:#0a0e17;}
.btn-pause{background:linear-gradient(135deg,#b08a20,#f0c040);color:#0a0e17;}
.btn-stop{background:linear-gradient(135deg,#8a2020,#e64040);color:#fff;}

/* ---------- PATTERNS ---------- */
.pat-group{margin-bottom:8px;}
.pat-group .gl{font-size:11px;color:#4a6080;margin-bottom:4px;}
.pat-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(64px,1fr));gap:5px;}
.pbtn{
    padding:10px 4px;border:1px solid #1e2d42;border-radius:8px;
    background:#0d1520;color:#9eafc4;font-size:13px;font-weight:600;
    cursor:pointer;text-align:center;transition:all .15s;
}
.pbtn:active{transform:scale(.95);}
.pbtn.act{background:#1a8a4a;border-color:#2ae67a;color:#fff;}
.pbtn.pending{border-color:#ffa500;color:#ffa500;animation:pendBlink .8s infinite;}
@keyframes pendBlink{0%,100%{opacity:1;}50%{opacity:.4;}}
.pbtn-rev{
    padding:10px 8px;border:1px solid #1e2d42;border-radius:8px;
    background:#0d1520;color:#9eafc4;font-size:12px;font-weight:600;
    cursor:pointer;text-align:center;transition:all .15s;
    margin-top:6px;width:100%;
}
.pbtn-rev:active{transform:scale(.95);}
.pbtn-rev.act{background:#b08a20;border-color:#f0c040;color:#fff;}
.pbtn-rev:disabled{opacity:.3;cursor:not-allowed;transform:none;}

/* ---------- GUNS ---------- */
.guns-row{display:flex;justify-content:space-around;align-items:center;flex-wrap:wrap;gap:8px;}
.gun-item{text-align:center;}
.gun-circle{
    width:36px;height:36px;border-radius:50%;
    border:2px solid #1e2d42;margin:0 auto 4px;
    display:flex;align-items:center;justify-content:center;
    font-size:10px;font-weight:bold;color:#3a4a60;
    transition:all .3s;
}
.gun-circle.on{background:#1a8a4a;border-color:#2ae67a;color:#fff;box-shadow:0 0 8px rgba(42,230,122,.4);}
.gun-circle.off{background:#1a1a2e;border-color:#2a2a40;color:#4a4a6a;}
.gun-label{font-size:10px;color:#6b7d9a;}

/* ---------- CALIBRATION ---------- */
.cal-row{display:flex;align-items:center;gap:8px;flex-wrap:wrap;}
.cal-btn{
    padding:10px 16px;border:1px solid #1e2d42;border-radius:8px;
    background:#0d1520;color:#9eafc4;font-size:13px;font-weight:600;
    cursor:pointer;transition:all .15s;flex-shrink:0;
}
.cal-btn:active{transform:scale(.95);}
.cal-btn.active{background:#b08a20;border-color:#f0c040;color:#fff;}
.cal-info{font-size:13px;color:#6b7d9a;line-height:1.6;}
.cal-info span{color:#2ae67a;font-weight:bold;}

/* ---------- SYSTEM INFO ---------- */
.sys-info{font-size:12px;color:#6b7d9a;line-height:1.8;}
.sys-info span{color:#2ae67a;}

/* ---------- FOOTER ---------- */
.footer{text-align:center;padding:10px;color:#3a4a60;font-size:10px;margin-top:4px;}

/* ---------- SERVICE MENU ---------- */
.svc-tabs{display:flex;gap:4px;margin-bottom:10px;}
.svc-tab{
    flex:1;padding:10px 8px;border:1px solid #1e2d42;border-radius:8px;
    background:#0d1520;color:#9eafc4;font-size:12px;font-weight:600;
    cursor:pointer;text-align:center;transition:all .15s;
}
.svc-tab:active{transform:scale(.95);}
.svc-tab.act{background:#1a3a5a;border-color:#2a7d9f;color:#fff;}
.rep-tbl{width:100%;border-collapse:collapse;}
.rep-tbl th{text-align:left;padding:4px;color:#6b7d9a;font-size:11px;border-bottom:1px solid #1e2d42;}
.rep-tbl td{padding:6px 4px;color:#e0e6f0;border-bottom:1px solid #0d1520;font-size:12px;}
.rep-tbl td:last-child{text-align:right;color:#2ae67a;}
.anom-warn{
    background:#3a1020;border:1px solid #e64040;border-radius:8px;
    padding:10px;margin-bottom:10px;text-align:center;
    font-size:12px;color:#e64040;font-weight:bold;
    animation:anomBlink 1s infinite;
}
.gun-circle.anom{border-color:#e64040;animation:anomBlink 1s infinite;}
@keyframes anomBlink{0%,100%{box-shadow:0 0 8px rgba(230,64,64,.6);}50%{box-shadow:none;}}
</style>
</head>
<body>
<div class="header">
    <h1>TRASSAR V3</h1>
    <div class="sub">Malowarka drogowa</div>
</div>

<div class="container">

    <!-- ========== STATUS ========== -->
    <div class="card">
        <div id="stWrap" class="st-wrap c-idle">
            <span class="st-dot pulse"></span>
            <span class="st-label" id="stLabel">Gotowy</span>
            <div class="st-big" id="stBig">GOTOWY</div>
        </div>

        <div class="info-grid">
            <div class="info-item">
                <div class="lbl">Wzorzec</div>
                <div class="val" id="vPat">P-1a</div>
            </div>
            <div class="info-item">
                <div class="lbl">Nazwa</div>
                <div class="val" id="vPatName" style="font-size:11px;">---</div>
            </div>
            <div class="info-item">
                <div class="lbl">Predkosc</div>
                <div class="val" id="vSpeed">0.0 km/h</div>
            </div>
        </div>
        <div class="info-grid" style="margin-top:6px;">
            <div class="info-item">
                <div class="lbl">Dystans</div>
                <div class="val" id="vDist">0.0 m</div>
            </div>
            <div class="info-item">
                <div class="lbl">Powierzchnia</div>
                <div class="val" id="vArea">0.00 m&sup2;</div>
            </div>
            <div class="info-item">
                <div class="lbl">Czas</div>
                <div class="val" id="vTime">00:00</div>
            </div>
        </div>
        <div class="info-grid two" style="margin-top:6px;">
            <div class="info-item">
                <div class="lbl">Odwrocony</div>
                <div class="val" id="vRev">NIE</div>
            </div>
            <div class="info-item">
                <div class="lbl">Kalibracja</div>
                <div class="val" id="vCal">---</div>
            </div>
        </div>
    </div>

    <!-- ========== CONTROL BUTTONS ========== -->
    <div class="controls">
        <button class="btn btn-start" id="btnStart" onclick="cmd('start')">START</button>
        <button class="btn btn-pause" id="btnPause" onclick="cmd('pause')">PAUZA</button>
        <button class="btn btn-stop" id="btnStop" onclick="cmd('stop')">STOP</button>
    </div>
    <div class="controls2">
        <button class="btn btn-gap" id="btnGap" onclick="cmd('start_from_gap')">START OD PRZERWY</button>
    </div>

    <!-- ========== PATTERN SELECTION ========== -->
    <div class="card">
        <h3>Wybor wzorca</h3>

        <div class="pat-group">
            <div class="gl">Przerywane (P-1x)</div>
            <div class="pat-grid">
                <div class="pbtn" data-pid="0" onclick="setPat(0)">P-1a</div>
                <div class="pbtn" data-pid="1" onclick="setPat(1)">P-1b</div>
                <div class="pbtn" data-pid="2" onclick="setPat(2)">P-1c</div>
                <div class="pbtn" data-pid="3" onclick="setPat(3)">P-1d</div>
                <div class="pbtn" data-pid="4" onclick="setPat(4)">P-1e</div>
            </div>
        </div>

        <div class="pat-group">
            <div class="gl">Ciagle (P-2x)</div>
            <div class="pat-grid">
                <div class="pbtn" data-pid="5" onclick="setPat(5)">P-2a</div>
                <div class="pbtn" data-pid="6" onclick="setPat(6)">P-2b</div>
            </div>
        </div>

        <div class="pat-group">
            <div class="gl">Przekraczalne (P-3x)</div>
            <div class="pat-grid">
                <div class="pbtn" data-pid="7" onclick="setPat(7)">P-3a</div>
                <div class="pbtn" data-pid="8" onclick="setPat(8)">P-3b</div>
            </div>
            <button class="pbtn-rev" id="btnRev" onclick="cmd('toggle_reverse')">Odwroc</button>
        </div>

        <div class="pat-group">
            <div class="gl">Podwojna / Ostrzegawcza</div>
            <div class="pat-grid">
                <div class="pbtn" data-pid="9" onclick="setPat(9)">P-4</div>
                <div class="pbtn" data-pid="10" onclick="setPat(10)">P-6</div>
            </div>
        </div>

        <div class="pat-group">
            <div class="gl">Krawedziowe (P-7x)</div>
            <div class="pat-grid">
                <div class="pbtn" data-pid="11" onclick="setPat(11)">P-7a</div>
                <div class="pbtn" data-pid="12" onclick="setPat(12)">P-7b</div>
                <div class="pbtn" data-pid="13" onclick="setPat(13)">P-7c</div>
                <div class="pbtn" data-pid="14" onclick="setPat(14)">P-7d</div>
            </div>
        </div>
        <!-- Pattern preview canvas -->
        <div style="margin-top:10px;">
            <div style="font-size:10px;color:#6b7d9a;margin-bottom:4px;">PODGLAD WZORCA</div>
            <canvas id="patCvs" width="480" height="70" style="width:100%;height:70px;background:#0d1520;border-radius:6px;border:1px solid #1e2d42;"></canvas>
        </div>
        <!-- Switch mode toggle -->
        <div style="margin-top:10px;display:flex;align-items:center;gap:8px;">
            <span style="font-size:10px;color:#6b7d9a;">ZMIANA WZORCA:</span>
            <button class="pbtn" id="swSmart" onclick="setSwitchMode(0)" style="font-size:10px;padding:4px 10px;">Smart</button>
            <button class="pbtn" id="swInst" onclick="setSwitchMode(1)" style="font-size:10px;padding:4px 10px;">Instant</button>
            <span id="swDesc" style="font-size:9px;color:#4a5d78;flex:1;">dokonczy cykl</span>
        </div>
    </div>

    <!-- ========== MODE SELECT ========== -->
    <div class="card">
        <h3>Tryb pracy</h3>
        <div class="pat-grid" style="grid-template-columns:1fr 1fr 1fr;">
            <div class="pbtn" id="mAuto" onclick="setMode(0)">AUTO</div>
            <div class="pbtn" id="mSemi" onclick="setMode(1)">SEMI</div>
            <div class="pbtn" id="mManual" onclick="setMode(2)">RECZNY</div>
        </div>
        <div id="modeDesc" style="font-size:11px;color:#6b7d9a;margin-top:8px;text-align:center;">Automatyczny - dystans steruje pistoletami</div>
        <div id="semiBtn" style="display:none;margin-top:8px;">
            <button class="btn btn-start" style="width:100%;" onclick="cmd('semi_next_line')">NASTEPNA LINIA</button>
        </div>
    </div>

    <!-- ========== CUSTOM PATTERN ========== -->
    <div class="card">
        <h3>Wzorzec wlasny</h3>
        <div style="display:flex;gap:4px;margin-bottom:10px;">
            <button class="svc-tab act" id="slotTab0" onclick="selSlot(0)">Slot 1</button>
            <button class="svc-tab" id="slotTab1" onclick="selSlot(1)">Slot 2</button>
            <button class="svc-tab" id="slotTab2" onclick="selSlot(2)">Slot 3</button>
        </div>
        <div id="cpGuns"></div>
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px;">
            <button class="cal-btn" onclick="saveCustomPat()">Zapisz slot</button>
            <button class="cal-btn" onclick="setPat(15)" id="cpUseBtn" style="opacity:.4;">Uzyj wzorca</button>
        </div>
    </div>

    <!-- ========== GUN STATUS ========== -->
    <div class="card">
        <h3>Pistolety</h3>
        <div class="guns-row">
            <div class="gun-item"><div class="gun-circle off" id="g0">P1</div><div class="gun-label">Os L 12</div></div>
            <div class="gun-item"><div class="gun-circle off" id="g1">P2</div><div class="gun-label">Os C 12</div></div>
            <div class="gun-item"><div class="gun-circle off" id="g2">P3</div><div class="gun-label">Os R 12</div></div>
            <div class="gun-item"><div class="gun-circle off" id="g3">P4</div><div class="gun-label">Os 24</div></div>
            <div class="gun-item"><div class="gun-circle off" id="g4">P5</div><div class="gun-label">Kraw 12</div></div>
            <div class="gun-item"><div class="gun-circle off" id="g5">P6</div><div class="gun-label">Kraw 24</div></div>
        </div>
    </div>

    <!-- ========== GUN ANOMALY WARNING ========== -->
    <div id="anomWarn" class="anom-warn" style="display:none;">
        ANOMALIA PISTOLETU - sprawdz dysze!
    </div>

    <!-- ========== CALIBRATION ========== -->
    <div class="card">
        <h3>Kalibracja enkodera</h3>
        <div class="cal-row">
            <button class="cal-btn" id="calBtn" onclick="calAction()">Rozpocznij</button>
            <div class="cal-info">
                Status: <span id="calSt">---</span><br>
                Impulsy/metr: <span id="calPpm">---</span>
            </div>
        </div>
        <div id="calPulseRow" style="display:none;margin-top:8px;font-size:12px;color:#6b7d9a;">
            Impulsy: <span style="color:#f0c040;font-weight:bold;" id="calPulses">0</span>
        </div>
    </div>

    <!-- ========== SPEED ALARM ========== -->
    <div class="card">
        <h3>Alarm predkosci</h3>
        <div class="cal-row">
            <div class="cal-info">
                Maks. predkosc: <span id="spdMax">15.0</span> km/h<br>
                <span id="spdWarn" style="display:none;color:#e64040;font-weight:bold;">PRZEKROCZENIE!</span>
            </div>
        </div>
        <div style="margin-top:10px;display:flex;align-items:center;gap:8px;flex-wrap:wrap;">
            <input type="range" id="spdSlider" min="5" max="30" step="0.5" value="15"
                style="flex:1;min-width:120px;accent-color:#2ae67a;">
            <span id="spdSliderVal" style="font-size:14px;font-weight:bold;color:#2ae67a;min-width:60px;">15.0 km/h</span>
            <button class="cal-btn" onclick="setMaxSpeed()">Zapisz</button>
        </div>
    </div>

    <!-- ========== SYSTEM INFO ========== -->
    <div class="card">
        <h3>Informacje systemowe</h3>
        <div class="sys-info">
            Firmware: <span id="sFw">---</span><br>
            Wolna RAM: <span id="sRam">---</span><br>
            Uptime: <span id="sUp">---</span><br>
            Klienci WiFi: <span id="sCli">---</span>
        </div>
    </div>

    <!-- ========== SERVICE MENU ========== -->
    <div class="card">
        <h3>Menu serwisowe</h3>
        <div class="svc-tabs">
            <button class="svc-tab act" onclick="svcTab(0)">Statystyki</button>
            <button class="svc-tab" onclick="svcTab(1)">Raporty SD</button>
        </div>
        <div id="svcP0">
            <div class="info-grid two">
                <div class="info-item"><div class="lbl">Dyst. calkowity</div><div class="val" id="ltDist">---</div></div>
                <div class="info-item"><div class="lbl">Pow. calkowita</div><div class="val" id="ltArea">---</div></div>
            </div>
            <div class="info-grid two" style="margin-top:6px;">
                <div class="info-item"><div class="lbl">Czas malowania</div><div class="val" id="ltTime">---</div></div>
                <div class="info-item"><div class="lbl">Karta SD</div><div class="val" id="ltSd">---</div></div>
            </div>
            <div style="margin-top:8px;font-size:11px;color:#6b7d9a;">Dystans per pistolet (sesja):</div>
            <div class="guns-row" style="margin-top:6px;" id="gunDistRow"></div>
            <div style="margin-top:8px;font-size:11px;color:#6b7d9a;">Licznik strzalow per pistolet (lifetime):</div>
            <div class="guns-row" style="margin-top:6px;" id="gunShotRow"></div>
        </div>
        <div id="svcP1" style="display:none;">
            <div id="repList" style="font-size:12px;color:#6b7d9a;">Ladowanie...</div>
            <button class="cal-btn" style="margin-top:8px;" onclick="loadReports()">Odswiez</button>
        </div>
    </div>

</div>

<div class="footer">
    TRASSAR V3 &copy; 2025 | Firmware v)rawhtml";

// --- HTML PART 2: od FW_VERSION do konca ---
static const char HTML_PART2[] PROGMEM = R"rawhtml(
</div>

<script>
/* ------- Pattern code table (mirrors PatternID enum) ------- */
const PAT_CODES=[
    "P-1a","P-1b","P-1c","P-1d","P-1e",
    "P-2a","P-2b",
    "P-3a","P-3b",
    "P-4","P-6",
    "P-7a","P-7b","P-7c","P-7d",
    "WLASNY"
];
/* P-3a=7, P-3b=8 are reversible */
const REV_PATS=[7,8];
const GUN_NAMES=["P1 Os L 12","P2 Os C 12","P3 Os R 12","P4 Os 24","P5 Kraw 12","P6 Kraw 24"];
const MODE_DESC=["Automatyczny - dystans steruje pistoletami","Polautomatyczny - auto linia, reczna przerwa","Reczny - trzymaj START aby strzelac"];

let calibrating=false;
let spdSliderLoaded=false;
let cpInited=false;
let curSlot=0;
/* Pattern definitions: array of [gunName, widthCm, lineLen, gapLen] per gun */
/* lineLen=0 => continuous */
const PAT_DEFS=[
    [['P2',12,4,8]],                         /* P-1a */
    [['P2',12,2,4]],                         /* P-1b */
    [['P2',12,2,2]],                         /* P-1c */
    [['P2',12,1,1]],                         /* P-1d */
    [['P4',24,1,1]],                         /* P-1e */
    [['P2',12,0,0]],                         /* P-2a */
    [['P4',24,0,0]],                         /* P-2b */
    [['P1',12,0,0],['P3',12,4,2]],          /* P-3a */
    [['P1',12,0,0],['P3',12,1,1]],          /* P-3b */
    [['P1',12,0,0],['P3',12,0,0]],          /* P-4  */
    [['P5',12,4,2]],                         /* P-6  */
    [['P6',24,1,1]],                         /* P-7a */
    [['P6',24,0,0]],                         /* P-7b */
    [['P5',12,1,1]],                         /* P-7c */
    [['P5',12,0,0]],                         /* P-7d */
    []                                        /* custom */
];

/* ------- Commands ------- */
function cmd(action){
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action='+action
    }).then(r=>r.json()).then(()=>fetchStatus());
}
function setPat(id){
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=set_pattern&value='+id
    }).then(r=>r.json()).then(()=>fetchStatus());
}
function calAction(){
    if(calibrating){
        cmd('cal_finish');
    } else {
        cmd('cal_start');
    }
}
function setMaxSpeed(){
    let val=document.getElementById('spdSlider').value;
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=set_max_speed&value='+val
    }).then(r=>r.json()).then(()=>fetchStatus());
}
/* ------- Mode select ------- */
function setMode(m){
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=set_mode&value='+m
    }).then(r=>r.json()).then(()=>fetchStatus());
}
/* ------- Switch mode ------- */
function setSwitchMode(m){
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=set_switch_mode&value='+m
    }).then(r=>r.json()).then(()=>fetchStatus());
}
/* ------- Custom pattern ------- */
function cpGunChanged(i){
    let sel=document.getElementById('cpG'+i);
    let row=document.getElementById('cpRow'+i);
    if(sel.value==='2'){row.style.display='grid';}
    else{row.style.display='none';}
}
function initCustomGuns(){
    if(cpInited)return;cpInited=true;
    let el=document.getElementById('cpGuns');
    let h='';
    let inS='style="width:100%;padding:6px;background:#0d1520;border:1px solid #1e2d42;border-radius:6px;color:#e0e6f0;font-size:13px;"';
    for(let i=0;i<6;i++){
        h+='<div style="display:grid;grid-template-columns:1fr 2fr;gap:6px;align-items:center;margin-bottom:4px;">';
        h+='<span style="font-size:12px;color:#9eafc4;">'+GUN_NAMES[i]+'</span>';
        h+='<select id="cpG'+i+'" onchange="cpGunChanged('+i+')" style="padding:6px;background:#0d1520;border:1px solid #1e2d42;border-radius:6px;color:#e0e6f0;font-size:12px;">';
        h+='<option value="0">Wylaczony</option><option value="1">Ciagly</option><option value="2">Przerywany</option>';
        h+='</select></div>';
        h+='<div id="cpRow'+i+'" style="display:none;grid-template-columns:1fr 1fr;gap:6px;margin:0 0 8px 0;padding-left:8px;border-left:2px solid #1e2d42;">';
        h+='<div><label style="font-size:10px;color:#6b7d9a;">Kreska [m]</label>';
        h+='<input type="number" id="cpLn'+i+'" value="4.0" min="0.1" max="50" step="0.1" '+inS+'></div>';
        h+='<div><label style="font-size:10px;color:#6b7d9a;">Przerwa [m]</label>';
        h+='<input type="number" id="cpGp'+i+'" value="8.0" min="0.1" max="50" step="0.1" '+inS+'></div>';
        h+='</div>';
    }
    el.innerHTML=h;
}
function saveCustomPat(){
    let body='action=save_custom_pattern&slot='+curSlot;
    for(let i=0;i<6;i++){
        body+='&g'+i+'='+document.getElementById('cpG'+i).value;
        let ln=document.getElementById('cpLn'+i).value||'4.0';
        let gp=document.getElementById('cpGp'+i).value||'8.0';
        body+='&ln'+i+'='+ln+'&gp'+i+'='+gp;
    }
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:body
    }).then(r=>r.json()).then(()=>fetchStatus());
}
function selSlot(s){
    curSlot=s;
    for(let i=0;i<3;i++){
        let t=document.getElementById('slotTab'+i);
        if(i===s)t.classList.add('act');else t.classList.remove('act');
    }
    fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=activate_slot&value='+s}).then(r=>r.json()).then(()=>fetchStatus());
}
/* ------- Pattern preview canvas (multi-gun) ------- */
function drawPatPreview(patIdx){
    let cvs=document.getElementById('patCvs');if(!cvs)return;
    let ctx=cvs.getContext('2d');
    let guns=PAT_DEFS[patIdx];
    if(!guns||guns.length===0){
        cvs.height=50;cvs.style.height='50px';
        ctx.clearRect(0,0,cvs.width,cvs.height);
        ctx.fillStyle='#6b7d9a';ctx.font='12px sans-serif';ctx.textAlign='center';
        ctx.fillText('Wzorzec wlasny',cvs.width/2,28);return;
    }
    let nG=guns.length;
    let rowH=32,pad=4,lblW=72,topM=4;
    let totalH=nG*rowH+nG*pad+topM+14;
    cvs.height=totalH;cvs.style.height=totalH+'px';
    let W=cvs.width;
    ctx.clearRect(0,0,W,totalH);
    let drawW=W-lblW-10;
    /* find max cycle for consistent scale */
    let maxCyc=0;
    for(let g=0;g<nG;g++){let c=guns[g][2]+guns[g][3];if(c>maxCyc)maxCyc=c;}
    for(let g=0;g<nG;g++){
        let gn=guns[g][0],wCm=guns[g][1],ln=guns[g][2],gp=guns[g][3];
        let y=topM+g*(rowH+pad);
        /* label: gun name + width badge */
        ctx.fillStyle='#9eafc4';ctx.font='bold 11px sans-serif';ctx.textAlign='left';
        ctx.fillText(gn,4,y+14);
        let badge=wCm+'cm';
        ctx.fillStyle=wCm>=24?'#e6a02a':'#2a9de6';
        ctx.font='9px sans-serif';
        ctx.fillText(badge,4,y+26);
        let wTag=wCm>=24?'szer.':'wask.';
        ctx.fillStyle='#4a5d78';ctx.fillText(wTag,32,y+26);
        /* draw pattern bar */
        let x0=lblW;
        if(ln<=0&&gp<=0){
            /* continuous */
            ctx.fillStyle='#2ae67a';ctx.fillRect(x0,y+2,drawW,rowH-4);
            ctx.fillStyle='#0a0e17';ctx.font='11px sans-serif';ctx.textAlign='center';
            ctx.fillText('Ciagly',x0+drawW/2,y+rowH/2+4);
            ctx.textAlign='left';
        } else {
            let cycle=ln+gp;if(cycle<=0)continue;
            let scale=drawW/Math.max(cycle*3,cycle);
            if(scale>50)scale=50;if(scale<2)scale=2;
            let x=x0;
            for(let rep=0;rep<30&&x<x0+drawW;rep++){
                let lw=Math.min(ln*scale,x0+drawW-x);
                ctx.fillStyle='#2ae67a';ctx.fillRect(x,y+2,Math.max(lw,1),rowH-4);
                x+=ln*scale;
                if(x>=x0+drawW)break;
                let gw=Math.min(gp*scale,x0+drawW-x);
                ctx.fillStyle='#1e2d42';ctx.fillRect(x,y+2,Math.max(gw,1),rowH-4);
                x+=gp*scale;
            }
            /* dimensions label */
            ctx.fillStyle='#9eafc4';ctx.font='10px sans-serif';ctx.textAlign='center';
            ctx.fillText(ln+'m / '+gp+'m',x0+drawW/2,y+rowH-2);
            ctx.textAlign='left';
        }
    }
}
/* Speed slider live update */
document.addEventListener('DOMContentLoaded',function(){
    let sl=document.getElementById('spdSlider');
    if(sl) sl.addEventListener('input',function(){
        document.getElementById('spdSliderVal').textContent=parseFloat(this.value).toFixed(1)+' km/h';
    });
    initCustomGuns();
});

/* ------- Time format ------- */
function fmtTime(sec){
    let h=Math.floor(sec/3600);
    let m=Math.floor((sec%3600)/60);
    let s=sec%60;
    if(h>0) return h+':'+String(m).padStart(2,'0')+':'+String(s).padStart(2,'0');
    return String(m).padStart(2,'0')+':'+String(s).padStart(2,'0');
}

/* ------- Fetch & Update UI ------- */
function fetchStatus(){
    fetch('/api/status').then(r=>r.json()).then(d=>{
        /* State */
        let w=document.getElementById('stWrap');
        let labels={idle:'Gotowy',painting:'Malowanie',paused:'Pauza',stopped:'Zatrzymany'};
        let upper={idle:'GOTOWY',painting:'MALOWANIE',paused:'PAUZA',stopped:'ZATRZYMANY'};
        w.className='st-wrap c-'+d.state;
        document.getElementById('stLabel').textContent=labels[d.state]||d.state;
        document.getElementById('stBig').textContent=upper[d.state]||d.state.toUpperCase();

        /* Info cards */
        document.getElementById('vPat').textContent=d.pattern;
        document.getElementById('vPatName').textContent=d.patternName;
        let spdEl=document.getElementById('vSpeed');
        spdEl.textContent=d.speed+' km/h';
        if(d.overspeed){spdEl.className='val';spdEl.style.color='#e64040';}
        else if(d.lowSpeed){spdEl.className='val warn';spdEl.style.color='';}
        else{spdEl.className='val';spdEl.style.color='';}
        document.getElementById('vDist').textContent=d.distance+' m';
        document.getElementById('vArea').innerHTML=d.area+' m&sup2;';
        document.getElementById('vTime').textContent=fmtTime(d.elapsed);

        /* Reversed */
        let revEl=document.getElementById('vRev');
        if(d.reversed){revEl.textContent='TAK';revEl.className='val warn';}
        else{revEl.textContent='NIE';revEl.className='val';}

        /* Calibration status in info card */
        let calEl=document.getElementById('vCal');
        if(d.calibrating){calEl.textContent='Trwa...';calEl.className='val warn';}
        else if(d.calibrated){calEl.textContent='TAK';calEl.className='val';}
        else{calEl.textContent='NIE';calEl.className='val warn';}

        /* Gap start indicator */
        let gapEl=document.getElementById('vRev');
        if(d.gapStart){
            document.getElementById('stBig').textContent+=' [PRZERWA]';
        }

        /* Control buttons */
        let bs=document.getElementById('btnStart');
        let bp=document.getElementById('btnPause');
        let bt=document.getElementById('btnStop');
        let bg=document.getElementById('btnGap');
        bs.disabled=(d.state==='painting');
        bp.disabled=(d.state!=='painting');
        bt.disabled=(d.state==='idle'||d.state==='stopped');
        bg.disabled=(d.state==='painting'||d.state==='paused');
        bs.textContent=(d.state==='paused')?'WZNOW':'START';

        /* Pattern buttons highlight + pending indicator */
        let curIdx=PAT_CODES.indexOf(d.pattern);
        let pendIdx=d.patternPending?PAT_CODES.indexOf(d.pendingPattern):-1;
        document.querySelectorAll('.pbtn').forEach(function(el){
            let pid=parseInt(el.getAttribute('data-pid'));
            if(pid===curIdx) el.classList.add('act');
            else el.classList.remove('act');
            if(pid===pendIdx) el.classList.add('pending');
            else el.classList.remove('pending');
        });

        /* Reverse button */
        let rb=document.getElementById('btnRev');
        if(REV_PATS.indexOf(curIdx)>=0){
            rb.disabled=false;
            if(d.reversed) rb.classList.add('act');
            else rb.classList.remove('act');
        } else {
            rb.disabled=true;
            rb.classList.remove('act');
        }

        /* Gun indicators + anomaly */
        for(let i=0;i<6;i++){
            let gc=document.getElementById('g'+i);
            if(d.guns[i]){gc.className='gun-circle on';}
            else if(d.gunAnomaly&&d.gunAnomaly[i]){gc.className='gun-circle off anom';}
            else{gc.className='gun-circle off';}
        }
        /* Gun anomaly warning */
        let anomEl=document.getElementById('anomWarn');
        if(d.gunAnomalyDetected){anomEl.style.display='block';}
        else{anomEl.style.display='none';}

        /* Calibration section */
        calibrating=d.calibrating;
        let cb=document.getElementById('calBtn');
        if(d.calibrating){
            cb.textContent='Zakoncz';
            cb.classList.add('active');
            document.getElementById('calPulseRow').style.display='block';
            document.getElementById('calPulses').textContent=d.calPulses;
        } else {
            cb.textContent='Rozpocznij';
            cb.classList.remove('active');
            document.getElementById('calPulseRow').style.display='none';
        }
        document.getElementById('calSt').textContent=d.calibrated?'Skalibrowany':'Domyslny';
        document.getElementById('calPpm').textContent=d.ppm;

        /* Speed alarm section */
        document.getElementById('spdMax').textContent=d.maxSpeed;
        if(!spdSliderLoaded){
            document.getElementById('spdSlider').value=parseFloat(d.maxSpeed);
            document.getElementById('spdSliderVal').textContent=d.maxSpeed+' km/h';
            spdSliderLoaded=true;
        }
        let spdWarnEl=document.getElementById('spdWarn');
        if(d.overspeed){spdWarnEl.style.display='inline';}
        else{spdWarnEl.style.display='none';}

        /* Mode selector */
        let modes=['mAuto','mSemi','mManual'];
        let mIdx={auto:0,semi:1,manual:2};
        let mi=mIdx[d.mode]||0;
        modes.forEach(function(id,i){
            let el=document.getElementById(id);
            if(i===mi)el.classList.add('act');
            else el.classList.remove('act');
        });
        document.getElementById('modeDesc').textContent=MODE_DESC[mi];
        /* Semi-auto next line button */
        let semiEl=document.getElementById('semiBtn');
        if(d.mode==='semi'&&d.state==='painting'&&d.semiLineComplete){
            semiEl.style.display='block';
        }else{semiEl.style.display='none';}
        /* Custom pattern use button */
        let cpBtn=document.getElementById('cpUseBtn');
        if(d.customValid){cpBtn.style.opacity='1';cpBtn.disabled=false;}
        else{cpBtn.style.opacity='.4';cpBtn.disabled=true;}

        /* System info */
        document.getElementById('sFw').textContent='v'+d.firmware;
        document.getElementById('sRam').textContent=Math.round(d.freeHeap/1024)+' KB';
        let u=d.uptime;
        let uh=Math.floor(u/3600),um=Math.floor((u%3600)/60),us=u%60;
        document.getElementById('sUp').textContent=uh+'h '+um+'m '+us+'s';
        document.getElementById('sCli').textContent=d.clients;

        /* Pattern preview canvas */
        drawPatPreview(d.patternIdx);

        /* Slot tabs sync */
        curSlot=d.activeSlot||0;
        for(let i=0;i<3;i++){
            let st=document.getElementById('slotTab'+i);
            if(i===curSlot)st.classList.add('act');else st.classList.remove('act');
            if(d.slotsValid&&d.slotsValid[i])st.style.opacity='1';else st.style.opacity='.5';
        }

        /* Switch mode sync */
        let isSmart=d.smartSwitch!==false;
        let swS=document.getElementById('swSmart'),swI=document.getElementById('swInst');
        if(swS&&swI){
            swS.style.background=isSmart?'#2ae67a':'';swS.style.color=isSmart?'#0a0e17':'';
            swI.style.background=!isSmart?'#e6a02a':'';swI.style.color=!isSmart?'#0a0e17':'';
            document.getElementById('swDesc').textContent=isSmart?'dokonczy cykl przed zmiana':'natychmiastowa zmiana wzorca';
        }

    }).catch(e=>console.error('Status error:',e));
}

/* ------- Service Menu Tabs ------- */
let activeTab=0;
let svcTick=0;
function svcTab(n){
    document.querySelectorAll('.svc-tab').forEach(function(t,i){t.classList.toggle('act',i===n);});
    document.getElementById('svcP0').style.display=n===0?'block':'none';
    document.getElementById('svcP1').style.display=n===1?'block':'none';
    activeTab=n;
    if(n===0)loadStats();
    if(n===1)loadReports();
}
function loadStats(){
    fetch('/api/stats').then(function(r){return r.json();}).then(function(d){
        document.getElementById('ltDist').textContent=d.lifetimeDistanceM+' m';
        document.getElementById('ltArea').innerHTML=d.lifetimeAreaM2+' m&sup2;';
        document.getElementById('ltTime').textContent=fmtTime(d.lifetimePaintTimeSec);
        let sdEl=document.getElementById('ltSd');
        sdEl.textContent=d.sdReady?'OK ('+d.reportCount+')':'BRAK';
        sdEl.style.color=d.sdReady?'':'#e64040';
        let row=document.getElementById('gunDistRow');
        let h='';
        for(let i=0;i<6;i++){
            h+='<div class="gun-item"><div class="gun-circle off" style="width:42px;height:42px;font-size:9px;">'+d.gunDistances[i]+'m</div><div class="gun-label">P'+(i+1)+'</div></div>';
        }
        row.innerHTML=h;
        /* Gun shot counts (lifetime) */
        let sRow=document.getElementById('gunShotRow');
        if(sRow&&d.gunShotCounts){
            let sh='';
            for(let i=0;i<6;i++){
                sh+='<div class="gun-item"><div class="gun-circle off" style="width:42px;height:42px;font-size:9px;">'+d.gunShotCounts[i]+'</div><div class="gun-label">P'+(i+1)+'</div></div>';
            }
            sRow.innerHTML=sh;
        }
    }).catch(function(e){console.error('Stats:',e);});
}
function loadReports(){
    let el=document.getElementById('repList');
    el.innerHTML='<span style="color:#6b7d9a;">Ladowanie...</span>';
    fetch('/api/reports').then(function(r){return r.json();}).then(function(d){
        if(d.length===0){el.innerHTML='Brak raportow na karcie SD.';return;}
        let h='<table class="rep-tbl"><tr><th>Plik</th><th>Rozmiar</th><th style="text-align:right">Pobierz</th></tr>';
        d.forEach(function(r){
            let sz=r.size>1024?(r.size/1024).toFixed(1)+' KB':r.size+' B';
            h+='<tr><td>'+r.file+'</td><td>'+sz+'</td>';
            h+='<td style="text-align:right"><a href="/api/reports/download?file='+encodeURIComponent(r.file)+'" style="color:#2ae67a;text-decoration:none;font-weight:bold;">CSV</a></td></tr>';
        });
        h+='</table>';
        el.innerHTML=h;
    }).catch(function(){el.innerHTML='Blad ladowania raportow.';});
}
loadStats();

/* Auto-refresh every 1 second + stats every 10s */
setInterval(function(){
    fetchStatus();
    svcTick++;
    if(svcTick%10===0&&activeTab===0)loadStats();
},1000);
fetchStatus();
</script>
</body>
</html>)rawhtml";

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
    if (!reportLogger.isReady() || !SD.exists(path.c_str())) {
        server.send(404, "text/plain", "Plik nie znaleziony");
        return;
    }

    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) {
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
    Serial.printf("[WWW] Pobranie raportu: %s\n", fname.c_str());
}

// buildHtmlPage() - nie uzywane, HTML wysylany chunkami z handleRoot()
String TrassarWebServer::buildHtmlPage() {
    return String();
}
