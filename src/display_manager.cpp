// ============================================================
// TrassarV3 - Implementacja modulu wyswietlacza ILI9341
// 240x320 portret, podswietlenie LEDC PWM
// ============================================================

#include "display_manager.h"
#include "patterns.h"

#include <Free_Fonts.h>   // FreeSansBold18pt7b, FreeSansBold9pt7b, FreeSans9pt7b, FreeMono9pt7b

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

    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.drawString("TrassarV3", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 - 40);

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Malowarka drogowa", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2);

    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "v%s", FW_VERSION);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(verBuf, TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 30);

    tft.setFreeFont(&FreeMono9pt7b);
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
    tft.setFreeFont(&FreeSansBold9pt7b);
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
    tft.setFreeFont(&FreeSans9pt7b);
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

    tft.setFreeFont(&FreeMono9pt7b);
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
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(patCode, TFT_SCREEN_W / 2, y + 18);
    tft.setTextDatum(TL_DATUM);

    y += 40;
    tft.setFreeFont(&FreeSans9pt7b);
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
    tft.setFreeFont(&FreeSans9pt7b);
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
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    snprintf(buf, sizeof(buf), "%s   %s", timeStr, dateStr);
    tft.drawString(buf, TFT_SCREEN_W / 2, y + 6);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("START=maluj STOP(1s)=menu", 6, y);

    drawStatusBar(g_state.machineState, timeStr);
}

// ============================================================
//  EKRAN MALOWANIA
// ============================================================
void DisplayManager::drawPaintingScreen(MachineState state, const char* patCode,
                                        float speedKmh, float distM,
                                        float areaM2, unsigned long elapsedSec,
                                        const bool gunStates[6], bool reversed) {
    clear();

    const char* title = (state == STATE_PAUSED) ? "PAUZA" : "MALOWANIE";
    drawHeader(title);

    int y = 44;
    char buf[48];

    // --- Status (duzy napis) ---
    uint16_t sc = stateColor(state);
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(sc, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(stateStr(state), TFT_SCREEN_W / 2, y + 14);
    tft.setTextDatum(TL_DATUM);

    y += 34;

    // --- Wzorzec ---
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    if (reversed) {
        snprintf(buf, sizeof(buf), "%s [ODW]", patCode);
        tft.drawString(buf, TFT_SCREEN_W / 2, y);
    } else {
        tft.drawString(patCode, TFT_SCREEN_W / 2, y);
    }
    tft.setTextDatum(TL_DATUM);

    y += 16;
    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 8;

    // --- Predkosc ---
    tft.setFreeFont(&FreeSans9pt7b);
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
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);

    if (state == STATE_PAINTING) {
        tft.drawString("START=pauza STOP=stop", 6, y);
    } else if (state == STATE_PAUSED) {
        tft.drawString("START=wznow STOP=stop", 6, y);
    }

    drawStatusBar(state, "");
}

// ============================================================
//  MENU GLOWNE  (6 pozycji)
// ============================================================
void DisplayManager::drawMainMenu(int selectedIndex) {
    clear();
    drawHeader("MENU GLOWNE");

    static const char* labels[6] = {
        "Wybor wzorca",
        "Kalibracja enkodera",
        "Statystyki",
        "Czas i data",
        "Informacje WiFi",
        "Info systemowe"
    };

    const int itemH  = 36;
    const int startY = 42;

    tft.setFreeFont(&FreeSans9pt7b);

    for (int i = 0; i < 6; i++) {
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
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("ENC=nawiguj  SEL=wejdz", 6, y);
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  WYBOR WZORCA  (15 pozycji, przewijane)
// ============================================================
void DisplayManager::drawPatternSelect(int selectedIndex) {
    clear();
    drawHeader("WYBOR WZORCA");

    const int itemH   = 28;
    const int startY  = 40;
    const int visible = 8;   // ile pozycji widocznych

    // Oblicz offset przewijania tak, by zaznaczony element byl widoczny
    int offset = 0;
    if (selectedIndex >= visible) {
        offset = selectedIndex - visible + 1;
    }
    if (offset > PAT_COUNT - visible) {
        offset = PAT_COUNT - visible;
    }
    if (offset < 0) offset = 0;

    tft.setFreeFont(&FreeSans9pt7b);
    char buf[40];

    for (int v = 0; v < visible && (offset + v) < PAT_COUNT; v++) {
        int idx = offset + v;
        int iy  = startY + v * itemH;
        bool sel = (idx == selectedIndex);

        uint16_t bg = sel ? COLOR_MENU_SEL : COLOR_BG;
        uint16_t fg = sel ? COLOR_TEXT      : COLOR_MENU_TXT;

        tft.fillRect(0, iy, TFT_SCREEN_W, itemH, bg);
        tft.setTextColor(fg, bg);
        tft.setTextDatum(ML_DATUM);

        if (sel) {
            tft.drawString(">", 4, iy + itemH / 2);
        }

        const PatternDef& pat = PatternManager::patterns[idx];
        snprintf(buf, sizeof(buf), "%-5s %s", pat.code, pat.name);
        tft.drawString(buf, 18, iy + itemH / 2);

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + itemH - 1, TFT_SCREEN_W, COLOR_DIVIDER);
    }

    // Wskaznik pozycji (pasek przewijania)
    int barAreaY = startY;
    int barAreaH = visible * itemH;
    int barH     = barAreaH * visible / PAT_COUNT;
    if (barH < 8) barH = 8;
    int barY = barAreaY + (barAreaH - barH) * offset / (PAT_COUNT - visible > 0 ? PAT_COUNT - visible : 1);
    tft.fillRect(TFT_SCREEN_W - 4, barAreaY, 4, barAreaH, COLOR_BG);
    tft.fillRect(TFT_SCREEN_W - 3, barY, 3, barH, COLOR_DIVIDER);

    // --- Podpowiedzi ---
    int y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("ENC=nawiguj  SEL=wybierz", 6, y);
    y += 18;
    tft.drawString("STOP(1s)=powrot", 6, y);

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

    tft.setFreeFont(&FreeSans9pt7b);

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
        tft.setFreeFont(&FreeSansBold18pt7b);
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("POMIAR...", TFT_SCREEN_W / 2, y + 18);
        tft.setTextDatum(TL_DATUM);
        y += 45;

        tft.setFreeFont(&FreeSans9pt7b);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString("Impulsy:", 16, y);
        snprintf(buf, sizeof(buf), "%.0f", pulses);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
        tft.setTextDatum(TL_DATUM);
    } else {
        tft.setFreeFont(&FreeSansBold9pt7b);
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
    tft.setFreeFont(&FreeSans9pt7b);
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
    tft.setFreeFont(&FreeMono9pt7b);
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
//  STATYSTYKI
// ============================================================
void DisplayManager::drawStatisticsScreen(float sessDist, float sessArea, unsigned long sessTime,
                                          float ltDist, float ltArea, uint32_t ltTime) {
    clear();
    drawHeader("STATYSTYKI");

    int y = 48;
    char buf[48];
    char timeBuf[16];

    // --- Sesja ---
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("Biezaca sesja", 16, y);
    y += 22;

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Dystans:", 16, y);
    if (sessDist >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", sessDist / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", sessDist);
    }
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 22;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Powierzchnia:", 16, y);
    snprintf(buf, sizeof(buf), "%.1f m2", sessArea);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 22;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Czas pracy:", 16, y);
    fmtTime(sessTime, timeBuf, sizeof(timeBuf));
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(timeBuf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 28;

    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 12;

    // --- Laczne (lifetime) ---
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("Laczne (calkowite)", 16, y);
    y += 22;

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Dystans:", 16, y);
    if (ltDist >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", ltDist / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", ltDist);
    }
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 22;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Powierzchnia:", 16, y);
    snprintf(buf, sizeof(buf), "%.1f m2", ltArea);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);
    y += 22;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Czas pracy:", 16, y);
    fmtTime(ltTime, timeBuf, sizeof(timeBuf));
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(timeBuf, TFT_SCREEN_W - 16, y + 7);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 40;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  INFORMACJE WIFI
// ============================================================
void DisplayManager::drawWifiInfo(const char* ssid, const char* ip, int clients) {
    clear();
    drawHeader("INFORMACJE WIFI");

    int y = 50;
    char buf[48];

    tft.setFreeFont(&FreeSans9pt7b);

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Tryb: Access Point", 16, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SSID:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(ssid, 70, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Haslo:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(WIFI_AP_PASS, 80, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("IP:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(ip, 46, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "Polaczeni klienci: %d", clients);
    tft.drawString(buf, 16, y);
    y += 30;

    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 14;

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

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 40;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  INFORMACJE SYSTEMOWE
// ============================================================
void DisplayManager::drawSystemInfo(const char* fwVer, uint32_t freeHeap, uint32_t uptime) {
    clear();
    drawHeader("INFO SYSTEMOWE");

    int y = 50;
    char buf[48];

    tft.setFreeFont(&FreeSans9pt7b);

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Firmware:", 16, y);
    snprintf(buf, sizeof(buf), "v%s", fwVer);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(buf, 130, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Kompilacja:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(FW_DATE, 130, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Wolna RAM:", 16, y);
    snprintf(buf, sizeof(buf), "%lu KB", (unsigned long)(freeHeap / 1024));
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(buf, 130, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Uptime:", 16, y);
    {
        unsigned long s = uptime;
        unsigned long h = s / 3600;
        unsigned long m = (s % 3600) / 60;
        snprintf(buf, sizeof(buf), "%luh %lum %lus", h, m, s % 60);
    }
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(buf, 130, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Platforma:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("ESP32-S3 N16R8", 130, y);
    y += 26;

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Wyswietlacz:", 16, y);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("ILI9341 240x320", 130, y);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 40;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 10;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);

    drawStatusBar(g_state.machineState, "");
}

// ============================================================
//  CZAS I DATA
// ============================================================
void DisplayManager::drawTimeSettings(const char* timeStr, const char* dateStr, int selectedField) {
    clear();
    drawHeader("CZAS I DATA");

    int y = 50;

    // Biezacy czas/data (duzy)
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Aktualny czas:", 16, y);
    y += 24;

    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(timeStr, TFT_SCREEN_W / 2, y + 10);
    tft.setTextDatum(TL_DATUM);
    y += 30;

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(dateStr, TFT_SCREEN_W / 2, y + 4);
    tft.setTextDatum(TL_DATUM);
    y += 20;

    tft.drawFastHLine(10, y, TFT_SCREEN_W - 20, COLOR_DIVIDER);
    y += 12;

    // Pola edycji
    static const char* fields[6] = {
        "Godzina", "Minuta", "Sekunda",
        "Dzien",   "Miesiac", "Rok"
    };

    tft.setFreeFont(&FreeSans9pt7b);
    for (int i = 0; i < 6; i++) {
        bool sel = (selectedField == i);
        uint16_t fg = sel ? COLOR_ACCENT : COLOR_MENU_TXT;
        tft.setTextColor(fg, COLOR_BG);
        if (sel) {
            tft.drawString(">", 6, y);
        }
        tft.drawString(fields[i], 20, y);
        y += 22;
    }

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 62;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 8;
    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=pole  ENC=wartosc", 6, y);
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
