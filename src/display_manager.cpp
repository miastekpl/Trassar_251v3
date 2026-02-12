// ============================================================
// TrassarV3 - Implementacja modułu wyświetlacza ILI9341
// ============================================================

#include "display_manager.h"

DisplayManager display;

void DisplayManager::begin() {
    // Inicjalizacja podświetlenia przez LEDC (PWM)
    ledcSetup(TFT_BL_LEDC_CH, TFT_BL_LEDC_FREQ, TFT_BL_LEDC_RES);
    ledcAttachPin(PIN_TFT_BL, TFT_BL_LEDC_CH);
    setBacklight(TFT_BACKLIGHT_PWM);

    delay(100);  // Daj czas na stabilizację zasilania wyświetlacza

    tft.init();
    tft.setRotation(0);  // Portret 240x320
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);

    // Ekran powitalny
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.drawString("TrassarV3", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 - 30);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Sterownik maszyny malarskiej", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 10);

    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "Firmware v%s", FW_VERSION);
    tft.drawString(verBuf, TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 40);

    tft.drawString("Inicjalizacja...", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 80);
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::setBacklight(uint8_t brightness) {
    ledcWrite(TFT_BL_LEDC_CH, brightness);
}

void DisplayManager::clear() {
    tft.fillScreen(COLOR_BG);
}

// ============ Elementy pomocnicze ============

void DisplayManager::drawHeader(const char* title) {
    tft.fillRect(0, 0, TFT_SCREEN_W, 36, COLOR_HEADER_BG);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_HEADER_TXT, COLOR_HEADER_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, TFT_SCREEN_W / 2, 18);
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawStatusBar(MachineState state, const char* timeStr) {
    int y = TFT_SCREEN_H - 28;
    tft.fillRect(0, y, TFT_SCREEN_W, 28, COLOR_HEADER_BG);

    // Status po lewej
    uint16_t stColor = stateToColor(state);
    tft.fillCircle(12, y + 14, 6, stColor);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_HEADER_TXT, COLOR_HEADER_BG);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(stateToString(state), 24, y + 14);

    // Czas po prawej
    tft.setTextDatum(MR_DATUM);
    tft.drawString(timeStr, TFT_SCREEN_W - 6, y + 14);
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    tft.drawRect(x, y, w, h, COLOR_DIVIDER);
    int fillW = (w - 2) * percent / 100;
    if (fillW > 0) {
        tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
    }
    if (fillW < w - 2) {
        tft.fillRect(x + 1 + fillW, y + 1, w - 2 - fillW, h - 2, COLOR_BG);
    }
}

void DisplayManager::drawMenuItem(int y, const char* label, bool selected) {
    uint16_t bg = selected ? COLOR_MENU_SEL : COLOR_BG;
    uint16_t fg = selected ? COLOR_TEXT : COLOR_MENU_TXT;

    tft.fillRect(0, y, TFT_SCREEN_W, 36, bg);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(fg, bg);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(label, 16, y + 18);

    if (selected) {
        tft.drawString(">", 4, y + 18);
    }

    tft.setTextDatum(TL_DATUM);
    // Linia podziału
    tft.drawFastHLine(0, y + 35, TFT_SCREEN_W, COLOR_DIVIDER);
}

// ============ Ekrany ============

void DisplayManager::drawHomeScreen(const char* timeStr, const char* dateStr) {
    clear();
    drawHeader("TrassarV3");

    int y = 50;

    // Czas - duży
    tft.setFreeFont(&FreeSansBold24pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(timeStr, TFT_SCREEN_W / 2, y + 30);

    // Data
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString(dateStr, TFT_SCREEN_W / 2, y + 65);
    tft.setTextDatum(TL_DATUM);

    // Status maszyny
    y = 140;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 15;

    uint16_t stColor = stateToColor(g_state.machineState);
    tft.fillCircle(30, y + 8, 8, stColor);

    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Status:", 48, y);

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(stColor, COLOR_BG);
    tft.drawString(stateToString(g_state.machineState), 120, y);

    // Informacje o ustawieniach
    y += 35;
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    char buf[40];
    snprintf(buf, sizeof(buf), "Predkosc: %d%%", g_state.paintSpeed);
    tft.drawString(buf, 20, y);

    y += 22;
    snprintf(buf, sizeof(buf), "Przejazdy: %d", g_state.paintPasses);
    tft.drawString(buf, 20, y);

    // Podpowiedzi nawigacyjne
    y = TFT_SCREEN_H - 60;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeMono9pt7b);
    tft.drawString("START=maluj STOP(1s)=menu", 6, y);

    drawStatusBar(g_state.machineState, timeStr);
}

void DisplayManager::drawMainMenu(int selectedIndex) {
    clear();
    drawHeader("MENU GLOWNE");

    int startY = 44;
    for (int i = 0; i < 4; i++) {
        const char* labels[] = {
            "Ustawienia malowania",
            "Czas i data",
            "Informacje WiFi",
            "Informacje systemowe"
        };
        drawMenuItem(startY + i * 37, labels[i], (i == selectedIndex));
    }

    // Podpowiedzi
    int y = TFT_SCREEN_H - 60;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=nawiguj SEL(1s)=wejdz", 6, y);
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

void DisplayManager::drawPaintSettings(int selectedField, int speed, int passes) {
    clear();
    drawHeader("USTAWIENIA MALOWANIA");

    int y = 55;

    // Prędkość
    bool sel0 = (selectedField == 0);
    tft.fillRect(0, y, TFT_SCREEN_W, 50, sel0 ? COLOR_MENU_SEL : COLOR_BG);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(sel0 ? COLOR_TEXT : COLOR_MENU_TXT, sel0 ? COLOR_MENU_SEL : COLOR_BG);
    tft.drawString("Predkosc:", 16, y + 5);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", speed);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_ACCENT, sel0 ? COLOR_MENU_SEL : COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 25);
    tft.setTextDatum(TL_DATUM);

    drawProgressBar(16, y + 38, TFT_SCREEN_W - 32, 8, speed, COLOR_ACCENT);

    y += 60;
    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 10;

    // Przejazdy
    bool sel1 = (selectedField == 1);
    tft.fillRect(0, y, TFT_SCREEN_W, 50, sel1 ? COLOR_MENU_SEL : COLOR_BG);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(sel1 ? COLOR_TEXT : COLOR_MENU_TXT, sel1 ? COLOR_MENU_SEL : COLOR_BG);
    tft.drawString("Liczba przejsc:", 16, y + 5);

    snprintf(buf, sizeof(buf), "%d", passes);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_ACCENT, sel1 ? COLOR_MENU_SEL : COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 25);
    tft.setTextDatum(TL_DATUM);

    // Podpowiedzi
    y = TFT_SCREEN_H - 60;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=pole  ENC=wartosc", 6, y);
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

void DisplayManager::drawTimeSettings(const char* timeStr, const char* dateStr, int selectedField) {
    clear();
    drawHeader("CZAS I DATA");

    int y = 55;
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Aktualny czas:", 16, y);
    y += 22;

    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(timeStr, 16, y);
    y += 28;
    tft.drawString(dateStr, 16, y);

    y += 40;
    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 15;

    const char* fields[] = {"Godzina", "Minuta", "Sekunda", "Dzien", "Miesiac", "Rok"};
    for (int i = 0; i < 6; i++) {
        bool sel = (selectedField == i);
        tft.setFreeFont(&FreeSans9pt7b);
        tft.setTextColor(sel ? COLOR_ACCENT : COLOR_MENU_TXT, COLOR_BG);
        if (sel) tft.drawString(">", 4, y);
        tft.drawString(fields[i], 16, y);
        y += 22;
    }

    // Podpowiedzi
    y = TFT_SCREEN_H - 60;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=pole  ENC=wartosc", 6, y);
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

void DisplayManager::drawWifiInfo(const char* ssid, const char* ip, int clients) {
    clear();
    drawHeader("INFORMACJE WIFI");

    int y = 55;
    tft.setFreeFont(&FreeSans9pt7b);

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Tryb: Access Point", 16, y);
    y += 28;

    tft.drawString("SSID:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(ssid, 70, y);
    y += 28;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Haslo:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(WIFI_AP_PASS, 80, y);
    y += 28;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("IP:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(ip, 46, y);
    y += 28;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    char buf[32];
    snprintf(buf, sizeof(buf), "Polaczeni klienci: %d", clients);
    tft.drawString(buf, 16, y);
    y += 35;

    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 15;

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Polacz sie z siecia WiFi", 16, y);
    y += 20;
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(ssid, 16, y);
    y += 20;
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("i otworz przegladarke:", 16, y);
    y += 22;
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_WARNING, COLOR_BG);
    char urlBuf[40];
    snprintf(urlBuf, sizeof(urlBuf), "http://%s", ip);
    tft.drawString(urlBuf, 16, y);

    // Podpowiedzi
    y = TFT_SCREEN_H - 40;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

void DisplayManager::drawSystemInfo(const char* fwVer, const char* fwDate, uint32_t freeHeap, uint32_t uptime) {
    clear();
    drawHeader("INFORMACJE SYSTEMOWE");

    int y = 55;
    tft.setFreeFont(&FreeSans9pt7b);

    char buf[48];

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Firmware:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    snprintf(buf, sizeof(buf), "v%s", fwVer);
    tft.drawString(buf, 120, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Kompilacja:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(fwDate, 120, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Wolna RAM:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    snprintf(buf, sizeof(buf), "%lu KB", freeHeap / 1024);
    tft.drawString(buf, 120, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Uptime:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    unsigned long s = uptime / 1000;
    unsigned long h = s / 3600;
    unsigned long m = (s % 3600) / 60;
    snprintf(buf, sizeof(buf), "%luh %lum %lus", h, m, s % 60);
    tft.drawString(buf, 120, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Platforma:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("ESP32-S3 N16R8", 120, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Wyswietlacz:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("ILI9341 240x320", 120, y);

    // Podpowiedzi
    y = TFT_SCREEN_H - 40;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

void DisplayManager::drawPaintingScreen(MachineState state, int speed, int currentPass, int totalPasses, unsigned long elapsed) {
    clear();

    const char* title = (state == STATE_PAUSED) ? "PAUZA" : "MALOWANIE";
    drawHeader(title);

    int y = 50;

    // Status
    uint16_t stColor = stateToColor(state);
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(stColor, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(stateToString(state), TFT_SCREEN_W / 2, y + 15);
    tft.setTextDatum(TL_DATUM);

    y = 90;
    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 15;

    tft.setFreeFont(&FreeSans9pt7b);

    // Czas trwania
    char elapsedBuf[16];
    formatElapsed(elapsed, elapsedBuf, sizeof(elapsedBuf));
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Czas:", 16, y);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(elapsedBuf, 80, y - 2);
    y += 35;

    // Prędkość
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Predkosc:", 16, y);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", speed);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(buf, 120, y);
    y += 30;

    // Pasek postępu prędkości
    drawProgressBar(16, y, TFT_SCREEN_W - 32, 10, speed, COLOR_ACCENT);
    y += 25;

    // Przejazdy
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Przejazd:", 16, y);
    snprintf(buf, sizeof(buf), "%d / %d", currentPass, totalPasses);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(buf, 120, y);
    y += 25;

    // Pasek postępu przejść
    int passPercent = (totalPasses > 0) ? (currentPass * 100 / totalPasses) : 0;
    drawProgressBar(16, y, TFT_SCREEN_W - 32, 10, passPercent, COLOR_WARNING);

    // Podpowiedzi
    y = TFT_SCREEN_H - 60;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);

    if (state == STATE_RUNNING) {
        tft.drawString("START=pauza STOP=stop", 6, y);
        y += 18;
        tft.drawString("ENC=predkosc", 6, y);
    } else if (state == STATE_PAUSED) {
        tft.drawString("START=wznow STOP=stop", 6, y);
    }

    drawStatusBar(state, "");
}

// ============ Funkcje pomocnicze ============

const char* DisplayManager::stateToString(MachineState state) {
    switch (state) {
        case STATE_IDLE:    return "Gotowy";
        case STATE_RUNNING: return "Malowanie";
        case STATE_PAUSED:  return "Pauza";
        case STATE_STOPPED: return "Zatrzymany";
        case STATE_ERROR:   return "Blad";
        default:            return "?";
    }
}

uint16_t DisplayManager::stateToColor(MachineState state) {
    switch (state) {
        case STATE_IDLE:    return COLOR_STATUS_IDLE;
        case STATE_RUNNING: return COLOR_STATUS_RUN;
        case STATE_PAUSED:  return COLOR_STATUS_PAUSE;
        case STATE_STOPPED: return COLOR_STATUS_STOP;
        case STATE_ERROR:   return COLOR_ERROR;
        default:            return COLOR_STATUS_IDLE;
    }
}

void DisplayManager::formatElapsed(unsigned long ms, char* buf, size_t len) {
    unsigned long totalSec = ms / 1000;
    unsigned long h = totalSec / 3600;
    unsigned long m = (totalSec % 3600) / 60;
    unsigned long s = totalSec % 60;

    if (h > 0) {
        snprintf(buf, len, "%lu:%02lu:%02lu", h, m, s);
    } else {
        snprintf(buf, len, "%02lu:%02lu", m, s);
    }
}
