#pragma once
// ============================================================
// TrassarV3 - HTML/CSS/JS panelu sterowania (PROGMEM)
// Wydzielone z web_server.cpp dla czytelnosci
// Strona wysylana chunkami: HTML_PART1 + FW_VERSION + HTML_PART2
// ============================================================

#include <Arduino.h>

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

/* ---------- TFT PREVIEW ---------- */
.tft-wrap{
    background:#000;border-radius:8px;border:2px solid #2a3a50;
    padding:0;margin-bottom:12px;overflow:hidden;
    position:relative;
}
.tft-wrap canvas{display:block;width:100%;height:auto;image-rendering:pixelated;}
.tft-label{
    position:absolute;top:4px;right:6px;font-size:9px;color:#3a5a3a;
    pointer-events:none;letter-spacing:1px;
}

/* ---------- SCREEN NAV ---------- */
.scr-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px;}
.scr-btn{
    padding:10px 6px;border:1px solid #1e2d42;border-radius:8px;
    background:#0d1520;color:#9eafc4;font-size:12px;font-weight:600;
    cursor:pointer;text-align:center;transition:all .15s;
}
.scr-btn:active{transform:scale(.95);}
.scr-btn.act{background:#1a3a5a;border-color:#2a7d9f;color:#fff;}
.scr-btn .scr-ico{font-size:16px;display:block;margin-bottom:2px;}

/* ---------- VIRTUAL BUTTONS ---------- */
.vbtn-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;}
.vbtn{
    padding:14px 6px;border:1px solid #1e2d42;border-radius:10px;
    background:#0d1520;color:#9eafc4;font-size:12px;font-weight:700;
    cursor:pointer;text-align:center;transition:all .15s;
    text-transform:uppercase;letter-spacing:.5px;
}
.vbtn:active{transform:scale(.93);background:#1a2a40;}
.vbtn .vico{font-size:18px;display:block;margin-bottom:2px;}
.vbtn-sel{border-color:#2a7d9f;color:#2ae67a;}
.vbtn-stop{border-color:#8a2020;color:#e64040;}
.vbtn-start{border-color:#1a8a4a;color:#2ae67a;}

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
        <h3>Progi predkosci</h3>
        <div class="cal-row">
            <div class="cal-info">
                Min: <span id="spdMin" style="color:#f0c040;">3.0</span> km/h &nbsp;|&nbsp;
                Maks: <span id="spdMax" style="color:#e64040;">15.0</span> km/h<br>
                <span id="spdWarn" style="display:none;color:#e64040;font-weight:bold;">PRZEKROCZENIE!</span>
                <span id="spdLowWarn" style="display:none;color:#f0c040;font-weight:bold;">NISKA PREDKOSC</span>
            </div>
        </div>
        <div style="margin-top:8px;font-size:11px;color:#6b7d9a;">Min. predkosc (pistolety OFF ponizej):</div>
        <div style="margin-top:4px;display:flex;align-items:center;gap:8px;flex-wrap:wrap;">
            <input type="range" id="minSpdSlider" min="0" max="10" step="0.5" value="3"
                style="flex:1;min-width:120px;accent-color:#f0c040;">
            <span id="minSpdSliderVal" style="font-size:14px;font-weight:bold;color:#f0c040;min-width:60px;">3.0 km/h</span>
            <button class="cal-btn" onclick="setMinSpeed()">Zapisz</button>
        </div>
        <div style="margin-top:8px;font-size:11px;color:#6b7d9a;">Maks. predkosc (alarm przekroczenia):</div>
        <div style="margin-top:4px;display:flex;align-items:center;gap:8px;flex-wrap:wrap;">
            <input type="range" id="spdSlider" min="5" max="30" step="0.5" value="15"
                style="flex:1;min-width:120px;accent-color:#2ae67a;">
            <span id="spdSliderVal" style="font-size:14px;font-weight:bold;color:#2ae67a;min-width:60px;">15.0 km/h</span>
            <button class="cal-btn" onclick="setMaxSpeed()">Zapisz</button>
        </div>
    </div>

    <!-- ========== GPS ========== -->
    <div class="card">
        <h3>GPS</h3>
        <div class="info-grid two">
            <div class="info-item">
                <div class="lbl">Fix / Satelity</div>
                <div class="val" id="gpsFix">---</div>
            </div>
            <div class="info-item">
                <div class="lbl">HDOP</div>
                <div class="val" id="gpsHdop">---</div>
            </div>
        </div>
        <div class="info-grid two" style="margin-top:6px;">
            <div class="info-item">
                <div class="lbl">Pozycja</div>
                <div class="val" id="gpsPos" style="font-size:11px;">---</div>
            </div>
            <div class="info-item">
                <div class="lbl">Predkosc GPS</div>
                <div class="val" id="gpsSpd">---</div>
            </div>
        </div>
    </div>

    <!-- ========== TFT SCREEN PREVIEW ========== -->
    <div class="card">
        <h3>Podglad ekranu TFT</h3>
        <div class="tft-wrap">
            <canvas id="tftCvs" width="320" height="240"></canvas>
            <span class="tft-label">ILI9341</span>
        </div>
    </div>

    <!-- ========== VIRTUAL BUTTONS ========== -->
    <div class="card">
        <h3>Przyciski wirtualne</h3>
        <div style="font-size:10px;color:#6b7d9a;margin-bottom:8px;">Sterowanie menu i ekranami serwisowymi</div>
        <div class="vbtn-grid">
            <div class="vbtn vbtn-start" onclick="sendEvt(1)"><span class="vico">&#9654;</span>START</div>
            <div class="vbtn vbtn-sel" onclick="sendEvt(6)"><span class="vico">&#9660;</span>SEL</div>
            <div class="vbtn vbtn-stop" onclick="sendEvt(4)"><span class="vico">&#9632;</span>STOP</div>
        </div>
        <div class="vbtn-grid" style="margin-top:6px;">
            <div class="vbtn vbtn-start" onclick="sendEvt(2)"><span class="vico">&#9654;&#9654;</span>START(1s)</div>
            <div class="vbtn vbtn-sel" onclick="sendEvt(7)"><span class="vico">&#10004;</span>SEL(1s)</div>
            <div class="vbtn vbtn-stop" onclick="sendEvt(5)"><span class="vico">&#9194;</span>STOP(1s)</div>
        </div>
    </div>

    <!-- ========== SCREEN NAVIGATION ========== -->
    <div class="card">
        <h3>Nawigacja ekranow TFT</h3>
        <div style="font-size:11px;color:#6b7d9a;margin-bottom:8px;">Aktualny ekran: <span id="scrCur" style="color:#2ae67a;font-weight:bold;">---</span></div>
        <div style="font-size:10px;color:#4a6080;margin-bottom:6px;">Glowne</div>
        <div class="scr-grid">
            <div class="scr-btn" data-scr="0" onclick="setScreen(0)"><span class="scr-ico">&#8962;</span>Ekran glowny</div>
            <div class="scr-btn" data-scr="7" onclick="setScreen(7)"><span class="scr-ico">&#9881;</span>Przygotowanie</div>
            <div class="scr-btn" data-scr="1" onclick="setScreen(1)"><span class="scr-ico">&#9654;</span>Malowanie</div>
            <div class="scr-btn" data-scr="10" onclick="setScreen(10)"><span class="scr-ico">&#9745;</span>Podsumowanie</div>
        </div>
        <div style="font-size:10px;color:#4a6080;margin:10px 0 6px;">Menu serwisowe</div>
        <div class="scr-grid">
            <div class="scr-btn" data-scr="2" onclick="setScreen(2)"><span class="scr-ico">&#9776;</span>Menu serwisowe</div>
            <div class="scr-btn" data-scr="3" onclick="setScreen(3)"><span class="scr-ico">&#8982;</span>Kalibracja</div>
            <div class="scr-btn" data-scr="4" onclick="setScreen(4)"><span class="scr-ico">&#8644;</span>Pomiar dystansu</div>
            <div class="scr-btn" data-scr="5" onclick="setScreen(5)"><span class="scr-ico">&#128196;</span>Raporty</div>
            <div class="scr-btn" data-scr="6" onclick="setScreen(6)"><span class="scr-ico">&#128167;</span>Czyszcz. dysz</div>
            <div class="scr-btn" data-scr="11" onclick="setScreen(11)"><span class="scr-ico">&#128202;</span>Statystyki LT</div>
            <div class="scr-btn" data-scr="12" onclick="setScreen(12)"><span class="scr-ico">&#9998;</span>Wzorzec wlasny</div>
            <div class="scr-btn" data-scr="13" onclick="setScreen(13)"><span class="scr-ico">&#128190;</span>Eksport stat.</div>
        </div>
        <div style="font-size:10px;color:#4a6080;margin:10px 0 6px;">Resety</div>
        <div class="scr-grid">
            <div class="scr-btn" data-scr="8" onclick="setScreen(8)"><span class="scr-ico">&#8634;</span>Reset sesji</div>
            <div class="scr-btn" data-scr="9" onclick="setScreen(9)"><span class="scr-ico">&#9888;</span>Reset licznikow</div>
        </div>
    </div>

    <!-- ========== SYSTEM INFO ========== -->
    <div class="card">
        <h3>Informacje systemowe</h3>
        <div class="sys-info">
            Firmware: <span id="sFw">---</span><br>
            Wolna RAM: <span id="sRam">---</span><br>
            Uptime: <span id="sUp">---</span><br>
            Klienci WiFi: <span id="sCli">---</span><br>
            WebSocket: <span id="sWs" style="color:#e64040;">---</span>
        </div>
    </div>

    <!-- ========== SERVICE MENU ========== -->
    <div class="card">
        <h3>Menu serwisowe</h3>
        <div class="svc-tabs">
            <button class="svc-tab act" onclick="svcTab(0)">Statystyki</button>
            <button class="svc-tab" onclick="svcTab(1)">Raporty SD</button>
            <button class="svc-tab" onclick="svcTab(2)">Trasy GPS</button>
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
        <div id="svcP2" style="display:none;">
            <div id="trkList" style="font-size:12px;color:#6b7d9a;">Ladowanie...</div>
            <button class="cal-btn" style="margin-top:8px;" onclick="loadTracks()">Odswiez</button>
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
function setMinSpeed(){
    let val=document.getElementById('minSpdSlider').value;
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=set_min_speed&value='+val
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
/* ------- Screen navigation ------- */
const SCR_NAMES=['Ekran glowny','Malowanie','Menu serwisowe','Kalibracja',
    'Pomiar dystansu','Raporty','Czyszcz. dysz','Przygotowanie',
    'Reset sesji','Reset licznikow','Podsumowanie','Statystyki LT',
    'Wzorzec wlasny','Eksport stat.'];
function setScreen(id){
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=set_screen&value='+id
    }).then(r=>r.json()).then(()=>fetchStatus());
}
function sendEvt(id){
    fetch('/api/control',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'action=send_event&value='+id
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
/* Speed sliders live update */
document.addEventListener('DOMContentLoaded',function(){
    let sl=document.getElementById('spdSlider');
    if(sl) sl.addEventListener('input',function(){
        document.getElementById('spdSliderVal').textContent=parseFloat(this.value).toFixed(1)+' km/h';
    });
    let msl=document.getElementById('minSpdSlider');
    if(msl) msl.addEventListener('input',function(){
        document.getElementById('minSpdSliderVal').textContent=parseFloat(this.value).toFixed(1)+' km/h';
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

/* ============================================= */
/* TFT Screen Preview - Canvas 320x240          */
/* Mirrors physical ILI9341 display              */
/* ============================================= */
const TFT_W=320,TFT_H=240;
const TFT_BG='#000010',TFT_TEXT='#e0e0e0',TFT_ACCENT='#2ae67a',TFT_WARN='#f0c040',TFT_ERR='#e64040';
const TFT_MENU='#8090a0',TFT_DIV='#1a2a3a',TFT_SEL='#1a2a40';
const TFT_GUN_ON='#20c050',TFT_GUN_OFF='#1a2030';
const TFT_GUN_SHORT=['P1','P2','P3','P4','P5','P6'];

function tftClear(ctx){ctx.fillStyle=TFT_BG;ctx.fillRect(0,0,TFT_W,TFT_H);}
function tftHeader(ctx,title){
    ctx.fillStyle='#0a1020';ctx.fillRect(0,0,TFT_W,22);
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 14px monospace';
    ctx.textAlign='center';ctx.fillText(title,TFT_W/2,16);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(0,22,TFT_W,1);ctx.textAlign='left';
}
function tftHint(ctx,txt){
    ctx.fillStyle=TFT_MENU;ctx.font='9px monospace';
    ctx.textAlign='left';ctx.fillText(txt,6,TFT_H-6);
}
function tftGunRects(ctx,y,gunsCfg,gunStates,paused){
    let gw=44,gh=20,gap=6,total=6*gw+5*gap;
    let x0=(TFT_W-total)/2;
    for(let i=0;i<6;i++){
        let x=x0+i*(gw+gap);
        let inPat=gunsCfg&&gunsCfg[i]&&gunsCfg[i].mode!==0;
        let firing=gunStates&&gunStates[i];
        let bg;
        if(firing) bg=TFT_GUN_ON;
        else if(paused&&inPat) bg=TFT_WARN;
        else if(inPat) bg=TFT_WARN;
        else bg='#2a2a3a';
        ctx.fillStyle=bg;ctx.fillRect(x,y,gw,gh);
        ctx.strokeStyle=TFT_DIV;ctx.strokeRect(x,y,gw,gh);
        ctx.fillStyle=firing?'#fff':TFT_MENU;ctx.font='bold 10px monospace';
        ctx.textAlign='center';ctx.fillText(TFT_GUN_SHORT[i],x+gw/2,y+14);
    }
    ctx.textAlign='left';
}
function tftPatViz(ctx,vx,vy,vw,vh,patIdx,reversed){
    let guns=PAT_DEFS[patIdx];
    if(!guns||guns.length===0){ctx.fillStyle=TFT_BG;ctx.fillRect(vx,vy,vw,vh);return;}
    ctx.fillStyle=TFT_BG;ctx.fillRect(vx,vy,vw,vh);
    let lblH=14,colH=vh-lblH,colY=vy+lblH;
    let colWidths=[];let totalW=0;
    for(let a=0;a<guns.length;a++){
        let gn=guns[a][0];
        let w=(gn==='P4'||gn==='P6')?22:14;
        colWidths.push(w);totalW+=w;
    }
    totalW+=(guns.length-1)*4;
    let cx=vx+(vw-totalW)/2;
    ctx.font='9px monospace';
    for(let a=0;a<guns.length;a++){
        let gn=guns[a][0],ln=guns[a][2],gp=guns[a][3],cw=colWidths[a];
        ctx.fillStyle=TFT_MENU;ctx.textAlign='center';
        ctx.fillText(gn,cx+cw/2,vy+10);
        if(ln<=0&&gp<=0){
            ctx.fillStyle=TFT_GUN_ON;ctx.fillRect(cx,colY,cw,colH);
        } else {
            let cycle=ln+gp;if(cycle<=0){cx+=cw+4;continue;}
            ctx.fillStyle=TFT_GUN_OFF;ctx.fillRect(cx,colY,cw,colH);
            let nCyc=cycle<=1.5?4:cycle<=3?3:2;
            let totL=cycle*nCyc,sc=colH/totL,pos=0;
            while(pos<totL){
                let y1=colY+Math.floor(pos*sc);
                let y2=colY+Math.floor((pos+ln)*sc);
                if(y1>=colY+colH)break;if(y2>colY+colH)y2=colY+colH;
                ctx.fillStyle=TFT_GUN_ON;ctx.fillRect(cx,y1,cw,y2-y1);
                pos+=cycle;
            }
        }
        ctx.strokeStyle=TFT_DIV;ctx.strokeRect(cx,colY,cw,colH);
        cx+=cw+4;
    }
    ctx.textAlign='left';
}
/* Build gun config from PAT_DEFS for gun rects */
function tftGunCfgFromPat(patIdx){
    let cfg=[];
    for(let i=0;i<6;i++) cfg.push({mode:0,lineLen:0,gapLen:0});
    let guns=PAT_DEFS[patIdx];if(!guns)return cfg;
    for(let a=0;a<guns.length;a++){
        let gn=guns[a][0],ln=guns[a][2],gp=guns[a][3];
        let gi=parseInt(gn.replace('P',''))-1;
        if(gi<0||gi>=6)continue;
        if(ln<=0&&gp<=0) cfg[gi].mode=1;/*continuous*/
        else{cfg[gi].mode=2;cfg[gi].lineLen=ln;cfg[gi].gapLen=gp;}
    }
    return cfg;
}

/* ---- Screen 0: HOME ---- */
function tftDrawHome(ctx,d){
    tftClear(ctx);
    let pi=d.patternIdx||0;
    /* Left column: pattern code */
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 26px sans-serif';
    ctx.textAlign='left';ctx.fillText(d.pattern||'P-1a',6,32);
    /* Pattern name */
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';
    ctx.fillText(d.patternName||'',6,50);
    /* [ODW] flag */
    if(d.reversed){ctx.fillStyle=TFT_WARN;ctx.font='11px sans-serif';ctx.fillText('[ODW]',6,66);}
    /* Status */
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 11px sans-serif';ctx.fillText('Gotowy',6,84);
    /* Mode */
    let mTxt={auto:'AUTO',semi:'SEMI-AUTO',manual:'RECZNY'};
    ctx.fillStyle=TFT_MENU;ctx.font='10px monospace';ctx.fillText(mTxt[d.mode]||'AUTO',6,100);

    /* Center: pattern visualization */
    tftPatViz(ctx,100,4,120,178,pi,d.reversed);

    /* Right column: speed */
    ctx.fillStyle=TFT_TEXT;ctx.font='bold 26px sans-serif';ctx.textAlign='right';
    ctx.fillText(d.speed||'0.0',TFT_W-6,32);
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';ctx.fillText('km/h',TFT_W-6,48);
    /* Area */
    ctx.fillStyle=TFT_TEXT;ctx.font='bold 14px sans-serif';
    ctx.fillText((d.area||'0.00')+' m2',TFT_W-6,72);
    ctx.textAlign='left';

    /* Bottom: gun rects */
    let cfg=tftGunCfgFromPat(pi);
    tftGunRects(ctx,196,cfg,null,false);
}

/* ---- Screen 1: PAINTING ---- */
function tftDrawPainting(ctx,d){
    tftClear(ctx);
    let pi=d.patternIdx||0;
    let paused=d.state==='paused';
    /* Left column: pattern code */
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 26px sans-serif';
    ctx.textAlign='left';ctx.fillText(d.pattern||'P-1a',6,32);
    /* Flags */
    let fy=50;
    if(d.gapStart){ctx.fillStyle=TFT_WARN;ctx.font='11px sans-serif';ctx.fillText('[GAP]',6,fy);fy+=16;}
    if(d.reversed){ctx.fillStyle=TFT_WARN;ctx.font='11px sans-serif';ctx.fillText('[ODW]',6,fy);fy+=16;}
    /* State */
    let stC={painting:TFT_ACCENT,paused:TFT_WARN,stopped:TFT_ERR};
    let stT={painting:'Malowanie',paused:'Pauza',stopped:'Zatrzymany'};
    ctx.fillStyle=stC[d.state]||TFT_MENU;ctx.font='bold 11px sans-serif';
    ctx.fillText(stT[d.state]||d.state,6,fy+14);
    /* Session time */
    ctx.fillStyle=TFT_MENU;ctx.font='10px monospace';
    ctx.fillText(fmtTime(d.elapsed||0),6,fy+30);
    /* Session distance */
    let dist=parseFloat(d.distance)||0;
    let distTxt=dist>=1000?(dist/1000).toFixed(2)+' km':dist.toFixed(1)+' m';
    ctx.fillStyle=TFT_TEXT;ctx.font='11px sans-serif';
    ctx.fillText(distTxt,6,fy+46);
    /* Mode */
    let mTxt={auto:'AUTO',semi:'SEMI-AUTO',manual:'RECZNY'};
    ctx.fillStyle=TFT_MENU;ctx.font='10px monospace';ctx.fillText(mTxt[d.mode]||'AUTO',6,fy+62);

    /* Center: pattern visualization */
    tftPatViz(ctx,100,4,120,178,pi,d.reversed);

    /* Right column: speed */
    let spd=parseFloat(d.speed)||0;
    let spdColor=TFT_TEXT;
    if(d.overspeed) spdColor=TFT_ERR;
    else if(d.lowSpeed) spdColor=TFT_WARN;
    ctx.fillStyle=spdColor;ctx.font='bold 26px sans-serif';ctx.textAlign='right';
    ctx.fillText(d.speed||'0.0',TFT_W-6,32);
    ctx.fillStyle=d.overspeed?spdColor:TFT_MENU;ctx.font='11px sans-serif';
    ctx.fillText('km/h',TFT_W-6,48);
    /* Area */
    ctx.fillStyle=TFT_TEXT;ctx.font='bold 14px sans-serif';
    ctx.fillText((d.area||'0.00')+' m2',TFT_W-6,72);
    ctx.textAlign='left';

    /* Bottom: gun rects */
    let cfg=tftGunCfgFromPat(pi);
    let gs=d.guns||[0,0,0,0,0,0];
    tftGunRects(ctx,196,cfg,gs,paused);
}

/* ---- Screen 2: SERVICE MENU ---- */
function tftDrawServiceMenu(ctx,d){
    tftClear(ctx);tftHeader(ctx,'SERWIS');
    let items=['Kalibracja enkodera','Pomiar dystansu','Raporty',
               'Czyszczenie dysz','Statystyki lifetime','Wzorzec wlasny',
               'Eksport statystyk','Reset etapu','Reset licznikow'];
    let mi=d.menuIndex||0;
    ctx.font='12px sans-serif';
    let rowH=23,y0=28;
    for(let i=0;i<items.length;i++){
        let iy=y0+i*rowH;
        if(i===mi){ctx.fillStyle=TFT_SEL;ctx.fillRect(0,iy,TFT_W,rowH);
            ctx.fillStyle=TFT_ACCENT;ctx.fillText('\u25B6',8,iy+16);}
        else{ctx.fillStyle=TFT_MENU;}
        ctx.fillStyle=i===mi?TFT_ACCENT:TFT_MENU;
        ctx.font=i===mi?'bold 12px sans-serif':'12px sans-serif';
        ctx.fillText(items[i],24,iy+16);
        ctx.fillStyle=TFT_DIV;ctx.fillRect(0,iy+rowH-1,TFT_W,1);
    }
    tftHint(ctx,'SEL=dalej STOP=cofnij SEL(1s)=wejdz');
}

/* ---- Screen 3: CALIBRATION ---- */
function tftDrawCalibration(ctx,d){
    tftClear(ctx);tftHeader(ctx,'KALIBRACJA ENKODERA');
    ctx.fillStyle=TFT_TEXT;ctx.font='11px sans-serif';
    ctx.fillText('Odmierz 10 m i przejdz maszyna po prostej.',12,44);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,56,TFT_W-24,1);
    /* Status */
    ctx.textAlign='center';ctx.font='bold 18px sans-serif';
    if(d.calibrating){ctx.fillStyle=TFT_WARN;ctx.fillText('POMIAR...',TFT_W/2,90);}
    else{ctx.fillStyle=TFT_MENU;ctx.fillText('GOTOWY',TFT_W/2,90);}
    ctx.textAlign='left';
    /* Pulses */
    if(d.calibrating){
        ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';ctx.fillText('Impulsy:',12,120);
        ctx.fillStyle=TFT_ACCENT;ctx.textAlign='right';
        ctx.fillText(d.calPulses||'0',TFT_W-12,120);ctx.textAlign='left';
    }
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,138,TFT_W-24,1);
    /* PPM + status */
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';ctx.fillText('Imp/metr:',12,154);
    ctx.fillStyle=TFT_TEXT;ctx.fillText(d.ppm||'100.0',110,154);
    ctx.fillStyle=TFT_MENU;ctx.fillText('Status:',190,154);
    if(d.calibrated){ctx.fillStyle=TFT_ACCENT;ctx.fillText('OK',260,154);}
    else{ctx.fillStyle=TFT_WARN;ctx.fillText('Domyslny',260,154);}
    tftHint(ctx,d.calibrating?'START=zakoncz  STOP(1s)=powrot':'START=rozpocznij  STOP(1s)=powrot');
}

/* ---- Screen 4: DISTANCE METER ---- */
function tftDrawDistMeter(ctx,d){
    tftClear(ctx);tftHeader(ctx,'POMIAR DYSTANSU');
    ctx.textAlign='center';ctx.font='bold 12px sans-serif';
    ctx.fillStyle=TFT_MENU;ctx.fillText('GOTOWY',TFT_W/2,46);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(20,56,TFT_W-40,1);
    ctx.font='bold 22px sans-serif';ctx.fillStyle=TFT_TEXT;
    ctx.fillText('0.00 m',TFT_W/2,90);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(20,108,TFT_W-40,1);
    ctx.font='11px sans-serif';ctx.fillStyle=TFT_MENU;
    ctx.fillText('= 0 cm',TFT_W/2,126);
    ctx.textAlign='left';
    tftHint(ctx,'START=pomiar STOP=reset STOP(1s)=powrot');
}

/* ---- Screen 5: REPORTS ---- */
function tftDrawReports(ctx,d){
    tftClear(ctx);tftHeader(ctx,'RAPORTY');
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';ctx.fillText('SD:',12,44);
    ctx.fillStyle=TFT_ACCENT;ctx.fillText('OK',38,44);
    ctx.fillStyle=TFT_MENU;ctx.fillText('Plikow: ---',100,44);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,56,TFT_W-24,1);
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 11px sans-serif';ctx.fillText('Ostatni wpis:',12,72);
    ctx.fillStyle=TFT_MENU;ctx.font='10px monospace';ctx.fillText('Brak wpisow',12,92);
    tftHint(ctx,'STOP(1s)=powrot');
}

/* ---- Screen 6: NOZZLE CLEAN ---- */
function tftDrawNozzleClean(ctx,d){
    tftClear(ctx);tftHeader(ctx,'CZYSZCZENIE DYSZ');
    let pi=d.patternIdx||0;
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 18px sans-serif';ctx.fillText(d.pattern||'P-1a',8,54);
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';ctx.fillText(d.patternName||'',8,74);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(4,86,152,1);
    ctx.fillStyle=TFT_MENU;ctx.font='9px monospace';
    ctx.fillText('SEL=wzorzec',6,100);ctx.fillText('TRZYMAJ START',6,116);ctx.fillText('STOP(1s)=powrot',6,132);
    tftPatViz(ctx,164,28,152,136,pi,false);
    let cfg=tftGunCfgFromPat(pi);
    let gs=d.guns||[0,0,0,0,0,0];
    tftGunRects(ctx,196,cfg,gs,false);
}

/* ---- Screen 7: SETUP ---- */
function tftDrawSetup(ctx,d){
    tftClear(ctx);tftHeader(ctx,'PRZYGOTOWANIE');
    let labels=['Tryb pracy:','Przelaczanie:','Start:'];
    let mTxt={auto:'AUTO',semi:'SEMI-AUTO',manual:'RECZNY'};
    let vals=[mTxt[d.mode]||'AUTO',d.smartSwitch!==false?'Smart':'Instant',d.gapStart?'Od przerwy':'Normalny'];
    let colors=[TFT_ACCENT,d.smartSwitch!==false?TFT_ACCENT:TFT_WARN,d.gapStart?TFT_WARN:TFT_ACCENT];
    for(let i=0;i<3;i++){
        let iy=36+i*48;
        ctx.fillStyle=TFT_MENU;ctx.font='12px sans-serif';ctx.textAlign='left';
        ctx.fillText(labels[i],24,iy+30);
        ctx.fillStyle=colors[i];ctx.font='bold 14px sans-serif';ctx.textAlign='right';
        ctx.fillText(vals[i],TFT_W-24,iy+30);
        ctx.fillStyle=TFT_DIV;ctx.fillRect(0,iy+47,TFT_W,1);
    }
    ctx.textAlign='left';
    tftHint(ctx,'SEL=dalej SEL(1s)=zmien START=maluj');
}

/* ---- Screen 8: SESSION RESET ---- */
function tftDrawSessionReset(ctx,d){
    tftClear(ctx);tftHeader(ctx,'RESET ETAPU');
    ctx.fillStyle=TFT_TEXT;ctx.font='11px sans-serif';ctx.fillText('Biezacy etap pracy:',14,44);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,54,TFT_W-24,1);
    let dist=parseFloat(d.distance)||0;
    let distTxt=dist>=1000?(dist/1000).toFixed(2)+' km':dist.toFixed(1)+' m';
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';
    ctx.fillText('Dystans:',14,74);ctx.fillStyle=TFT_TEXT;ctx.fillText(distTxt,110,74);
    ctx.fillStyle=TFT_MENU;ctx.fillText('Powierzchnia:',14,94);ctx.fillStyle=TFT_TEXT;ctx.fillText((d.area||'0.00')+' m2',130,94);
    ctx.fillStyle=TFT_MENU;ctx.fillText('Czas:',14,114);ctx.fillStyle=TFT_TEXT;ctx.fillText(fmtTime(d.elapsed||0),110,114);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,128,TFT_W-24,1);
    ctx.fillStyle=TFT_WARN;ctx.font='bold 12px sans-serif';ctx.textAlign='center';
    ctx.fillText('Wyzerowac liczniki sesji?',TFT_W/2,150);ctx.textAlign='left';
    tftHint(ctx,'START = TAK    STOP = NIE');
}

/* ---- Screen 9: COUNTER RESET ---- */
function tftDrawCounterReset(ctx,d){
    tftClear(ctx);tftHeader(ctx,'RESET LICZNIKOW');
    ctx.fillStyle=TFT_ERR;ctx.font='bold 11px sans-serif';
    ctx.fillText('UWAGA! Zerowanie WSZYSTKICH licznikow.',8,42);
    ctx.fillStyle=TFT_ACCENT;ctx.font='11px sans-serif';
    ctx.fillText('Kalibracja NIE zostanie zmieniona.',8,60);
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,68,TFT_W-24,1);
    ctx.fillStyle=TFT_WARN;ctx.font='bold 12px sans-serif';ctx.textAlign='center';
    ctx.fillText('Wyzerowac wszystkie liczniki?',TFT_W/2,130);ctx.textAlign='left';
    tftHint(ctx,'START = TAK    STOP = NIE');
}

/* ---- Screen 10: SUMMARY ---- */
function tftDrawSummary(ctx,d){
    tftClear(ctx);tftHeader(ctx,'PODSUMOWANIE ETAPU');
    /* Pattern */
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 14px sans-serif';ctx.textAlign='center';
    ctx.fillText(d.pattern||'P-1a',TFT_W/2,44);ctx.textAlign='left';
    ctx.fillStyle=TFT_DIV;ctx.fillRect(12,52,TFT_W-24,1);
    let y=64;
    let dist=parseFloat(d.distance)||0;
    let distTxt=dist>=1000?(dist/1000).toFixed(2)+' km':dist.toFixed(1)+' m';
    let rows=[['Dystans:',distTxt],['Powierzchnia:',(d.area||'0.00')+' m2'],
              ['Czas:',fmtTime(d.elapsed||0)],['Predkosc:',(d.speed||'0.0')+' km/h']];
    if(d.gpsFix) rows.push(['GPS:',d.gpsLat+', '+d.gpsLng]);
    ctx.font='11px sans-serif';
    for(let i=0;i<rows.length;i++){
        ctx.fillStyle=TFT_MENU;ctx.textAlign='left';ctx.fillText(rows[i][0],14,y);
        ctx.fillStyle=TFT_TEXT;ctx.textAlign='right';ctx.fillText(rows[i][1],TFT_W-14,y);
        y+=18;
    }
    ctx.textAlign='left';
    tftHint(ctx,'START=kontynuuj STOP=nowy etap');
}

/* ---- Screen 11: LIFETIME STATS ---- */
function tftDrawLifetimeStats(ctx,d){
    tftClear(ctx);tftHeader(ctx,'STATYSTYKI LIFETIME');
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';
    ctx.fillText('Dane dostepne w zakladce Serwis',14,60);
    ctx.fillText('na panelu www (ponizej).',14,78);
    ctx.fillStyle=TFT_ACCENT;ctx.font='bold 12px sans-serif';
    ctx.fillText('Uzyj API /api/stats',14,110);
    tftHint(ctx,'STOP(1s)=powrot');
}

/* ---- Screen 12: CUSTOM PATTERN ---- */
function tftDrawCustomPattern(ctx,d){
    tftClear(ctx);tftHeader(ctx,'WZORZEC WLASNY');
    ctx.fillStyle=TFT_MENU;ctx.font='11px sans-serif';
    ctx.fillText('Edycja wzorca wlasnego',14,50);
    ctx.fillText('Konfiguracja dostepna na panelu www',14,68);
    ctx.fillText('(sekcja Wzorzec wlasny powyzej).',14,86);
    tftHint(ctx,'JOY:nawiguj SEL(1s)=zmien STOP(1s)=powrot');
}

/* ---- Screen 13: STATS EXPORT ---- */
function tftDrawStatsExport(ctx,d){
    tftClear(ctx);tftHeader(ctx,'EKSPORT STATYSTYK');
    ctx.fillStyle=TFT_MENU;ctx.font='bold 14px sans-serif';ctx.textAlign='center';
    ctx.fillText('Nacisnij START aby eksportowac',TFT_W/2,80);
    ctx.fillText('statystyki na karte SD',TFT_W/2,102);
    ctx.textAlign='left';
    tftHint(ctx,'START=eksportuj  STOP(1s)=powrot');
}

/* ---- Main TFT draw dispatcher ---- */
function tftDraw(d){
    let cvs=document.getElementById('tftCvs');if(!cvs)return;
    let ctx=cvs.getContext('2d');
    let scr=d.screen!==undefined?d.screen:0;
    switch(scr){
        case 0:tftDrawHome(ctx,d);break;
        case 1:tftDrawPainting(ctx,d);break;
        case 2:tftDrawServiceMenu(ctx,d);break;
        case 3:tftDrawCalibration(ctx,d);break;
        case 4:tftDrawDistMeter(ctx,d);break;
        case 5:tftDrawReports(ctx,d);break;
        case 6:tftDrawNozzleClean(ctx,d);break;
        case 7:tftDrawSetup(ctx,d);break;
        case 8:tftDrawSessionReset(ctx,d);break;
        case 9:tftDrawCounterReset(ctx,d);break;
        case 10:tftDrawSummary(ctx,d);break;
        case 11:tftDrawLifetimeStats(ctx,d);break;
        case 12:tftDrawCustomPattern(ctx,d);break;
        case 13:tftDrawStatsExport(ctx,d);break;
        default:tftClear(ctx);tftHeader(ctx,SCR_NAMES[scr]||'Ekran '+scr);break;
    }
}

/* ------- Apply status data to UI ------- */
function applyStatus(d){
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
        document.getElementById('spdMin').textContent=d.minSpeed;
        if(!spdSliderLoaded){
            document.getElementById('spdSlider').value=parseFloat(d.maxSpeed);
            document.getElementById('spdSliderVal').textContent=d.maxSpeed+' km/h';
            document.getElementById('minSpdSlider').value=parseFloat(d.minSpeed);
            document.getElementById('minSpdSliderVal').textContent=d.minSpeed+' km/h';
            spdSliderLoaded=true;
        }
        let spdWarnEl=document.getElementById('spdWarn');
        if(d.overspeed){spdWarnEl.style.display='inline';}
        else{spdWarnEl.style.display='none';}
        let spdLowEl=document.getElementById('spdLowWarn');
        if(d.lowSpeed&&d.state==='painting'){spdLowEl.style.display='inline';}
        else{spdLowEl.style.display='none';}

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
        let wsEl=document.getElementById('sWs');
        if(wsEl){wsEl.textContent=wsOk?'Polaczony':'Polling';wsEl.style.color=wsOk?'#2ae67a':'#f0c040';}

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

        /* GPS */
        let gpsFixEl=document.getElementById('gpsFix');
        if(gpsFixEl){
            if(d.gpsFix){gpsFixEl.textContent='TAK / '+d.gpsSat;gpsFixEl.style.color='#2ae67a';}
            else{gpsFixEl.textContent='BRAK / '+d.gpsSat;gpsFixEl.style.color='#e64040';}
        }
        let gpsPosEl=document.getElementById('gpsPos');
        if(gpsPosEl){
            if(d.gpsFix) gpsPosEl.textContent=d.gpsLat+', '+d.gpsLng;
            else gpsPosEl.textContent='---';
        }
        let gpsSpdEl=document.getElementById('gpsSpd');
        if(gpsSpdEl) gpsSpdEl.textContent=d.gpsSpeed+' km/h';
        let gpsHdEl=document.getElementById('gpsHdop');
        if(gpsHdEl) gpsHdEl.textContent=d.gpsHdop;

        /* Screen navigation highlight */
        if(d.screen!==undefined){
            let scrEl=document.getElementById('scrCur');
            if(scrEl) scrEl.textContent=SCR_NAMES[d.screen]||('Ekran '+d.screen);
            document.querySelectorAll('.scr-btn').forEach(function(el){
                let sid=parseInt(el.getAttribute('data-scr'));
                if(sid===d.screen) el.classList.add('act');
                else el.classList.remove('act');
            });
        }

        /* TFT screen preview */
        tftDraw(d);

}
/* ------- Fetch (REST fallback) ------- */
function fetchStatus(){
    fetch('/api/status').then(r=>r.json()).then(d=>applyStatus(d)).catch(e=>console.error('Status:',e));
}
/* ------- WebSocket (push updates co 500ms) ------- */
let ws=null,wsOk=false;
function wsConnect(){
    try{
        ws=new WebSocket('ws://'+location.hostname+':81/');
        ws.onopen=function(){wsOk=true;console.log('WS connected');};
        ws.onclose=function(){wsOk=false;setTimeout(wsConnect,2000);};
        ws.onerror=function(){};
        ws.onmessage=function(e){try{applyStatus(JSON.parse(e.data));}catch(x){}};
    }catch(x){setTimeout(wsConnect,3000);}
}
wsConnect();

/* ------- Service Menu Tabs ------- */
let activeTab=0;
let svcTick=0;
function svcTab(n){
    document.querySelectorAll('.svc-tab').forEach(function(t,i){t.classList.toggle('act',i===n);});
    document.getElementById('svcP0').style.display=n===0?'block':'none';
    document.getElementById('svcP1').style.display=n===1?'block':'none';
    document.getElementById('svcP2').style.display=n===2?'block':'none';
    activeTab=n;
    if(n===0)loadStats();
    if(n===1)loadReports();
    if(n===2)loadTracks();
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
            h+='<td style="text-align:right"><a href="/api/reports/download?file='+encodeURIComponent(r.file)+'" style="color:#2ae67a;text-decoration:none;font-weight:bold;">CSV</a> <a href="/api/reports/geojson?file='+encodeURIComponent(r.file)+'" style="color:#e6a02a;text-decoration:none;font-weight:bold;">GeoJSON</a></td></tr>';
        });
        h+='</table>';
        el.innerHTML=h;
    }).catch(function(){el.innerHTML='Blad ladowania raportow.';});
}
function loadTracks(){
    let el=document.getElementById('trkList');
    el.innerHTML='<span style="color:#6b7d9a;">Ladowanie...</span>';
    fetch('/api/tracks').then(function(r){return r.json();}).then(function(d){
        if(d.length===0){el.innerHTML='Brak tras GPS na karcie SD.';return;}
        let h='<table class="rep-tbl"><tr><th>Plik</th><th>Rozmiar</th><th style="text-align:right">Pobierz</th></tr>';
        d.forEach(function(r){
            let sz=r.size>1024?(r.size/1024).toFixed(1)+' KB':r.size+' B';
            let ext=r.file.split('.').pop().toUpperCase();
            h+='<tr><td>'+r.file+'</td><td>'+sz+'</td>';
            h+='<td style="text-align:right"><a href="/api/tracks/download?file='+encodeURIComponent(r.file)+'" style="color:#2ae67a;text-decoration:none;font-weight:bold;">'+ext+'</a></td></tr>';
        });
        h+='</table>';
        el.innerHTML=h;
    }).catch(function(){el.innerHTML='Blad ladowania tras.';});
}
loadStats();

/* Auto-refresh: WebSocket push (500ms) + fallback polling (2s gdy WS down) */
setInterval(function(){
    if(!wsOk)fetchStatus();
    svcTick++;
    if(svcTick%10===0&&activeTab===0)loadStats();
},2000);
fetchStatus();
</script>
</body>
</html>)rawhtml";
