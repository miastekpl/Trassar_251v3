// ============================================================
// TrassarV3 - Implementacja serwera WWW (WiFi AP)
// ============================================================

#include "web_server.h"
#include <ArduinoJson.h>

TrassarWebServer webServer;

void TrassarWebServer::begin() {
    // Uruchomienie Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CON);

    delay(100);
    Serial.print("[WiFi] AP uruchomiony. IP: ");
    Serial.println(WiFi.softAPIP());

    setupRoutes();
    server.begin();
    Serial.println("[WWW] Serwer HTTP uruchomiony na porcie 80");
}

void TrassarWebServer::update() {
    server.handleClient();
}

String TrassarWebServer::getIPAddress() {
    return WiFi.softAPIP().toString();
}

int TrassarWebServer::getConnectedClients() {
    return WiFi.softAPgetStationNum();
}

void TrassarWebServer::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/control", HTTP_POST, [this]() { handleControl(); });
    server.onNotFound([this]() { handleNotFound(); });
}

void TrassarWebServer::handleRoot() {
    server.send(200, "text/html", buildHtmlPage());
}

void TrassarWebServer::handleStatus() {
    server.send(200, "application/json", getStateJson());
}

void TrassarWebServer::handleControl() {
    if (!server.hasArg("action")) {
        server.send(400, "application/json", "{\"error\":\"brak parametru action\"}");
        return;
    }

    String action = server.arg("action");
    String result = "ok";

    if (action == "start") {
        if (g_state.machineState == STATE_IDLE || g_state.machineState == STATE_STOPPED) {
            g_state.machineState = STATE_RUNNING;
            g_state.paintStartTime = millis();
            g_state.paintElapsed = 0;
            g_state.totalPauseTime = 0;
            g_state.currentPass = 1;
            g_state.currentScreen = SCREEN_PAINTING;
        } else if (g_state.machineState == STATE_PAUSED) {
            g_state.machineState = STATE_RUNNING;
            g_state.totalPauseTime += millis() - g_state.pauseStartTime;
        }
    } else if (action == "pause") {
        if (g_state.machineState == STATE_RUNNING) {
            g_state.machineState = STATE_PAUSED;
            g_state.pauseStartTime = millis();
        }
    } else if (action == "stop") {
        if (g_state.machineState == STATE_RUNNING || g_state.machineState == STATE_PAUSED) {
            g_state.machineState = STATE_STOPPED;
            if (g_state.paintStartTime > 0) {
                g_state.paintElapsed = millis() - g_state.paintStartTime - g_state.totalPauseTime;
            }
        }
    } else if (action == "set_speed") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val >= 0 && val <= 100) {
                g_state.paintSpeed = val;
            }
        }
    } else if (action == "set_passes") {
        if (server.hasArg("value")) {
            int val = server.arg("value").toInt();
            if (val >= 1 && val <= 99) {
                g_state.paintPasses = val;
            }
        }
    } else {
        result = "nieznana akcja";
    }

    g_state.displayNeedsUpdate = true;
    server.send(200, "application/json", "{\"result\":\"" + result + "\"}");
}

void TrassarWebServer::handleNotFound() {
    server.send(404, "text/plain", "404 - Nie znaleziono");
}

String TrassarWebServer::getStateJson() {
    JsonDocument doc;

    const char* stateStr;
    switch (g_state.machineState) {
        case STATE_IDLE:    stateStr = "idle"; break;
        case STATE_RUNNING: stateStr = "running"; break;
        case STATE_PAUSED:  stateStr = "paused"; break;
        case STATE_STOPPED: stateStr = "stopped"; break;
        case STATE_ERROR:   stateStr = "error"; break;
        default:            stateStr = "unknown"; break;
    }

    doc["state"] = stateStr;
    doc["speed"] = g_state.paintSpeed;
    doc["passes"] = g_state.paintPasses;
    doc["currentPass"] = g_state.currentPass;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["firmware"] = FW_VERSION;
    doc["clients"] = WiFi.softAPgetStationNum();

    unsigned long elapsed = 0;
    if (g_state.machineState == STATE_RUNNING && g_state.paintStartTime > 0) {
        elapsed = millis() - g_state.paintStartTime - g_state.totalPauseTime;
    } else if (g_state.machineState == STATE_PAUSED && g_state.paintStartTime > 0) {
        elapsed = g_state.pauseStartTime - g_state.paintStartTime - g_state.totalPauseTime;
    } else {
        elapsed = g_state.paintElapsed;
    }
    doc["elapsed"] = elapsed / 1000;

    String output;
    serializeJson(doc, output);
    return output;
}

String TrassarWebServer::buildHtmlPage() {
    String html = R"rawhtml(
<!DOCTYPE html>
<html lang="pl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TrassarV3 - Panel sterowania</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: #0a0e17;
            color: #e0e6f0;
            min-height: 100vh;
        }
        .header {
            background: linear-gradient(135deg, #1a2332, #0d1520);
            border-bottom: 2px solid #2a7d4f;
            padding: 16px 20px;
            text-align: center;
        }
        .header h1 {
            font-size: 22px;
            color: #2ae67a;
            letter-spacing: 2px;
        }
        .header .sub {
            font-size: 12px;
            color: #6b7d9a;
            margin-top: 4px;
        }
        .container { max-width: 480px; margin: 0 auto; padding: 12px; }

        .status-card {
            background: #111927;
            border-radius: 12px;
            border: 1px solid #1e2d42;
            padding: 20px;
            margin-bottom: 12px;
            text-align: center;
        }
        .status-indicator {
            display: inline-block;
            width: 16px; height: 16px;
            border-radius: 50%;
            margin-right: 8px;
            vertical-align: middle;
            animation: pulse 2s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .status-text {
            font-size: 28px;
            font-weight: bold;
            margin: 8px 0;
        }
        .status-idle { color: #6b7d9a; }
        .status-idle .status-indicator { background: #6b7d9a; }
        .status-running { color: #2ae67a; }
        .status-running .status-indicator { background: #2ae67a; }
        .status-paused { color: #f0c040; }
        .status-paused .status-indicator { background: #f0c040; }
        .status-stopped { color: #e64040; }
        .status-stopped .status-indicator { background: #e64040; }

        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
            margin-top: 12px;
        }
        .info-item {
            background: #0d1520;
            border-radius: 8px;
            padding: 12px;
            text-align: center;
        }
        .info-item .label {
            font-size: 11px;
            color: #6b7d9a;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .info-item .value {
            font-size: 20px;
            font-weight: bold;
            color: #2ae67a;
            margin-top: 4px;
        }

        .controls {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 8px;
            margin-bottom: 12px;
        }
        .btn {
            padding: 14px 8px;
            border: none;
            border-radius: 10px;
            font-size: 14px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .btn:active { transform: scale(0.95); }
        .btn-start {
            background: linear-gradient(135deg, #1a8a4a, #2ae67a);
            color: #0a0e17;
        }
        .btn-pause {
            background: linear-gradient(135deg, #b08a20, #f0c040);
            color: #0a0e17;
        }
        .btn-stop {
            background: linear-gradient(135deg, #8a2020, #e64040);
            color: #fff;
        }
        .btn:disabled {
            opacity: 0.3;
            cursor: not-allowed;
            transform: none;
        }

        .settings-card {
            background: #111927;
            border-radius: 12px;
            border: 1px solid #1e2d42;
            padding: 16px;
            margin-bottom: 12px;
        }
        .settings-card h3 {
            font-size: 13px;
            color: #6b7d9a;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 12px;
        }
        .slider-row {
            display: flex;
            align-items: center;
            margin-bottom: 12px;
        }
        .slider-row label {
            width: 90px;
            font-size: 13px;
            color: #9eafc4;
        }
        .slider-row input[type=range] {
            flex: 1;
            margin: 0 10px;
            accent-color: #2ae67a;
        }
        .slider-row .val {
            width: 45px;
            text-align: right;
            font-weight: bold;
            color: #2ae67a;
        }
        .number-row {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .number-row label {
            width: 90px;
            font-size: 13px;
            color: #9eafc4;
        }
        .number-row button {
            width: 36px; height: 36px;
            border: 1px solid #2a7d4f;
            border-radius: 8px;
            background: #0d1520;
            color: #2ae67a;
            font-size: 18px;
            cursor: pointer;
        }
        .number-row .val {
            font-size: 18px;
            font-weight: bold;
            color: #2ae67a;
            min-width: 30px;
            text-align: center;
        }

        .progress-bar {
            width: 100%;
            height: 8px;
            background: #0d1520;
            border-radius: 4px;
            overflow: hidden;
            margin-top: 8px;
        }
        .progress-bar .fill {
            height: 100%;
            border-radius: 4px;
            transition: width 0.5s;
            background: linear-gradient(90deg, #1a8a4a, #2ae67a);
        }

        .footer {
            text-align: center;
            padding: 12px;
            color: #3a4a60;
            font-size: 11px;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>TRASSAR V3</h1>
        <div class="sub">Sterownik maszyny malarskiej</div>
    </div>

    <div class="container">
        <div class="status-card">
            <div id="statusWrap" class="status-idle">
                <span class="status-indicator"></span>
                <span id="statusLabel">Gotowy</span>
            </div>
            <div class="status-text" id="statusText">GOTOWY</div>
            <div id="elapsedWrap" style="color:#6b7d9a; font-size:13px; display:none;">
                Czas: <span id="elapsed">00:00</span>
            </div>

            <div class="info-grid">
                <div class="info-item">
                    <div class="label">Predkosc</div>
                    <div class="value" id="infoSpeed">50%</div>
                </div>
                <div class="info-item">
                    <div class="label">Przejazd</div>
                    <div class="value" id="infoPass">0/1</div>
                </div>
            </div>

            <div class="progress-bar" id="progressWrap" style="display:none; margin-top:12px;">
                <div class="fill" id="progressFill" style="width:0%"></div>
            </div>
        </div>

        <div class="controls">
            <button class="btn btn-start" id="btnStart" onclick="sendCmd('start')">START</button>
            <button class="btn btn-pause" id="btnPause" onclick="sendCmd('pause')">PAUZA</button>
            <button class="btn btn-stop" id="btnStop" onclick="sendCmd('stop')">STOP</button>
        </div>

        <div class="settings-card">
            <h3>Ustawienia malowania</h3>
            <div class="slider-row">
                <label>Predkosc:</label>
                <input type="range" id="speedSlider" min="0" max="100" value="50"
                       oninput="updateSpeed(this.value)">
                <span class="val" id="speedVal">50%</span>
            </div>
            <div class="number-row">
                <label>Przejazdy:</label>
                <button onclick="changePasses(-1)">-</button>
                <span class="val" id="passesVal">1</span>
                <button onclick="changePasses(1)">+</button>
            </div>
        </div>

        <div class="settings-card">
            <h3>Informacje systemowe</h3>
            <div style="font-size:13px; color:#6b7d9a; line-height:1.8;">
                Firmware: <span style="color:#2ae67a" id="infoFw">-</span><br>
                Wolna RAM: <span style="color:#2ae67a" id="infoRam">-</span><br>
                Uptime: <span style="color:#2ae67a" id="infoUptime">-</span><br>
                Klienci WiFi: <span style="color:#2ae67a" id="infoClients">-</span>
            </div>
        </div>
    </div>

    <div class="footer">
        TrassarV3 &copy; 2025 | Firmware v)rawhtml";

    html += FW_VERSION;
    html += R"rawhtml(
    </div>

    <script>
        let currentState = 'idle';
        let passes = 1;

        function sendCmd(action) {
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'action=' + action
            }).then(r => r.json()).then(d => { fetchStatus(); });
        }

        function updateSpeed(val) {
            document.getElementById('speedVal').textContent = val + '%';
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'action=set_speed&value=' + val
            });
        }

        function changePasses(delta) {
            passes += delta;
            if (passes < 1) passes = 1;
            if (passes > 99) passes = 99;
            document.getElementById('passesVal').textContent = passes;
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'action=set_passes&value=' + passes
            });
        }

        function formatTime(sec) {
            let h = Math.floor(sec / 3600);
            let m = Math.floor((sec % 3600) / 60);
            let s = sec % 60;
            if (h > 0) return h + ':' + String(m).padStart(2,'0') + ':' + String(s).padStart(2,'0');
            return String(m).padStart(2,'0') + ':' + String(s).padStart(2,'0');
        }

        function fetchStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(d => {
                    currentState = d.state;
                    passes = d.passes;

                    // Status
                    let wrap = document.getElementById('statusWrap');
                    let stText = document.getElementById('statusText');
                    let stLabel = document.getElementById('statusLabel');
                    wrap.className = 'status-' + d.state;

                    let labels = {idle:'Gotowy', running:'Malowanie', paused:'Pauza', stopped:'Zatrzymany', error:'Blad'};
                    let upper = {idle:'GOTOWY', running:'MALOWANIE', paused:'PAUZA', stopped:'ZATRZYMANY', error:'BLAD'};
                    stLabel.textContent = labels[d.state] || d.state;
                    stText.textContent = upper[d.state] || d.state.toUpperCase();

                    // Info
                    document.getElementById('infoSpeed').textContent = d.speed + '%';
                    document.getElementById('infoPass').textContent = d.currentPass + '/' + d.passes;

                    // Elapsed
                    let elWrap = document.getElementById('elapsedWrap');
                    let progWrap = document.getElementById('progressWrap');
                    if (d.state === 'running' || d.state === 'paused') {
                        elWrap.style.display = 'block';
                        document.getElementById('elapsed').textContent = formatTime(d.elapsed);
                        progWrap.style.display = 'block';
                        let pct = d.passes > 0 ? (d.currentPass / d.passes * 100) : 0;
                        document.getElementById('progressFill').style.width = pct + '%';
                    } else {
                        elWrap.style.display = d.elapsed > 0 ? 'block' : 'none';
                        if (d.elapsed > 0) document.getElementById('elapsed').textContent = formatTime(d.elapsed);
                        progWrap.style.display = 'none';
                    }

                    // Slider sync
                    document.getElementById('speedSlider').value = d.speed;
                    document.getElementById('speedVal').textContent = d.speed + '%';
                    document.getElementById('passesVal').textContent = d.passes;

                    // Buttons
                    let btnStart = document.getElementById('btnStart');
                    let btnPause = document.getElementById('btnPause');
                    let btnStop = document.getElementById('btnStop');

                    btnStart.disabled = (d.state === 'running');
                    btnPause.disabled = (d.state !== 'running');
                    btnStop.disabled = (d.state === 'idle' || d.state === 'stopped');

                    if (d.state === 'paused') {
                        btnStart.textContent = 'WZNOW';
                    } else {
                        btnStart.textContent = 'START';
                    }

                    // System info
                    document.getElementById('infoFw').textContent = 'v' + d.firmware;
                    document.getElementById('infoRam').textContent = Math.round(d.freeHeap/1024) + ' KB';
                    let up = d.uptime;
                    let uh = Math.floor(up/3600);
                    let um = Math.floor((up%3600)/60);
                    document.getElementById('infoUptime').textContent = uh + 'h ' + um + 'm ' + (up%60) + 's';
                    document.getElementById('infoClients').textContent = d.clients;
                })
                .catch(e => console.error('Blad pobierania statusu:', e));
        }

        // Auto-refresh co 1 sekundę
        setInterval(fetchStatus, 1000);
        fetchStatus();
    </script>
</body>
</html>
)rawhtml";

    return html;
}
