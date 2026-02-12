// ============================================================
// TrassarV3 - Implementacja modulu wyswietlacza ILI9341
// 240x320 portret, podswietlenie LEDC PWM
// ============================================================

#include "display_manager.h"
#include "patterns.h"

// Skróty do czcionek GFX (includowane automatycznie przez TFT_eSPI z LOAD_GFXFF=1)
#define FSB18 &FreeSansBold18pt7b
#define FSB9  &FreeSansBold9pt7b
#define FS9   &FreeSans9pt7b
#define FM9   &FreeMono9pt7b

DisplayManager display;

// ============================================================
//  begin() - inicjalizacja TFT + splash
// ============================================================
void DisplayManager::begin() {
    // Podswietlenie przez LEDC (PWM) - nie analogWrite
    ledcSetup(TFT_BL_LEDC_CH, TFT_BL_LEDC_FREQ, TFT_BL_LEDC_RES);
    ledcAttachPin(PIN_TFT_BL, TFT_BL_LEDC_CH);
    setBacklight(TFT_BACKLIGHT_PWM);

    delay(100);  // stabilizacja zasilania

    tft.init();
    tft.setRotation(0);          // portret 240x320
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);

    // ----- Ekran powitalny (splash) -----
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    tft.setFreeFont(FSB18);
    tft.drawString("TrassarV3", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 - 40);

    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Malowarka drogowa", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2);

    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "v%s", FW_VERSION);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(verBuf, TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 30);

    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Inicjalizacja...", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 70);

    tft.setTextDatum(TL_DATUM);
}

// ============================================================
//  Podswietlenie
// ============================================================
void DisplayManager::setBacklight(uint8_t brightness) {
    ledcWrite(TFT_BL_LEDC_CH, brightness);
}

// ============================================================
//  Czyszczenie ekranu
// ============================================================
void DisplayManager::clear() {
    tft.fillScreen(COLOR_BG);
}

// ============================================================
//  Elementy pomocnicze
// ============================================================

void DisplayManager::drawHeader(const char* title) {
    tft.fillRect(0, 0, TFT_SCREEN_W, 36, COLOR_HEADER_BG);
    tft.setFreeFont(FSB9);
    tft.setTextColor(COLOR_HEADER_TXT, COLOR_HEADER_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, TFT_SCREEN_W / 2, 18);
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawStatusBar(MachineState state, const char* timeStr) {
    const int y = TFT_SCREEN_H - 28;
    tft.fillRect(0, y, TFT_SCREEN_W, 28, COLOR_HEADER_BG);

    // Kolorowa kropka + tekst stanu po lewej
    uint16_t sc = stateColor(state);
    tft.fillCircle(12, y + 14, 6, sc);
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_HEADER_TXT, COLOR_HEADER_BG);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(stateStr(state), 24, y + 14);

    // Czas po prawej
    if (timeStr && timeStr[0]) {
        tft.setTextDatum(MR_DATUM);
        tft.drawString(timeStr, TFT_SCREEN_W - 6, y + 14);
    }
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawGunIndicators(int y, const bool gunStates[6]) {
    // 6 kol w rzedzie, rownomiernie rozlozonych
    const int circR  = 10;
    const int startX = 20;
    const int stepX  = (TFT_SCREEN_W - 2 * startX) / (NUM_GUNS - 1);

    tft.setFreeFont(FM9);
    tft.setTextDatum(MC_DATUM);

    for (int i = 0; i < NUM_GUNS; i++) {
        int cx = startX + i * stepX;
        int cy = y + circR;

        uint16_t col = gunStates[i] ? COLOR_GUN_ON : COLOR_GUN_OFF;
        tft.fillCircle(cx, cy, circR, col);
        tft.drawCircle(cx, cy, circR, COLOR_TEXT);

        // Etykieta P1..P6 pod kolem
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "P%d", i + 1);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString(lbl, cx, cy + circR + 12);
    }
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
    if (percent < 0)   percent = 0;
    if (percent > 100)  percent = 100;

    tft.drawRect(x, y, w, h, COLOR_DIVIDER);
    int fillW = (w - 2) * percent / 100;
    if (fillW > 0) {
        tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
    }
    if (fillW < w - 2) {
        tft.fillRect(x + 1 + fillW, y + 1, w - 2 - fillW, h - 2, COLOR_BG);
    }
}

// ============================================================
//  EKRAN GLOWNY (HOME)
// ============================================================
void DisplayManager::drawHomeScreen(const char* timeStr, const char* dateStr,
                                    const char* patCode, const char* patName,
                                    float speedKmh, float distanceM,
                                    bool calibrated, bool reversed) {
    clear();
    drawHeader("TrassarV3");

    int y = 44;
    char buf[48];

    // --- Wzorzec ---
    y += 10;
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(patCode, TFT_SCREEN_W / 2, y + 18);
    tft.setTextDatum(TL_DATUM);

    y += 40;
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(patName, TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    if (reversed) {
        y += 18;
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("[ODWROCONY]", TFT_SCREEN_W / 2, y);
        tft.setTextDatum(TL_DATUM);
    }

    // --- Separator ---
    y += 16;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 12;

    // --- Predkosc ---
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Predkosc:", 16, y);
    snprintf(buf, sizeof(buf), "%.1f km/h", speedKmh);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 24;

    // --- Dystans ---
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Dystans:", 16, y);
    if (distanceM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", distanceM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", distanceM);
    }
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 24;

    // --- Kalibracja ---
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Kalibracja:", 16, y);
    if (calibrated) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("OK", TFT_SCREEN_W - 16, y + 7);
    } else {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("BRAK", TFT_SCREEN_W - 16, y + 7);
    }
    tft.setTextDatum(TL_DATUM);
    y += 24;

    // --- Data i czas ---
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    snprintf(buf, sizeof(buf), "%s   %s", timeStr, dateStr);
    tft.drawString(buf, TFT_SCREEN_W / 2, y + 6);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("START=maluj STOP=od przerwy", 6, y);
    y += 16;
    tft.drawString("SEL=wzorzec STOP(1s)=menu", 6, y);

    drawStatusBar(g_state.machineState, timeStr);
}

// ============================================================
//  EKRAN MALOWANIA
// ============================================================
void DisplayManager::drawPaintingScreen(MachineState state, const char* patCode,
                                        float speedKmh, float distM,
                                        float areaM2, unsigned long elapsedSec,
                                        const bool gunStates[6], bool reversed,
                                        bool gapStart) {
    clear();

    const char* title = (state == STATE_PAUSED) ? "PAUZA" : "MALOWANIE";
    drawHeader(title);

    int y = 44;
    char buf[48];

    // --- Status (duzy napis) ---
    uint16_t sc = stateColor(state);
    tft.setFreeFont(FSB18);
    tft.setTextColor(sc, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(stateStr(state), TFT_SCREEN_W / 2, y + 14);
    tft.setTextDatum(TL_DATUM);

    y += 34;

    // --- Wzorzec ---
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    if (reversed && gapStart) {
        snprintf(buf, sizeof(buf), "%s [ODW] [PRZERWA]", patCode);
    } else if (reversed) {
        snprintf(buf, sizeof(buf), "%s [ODW]", patCode);
    } else if (gapStart) {
        snprintf(buf, sizeof(buf), "%s [PRZERWA]", patCode);
    } else {
        snprintf(buf, sizeof(buf), "%s", patCode);
    }
    tft.drawString(buf, TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    y += 16;
    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 8;

    // --- Predkosc ---
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("V:", 16, y);
    snprintf(buf, sizeof(buf), "%.1f km/h", speedKmh);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(buf, 36, y);

    // --- Czas ---
    char timeBuf[16];
    fmtTime(elapsedSec, timeBuf, sizeof(timeBuf));
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("T:", 140, y);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(timeBuf, 160, y);
    y += 22;

    // --- Dystans ---
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Dyst:", 16, y);
    if (distM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", distM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", distM);
    }
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(buf, 64, y);

    // --- Powierzchnia ---
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("S:", 140, y);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(buf, 160, y);
    y += 22;

    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 8;

    // --- Wskazniki pistoletow ---
    drawGunIndicators(y, gunStates);
    y += 38;

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);

    if (state == STATE_PAINTING) {
        tft.drawString("START=pauza STOP=stop", 6, y);
    } else if (state == STATE_PAUSED) {
        tft.drawString("START=wznow STOP=stop", 6, y);
    }

    drawStatusBar(state, "");
}

// ============================================================
//  MENU SERWISOWE  (4 pozycje)
// ============================================================
void DisplayManager::drawServiceMenu(int selectedIndex) {
    clear();
    drawHeader("SERWIS");

    static const char* labels[4] = {
        "Kalibracja enkodera",
        "Pomiar dystansu",
        "Raporty",
        "Czyszczenie dysz"
    };

    const int itemH  = 42;
    const int startY = 44;

    tft.setFreeFont(FS9);

    for (int i = 0; i < 4; i++) {
        int iy = startY + i * itemH;
        bool sel = (i == selectedIndex);
        uint16_t bg = sel ? COLOR_MENU_SEL : COLOR_BG;
        uint16_t fg = sel ? COLOR_TEXT      : COLOR_MENU_TXT;

        tft.fillRect(0, iy, TFT_SCREEN_W, itemH, bg);
        tft.setTextColor(fg, bg);
        tft.setTextDatum(ML_DATUM);

        if (sel) {
            tft.drawString(">", 4, iy + itemH / 2);
        }
        tft.drawString(labels[i], 18, iy + itemH / 2);

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + itemH - 1, TFT_SCREEN_W, COLOR_DIVIDER);
    }

    // --- Podpowiedzi ---
    int y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=dalej STOP=cofnij", 6, y);
    y += 18;
    tft.drawString("SEL(1s)=wejdz STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  KALIBRACJA ENKODERA
// ============================================================
void DisplayManager::drawCalibrationScreen(bool active, float pulses, float ppm, bool calibrated) {
    clear();
    drawHeader("KALIBRACJA ENKODERA");

    int y = 50;
    char buf[48];

    tft.setFreeFont(FS9);

    // Instrukcje
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Odmierz 10 m i przejdz", 16, y);
    y += 20;
    tft.drawString("maszyna po prostej.", 16, y);
    y += 30;

    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 15;

    // Aktualny status
    if (active) {
        tft.setFreeFont(FSB18);
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("POMIAR...", TFT_SCREEN_W / 2, y + 18);
        tft.setTextDatum(TL_DATUM);
        y += 45;

        tft.setFreeFont(FS9);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString("Impulsy:", 16, y);
        snprintf(buf, sizeof(buf), "%.0f", pulses);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
        tft.setTextDatum(TL_DATUM);
    } else {
        tft.setFreeFont(FSB9);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("GOTOWY", TFT_SCREEN_W / 2, y + 18);
        tft.setTextDatum(TL_DATUM);
        y += 45;
    }

    y += 25;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 12;

    // Biezace PPM
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Imp/metr:", 16, y);
    snprintf(buf, sizeof(buf), "%.1f", ppm);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 24;

    // Status kalibracji
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Status:", 16, y);
    if (calibrated) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("Skalibrowany", TFT_SCREEN_W - 16, y + 7);
    } else {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("Domyslny", TFT_SCREEN_W - 16, y + 7);
    }
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    if (active) {
        tft.drawString("START=zakoncz pomiar", 6, y);
    } else {
        tft.drawString("START=rozpocznij pomiar", 6, y);
    }
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  POMIAR DYSTANSU (cyfrowa miara)
// ============================================================
void DisplayManager::drawDistanceMeter(float distanceM, bool measuring) {
    clear();
    drawHeader("POMIAR DYSTANSU");

    int y = 50;
    char buf[48];

    // Status
    tft.setFreeFont(FSB9);
    tft.setTextColor(measuring ? COLOR_ACCENT : COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(measuring ? "POMIAR..." : "GOTOWY", TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    y += 30;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 20;

    // Duzy wynik dystansu
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    if (distanceM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", distanceM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.2f m", distanceM);
    }
    tft.drawString(buf, TFT_SCREEN_W / 2, y + 16);
    tft.setTextDatum(TL_DATUM);

    y += 50;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 16;

    // Wartosc w cm
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "= %.0f cm", distanceM * 100.0f);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    if (measuring) {
        tft.drawString("START=pauza STOP=reset", 6, y);
    } else {
        tft.drawString("START=pomiar STOP=reset", 6, y);
    }
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  RAPORTY (karta SD)
// ============================================================
void DisplayManager::drawReportsScreen(bool sdReady, int fileCount, const char* lastReport) {
    clear();
    drawHeader("RAPORTY");

    int y = 50;
    char buf[48];

    tft.setFreeFont(FS9);

    // Status SD
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Karta SD:", 16, y);
    if (sdReady) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("OK", TFT_SCREEN_W - 16, y + 7);
    } else {
        tft.setTextColor(COLOR_ERROR, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("BRAK", TFT_SCREEN_W - 16, y + 7);
    }
    tft.setTextDatum(TL_DATUM);
    y += 26;

    // Liczba plikow
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Plikow raportow:", 16, y);
    snprintf(buf, sizeof(buf), "%d", fileCount);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 30;

    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 12;

    // Ostatni wpis
    tft.setFreeFont(FSB9);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("Ostatni wpis:", 16, y);
    y += 24;

    tft.setFreeFont(FM9);
    if (lastReport && lastReport[0]) {
        // Parsuj CSV: data,godzina,wzorzec,dystans,powierzchnia
        // Wyswietl w czytelnej formie
        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        // Linia moze byc dluga - podziel
        char tmp[128];
        strncpy(tmp, lastReport, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = 0;

        // Znajdz pola
        char* tok = strtok(tmp, ",");
        if (tok) { // data
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Data:", 16, y);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(tok, 90, y);
            y += 18;
        }
        tok = strtok(NULL, ",");
        if (tok) { // godzina
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Godz:", 16, y);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(tok, 90, y);
            y += 18;
        }
        tok = strtok(NULL, ",");
        if (tok) { // wzorzec
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Wzorzec:", 16, y);
            tft.setTextColor(COLOR_ACCENT, COLOR_BG);
            tft.drawString(tok, 120, y);
            y += 18;
        }
        tok = strtok(NULL, ",");
        if (tok) { // dystans
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Dystans:", 16, y);
            snprintf(buf, sizeof(buf), "%s m", tok);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(buf, 120, y);
            y += 18;
        }
        tok = strtok(NULL, ",");
        if (tok) { // powierzchnia
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Pow:", 16, y);
            snprintf(buf, sizeof(buf), "%s m2", tok);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(buf, 120, y);
        }
    } else {
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString("Brak wpisow", 16, y);
    }

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 40;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  CZYSZCZENIE DYSZ
// ============================================================
void DisplayManager::drawNozzleClean(const char* patCode, const char* patName,
                                     const GunPatternCfg gunsCfg[6],
                                     const bool gunStates[6]) {
    clear();
    drawHeader("CZYSZCZENIE DYSZ");

    int y = 46;
    char buf[48];

    // Wybrany wzorzec
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(patCode, TFT_SCREEN_W / 2, y + 16);
    tft.setTextDatum(TL_DATUM);

    y += 40;
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(patName, TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    y += 20;
    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 12;

    // Pistoletowy wzorzec: 6 prostokatow
    // Zolty = uzyty we wzorcu, Zielony = aktywny (strzelajacy), Szary = nieuzywany
    const int rectW = 30;
    const int rectH = 40;
    const int gap   = 4;
    const int totalW = NUM_GUNS * rectW + (NUM_GUNS - 1) * gap;
    const int startX = (TFT_SCREEN_W - totalW) / 2;

    tft.setFreeFont(FM9);
    tft.setTextDatum(MC_DATUM);

    for (int i = 0; i < NUM_GUNS; i++) {
        int rx = startX + i * (rectW + gap);
        int ry = y;
        bool usedInPattern = (gunsCfg[i].mode != GUN_OFF);
        bool firing = gunStates[i];

        uint16_t col;
        if (firing) {
            col = COLOR_GUN_ON;        // zielony - strzela
        } else if (usedInPattern) {
            col = COLOR_WARNING;        // zolty - uzyty we wzorcu
        } else {
            col = COLOR_GUN_OFF;        // szary - nieuzywany
        }

        tft.fillRect(rx, ry, rectW, rectH, col);
        tft.drawRect(rx, ry, rectW, rectH, COLOR_TEXT);

        // Etykieta
        snprintf(buf, sizeof(buf), "P%d", i + 1);
        tft.setTextColor(COLOR_BG, col);
        tft.drawString(buf, rx + rectW / 2, ry + rectH / 2);
    }
    tft.setTextDatum(TL_DATUM);

    y += rectH + 14;

    // Legenda
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_WARNING, COLOR_BG);
    tft.drawString("Zolty=we wzorcu", 16, y);
    y += 16;
    tft.setTextColor(COLOR_GUN_ON, COLOR_BG);
    tft.drawString("Zielony=strzela", 16, y);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=wzorzec TRZYMAJ START", 6, y);
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  Funkcje pomocnicze prywatne
// ============================================================

const char* DisplayManager::stateStr(MachineState s) {
    switch (s) {
        case STATE_IDLE:     return "Gotowy";
        case STATE_PAINTING: return "Malowanie";
        case STATE_PAUSED:   return "Pauza";
        case STATE_STOPPED:  return "Zatrzymany";
        default:             return "?";
    }
}

uint16_t DisplayManager::stateColor(MachineState s) {
    switch (s) {
        case STATE_IDLE:     return COLOR_ACCENT;
        case STATE_PAINTING: return COLOR_ACCENT;
        case STATE_PAUSED:   return COLOR_WARNING;
        case STATE_STOPPED:  return COLOR_ERROR;
        default:             return COLOR_MENU_TXT;
    }
}

void DisplayManager::fmtTime(unsigned long sec, char* buf, size_t len) {
    unsigned long h = sec / 3600;
    unsigned long m = (sec % 3600) / 60;
    unsigned long s = sec % 60;

    if (h > 0) {
        snprintf(buf, len, "%lu:%02lu:%02lu", h, m, s);
    } else {
        snprintf(buf, len, "%02lu:%02lu", m, s);
    }
}
