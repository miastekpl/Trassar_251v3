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
    } else {
        result = "nieznana akcja";
    }

    g_state.displayNeedsUpdate = true;
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

    // Stan maszyny
    const char* stateStr;
    switch (g_state.machineState) {
        case STATE_IDLE:     stateStr = "idle";     break;
        case STATE_PAINTING: stateStr = "painting"; break;
        case STATE_PAUSED:   stateStr = "paused";   break;
        case STATE_STOPPED:  stateStr = "stopped";  break;
        default:             stateStr = "unknown";   break;
    }
    doc["state"] = stateStr;

    // Wzorzec
    const PatternDef& pat = patternMgr.getCurrent();
    doc["pattern"] = pat.code;
    doc["patternName"] = pat.name;
    doc["reversed"] = g_state.patternReversed;
    doc["gapStart"] = paintEngine.isGapStart();

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

    // Oczekujaca zmiana wzorca (smart switch)
    doc["patternPending"] = paintEngine.isPatternChangePending();
    if (paintEngine.isPatternChangePending()) {
        doc["pendingPattern"] = PatternManager::patterns[paintEngine.getPendingPattern()].code;
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
    "P-7a","P-7b","P-7c","P-7d"
];
/* P-3a=7, P-3b=8 are reversible */
const REV_PATS=[7,8];

let calibrating=false;
let spdSliderLoaded=false;

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
/* Speed slider live update */
document.addEventListener('DOMContentLoaded',function(){
    let sl=document.getElementById('spdSlider');
    if(sl) sl.addEventListener('input',function(){
        document.getElementById('spdSliderVal').textContent=parseFloat(this.value).toFixed(1)+' km/h';
    });
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

        /* System info */
        document.getElementById('sFw').textContent='v'+d.firmware;
        document.getElementById('sRam').textContent=Math.round(d.freeHeap/1024)+' KB';
        let u=d.uptime;
        let uh=Math.floor(u/3600),um=Math.floor((u%3600)/60),us=u%60;
        document.getElementById('sUp').textContent=uh+'h '+um+'m '+us+'s';
        document.getElementById('sCli').textContent=d.clients;

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
    }).catch(function(e){console.error('Stats:',e);});
}
function loadReports(){
    let el=document.getElementById('repList');
    el.innerHTML='<span style="color:#6b7d9a;">Ladowanie...</span>';
    fetch('/api/reports').then(function(r){return r.json();}).then(function(d){
        if(d.length===0){el.innerHTML='Brak raportow na karcie SD.';return;}
        let h='<table class="rep-tbl"><tr><th>Plik</th><th style="text-align:right">Rozmiar</th></tr>';
        d.forEach(function(r){
            let sz=r.size>1024?(r.size/1024).toFixed(1)+' KB':r.size+' B';
            h+='<tr><td>'+r.file+'</td><td>'+sz+'</td></tr>';
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

// buildHtmlPage() - nie uzywane, HTML wysylany chunkami z handleRoot()
String TrassarWebServer::buildHtmlPage() {
    return String();
}
