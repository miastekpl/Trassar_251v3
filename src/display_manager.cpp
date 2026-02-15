// ============================================================
// TrassarV3 - Implementacja modulu wyswietlacza ILI9341
// 320x240 landscape, podswietlenie LEDC PWM
// v2.3.0 - Wzorzec FSB24 u gory, predkosc FSB24, status nad pistoletami
//           Bez migania: setTextPadding zamiast clear()
// ============================================================

#include "display_manager.h"
#include "patterns.h"

// Skróty do czcionek GFX (includowane automatycznie przez TFT_eSPI z LOAD_GFXFF=1)
#define FSB24 &FreeSansBold24pt7b
#define FSB18 &FreeSansBold18pt7b
#define FSB12 &FreeSansBold12pt7b
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
    tft.setRotation(1);          // landscape 320x240
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);

    // ----- Ekran powitalny (splash) -----
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    tft.setFreeFont(FSB24);
    tft.drawString("TrassarV3", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 - 40);

    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Malowarka drogowa", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 10);

    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "v%s", FW_VERSION);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(verBuf, TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + 40);

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
    tft.fillRect(0, 0, TFT_SCREEN_W, 30, COLOR_HEADER_BG);
    tft.setFreeFont(FSB9);
    tft.setTextColor(COLOR_HEADER_TXT, COLOR_HEADER_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, TFT_SCREEN_W / 2, 15);
    tft.setTextDatum(TL_DATUM);
}

// ============================================================
//  drawGunRects - 6 prostokatow pistoletow na dole ekranu
//  Kolory: zolty=we wzorcu, zielony=strzela, zolty miganie=pauza, szary=nieuzywany
// ============================================================
void DisplayManager::drawGunRects(int y, const GunPatternCfg gunsCfg[6],
                                   const bool gunStates[6], bool paused) {
    const int rectW = 42;
    const int rectH = 50;
    const int gap   = 6;
    const int totalW = NUM_GUNS * rectW + (NUM_GUNS - 1) * gap;
    const int startX = (TFT_SCREEN_W - totalW) / 2;

    bool blinkOn = ((millis() / 500) % 2) == 0;

    tft.setFreeFont(FSB9);
    tft.setTextDatum(MC_DATUM);

    char lbl[4];
    for (int i = 0; i < NUM_GUNS; i++) {
        int rx = startX + i * (rectW + gap);
        int ry = y;
        bool usedInPattern = (gunsCfg[i].mode != GUN_OFF);
        bool firing = gunStates ? gunStates[i] : false;

        uint16_t col;
        if (firing) {
            col = COLOR_GUN_ON;             // zielony - strzela
        } else if (paused && usedInPattern) {
            // pauza: miganie zolty/czarny
            col = blinkOn ? COLOR_WARNING : COLOR_BG;
        } else if (usedInPattern) {
            col = COLOR_WARNING;            // zolty - uzyty we wzorcu
        } else {
            col = COLOR_GUN_OFF;            // szary - nieuzywany
        }

        tft.fillRect(rx, ry, rectW, rectH, col);
        tft.drawRect(rx, ry, rectW, rectH, COLOR_TEXT);

        // Etykieta P1..P6
        snprintf(lbl, sizeof(lbl), "P%d", i + 1);
        // Tekst ciemny na jasnym tle lub jasny na ciemnym
        if (col == COLOR_BG || col == COLOR_GUN_OFF) {
            tft.setTextColor(COLOR_TEXT, col);
        } else {
            tft.setTextColor(COLOR_BG, col);
        }
        tft.drawString(lbl, rx + rectW / 2, ry + rectH / 2);
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
//  EKRAN GLOWNY (HOME) - landscape 320x240
//  Bez clear() - uzywa setTextPadding do nadpisywania tresci
//  Layout:
//    Gora-lewo:  Wzorzec FSB24 + nazwa
//    Gora-prawo: Predkosc FSB24 + km/h + powierzchnia
//    Srodek:     Status "Gotowy"
//    Dol:        6 prostokatow pistoletow
// ============================================================
void DisplayManager::drawHomeScreen(const char* patCode, const char* patName,
                                    float speedKmh, float areaM2,
                                    const GunPatternCfg gunsCfg[6],
                                    bool reversed, bool hasReverse) {
    char buf[48];

    // ---- LEWY GORNY: Wzorzec (duzy, FSB24) u samej gory ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(180);
    tft.drawString(patCode, 8, 2);
    tft.setTextPadding(0);

    // Nazwa wzorca pod kodem
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(180);
    tft.drawString(patName, 8, 46);
    tft.setTextPadding(0);

    // [ODW] jesli odwrocony
    tft.fillRect(8, 64, 80, 18, COLOR_BG);
    if (reversed) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[ODW]", 8, 64);
    }

    // ---- PRAWY GORNY: Predkosc (duza, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%.1f", speedKmh);
    tft.setTextPadding(140);
    tft.drawString(buf, TFT_SCREEN_W - 8, 2);
    tft.setTextPadding(0);

    // Jednostka km/h
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(60);
    tft.drawString("km/h", TFT_SCREEN_W - 8, 46);
    tft.setTextPadding(0);

    // Powierzchnia
    tft.setFreeFont(FSB12);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextPadding(140);
    tft.drawString(buf, TFT_SCREEN_W - 8, 64);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- Separator ----
    tft.drawFastHLine(4, 86, TFT_SCREEN_W - 8, COLOR_DIVIDER);

    // ---- SRODEK: Status "Gotowy" nad prostokatami ----
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(240);
    tft.drawString("Gotowy", TFT_SCREEN_W / 2, 126);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- DOL: 6 prostokatow pistoletow ----
    bool homeGunStates[6] = {false, false, false, false, false, false};
    drawGunRects(TFT_SCREEN_H - 66, gunsCfg, homeGunStates, false);
}

// ============================================================
//  Wizualizacja wzorca - graficzne paski linia/przerwa
//  Rysuje 1 pasek na kazdy aktywny pistolet we wzorcu
//  Ciagle = pelny zielony pasek, Przerywane = segmenty
// ============================================================
void DisplayManager::drawPatternVisualization(int y, int maxH,
                                              const GunPatternCfg gunsCfg[6],
                                              bool reversed, bool gapStart) {
    // Efektywna konfiguracja (uwzglednia odwrocenie P1<->P3)
    GunPatternCfg cfg[NUM_GUNS];
    for (int i = 0; i < NUM_GUNS; i++) cfg[i] = gunsCfg[i];
    if (reversed) {
        GunPatternCfg tmp = cfg[GUN_P1];
        cfg[GUN_P1] = cfg[GUN_P3];
        cfg[GUN_P3] = tmp;
    }

    // Znajdz aktywne pistolety
    int activeIdx[NUM_GUNS];
    int activeCount = 0;
    for (int i = 0; i < NUM_GUNS; i++) {
        if (cfg[i].mode != GUN_OFF) {
            activeIdx[activeCount++] = i;
        }
    }
    if (activeCount == 0) return;

    // Wymiary paskow
    const int labelW = 28;
    const int barX = labelW + 4;
    const int barW = TFT_SCREEN_W - barX - 8;
    const int rowGap = 3;
    int barH = (maxH - (activeCount - 1) * rowGap) / activeCount;
    if (barH > 20) barH = 20;
    if (barH < 8)  barH = 8;

    // Czyszczenie calego obszaru wizualizacji (unika artefaktow)
    tft.fillRect(0, y, TFT_SCREEN_W, maxH, COLOR_BG);

    tft.setFreeFont(FM9);

    for (int a = 0; a < activeCount; a++) {
        int gi = activeIdx[a];
        int by = y + a * (barH + rowGap);

        // Etykieta pistoletu (P1..P6)
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "P%d", gi + 1);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(lbl, barX - 3, by + barH / 2);

        if (cfg[gi].mode == GUN_CONTINUOUS) {
            // Ciagla linia - pelny pasek zielony
            tft.fillRect(barX, by, barW, barH, COLOR_GUN_ON);
        } else if (cfg[gi].mode == GUN_DASHED) {
            float lineLen = cfg[gi].lineLen;
            float gapLen  = cfg[gi].gapLen;
            float cycle   = lineLen + gapLen;
            if (cycle <= 0) continue;

            // Tlo = kolor przerwy
            tft.fillRect(barX, by, barW, barH, COLOR_GUN_OFF);

            // Ile cykli pokazac (2-4 zaleznie od dlugosci cyklu)
            int numCycles = 2;
            if (cycle <= 3.0f) numCycles = 3;
            if (cycle <= 1.5f) numCycles = 4;
            float totalLen = cycle * numCycles;
            float scale = (float)barW / totalLen;

            // Rysuj segmenty linii
            // Przy gap start: przesuniecio o gapLen (zaczynamy od przerwy)
            float offset = gapStart ? gapLen : 0;
            float pos = offset;
            while (pos < totalLen) {
                int x1 = barX + (int)(pos * scale);
                int x2 = barX + (int)((pos + lineLen) * scale);
                if (x1 >= barX + barW) break;
                if (x2 > barX + barW) x2 = barX + barW;
                tft.fillRect(x1, by, x2 - x1, barH, COLOR_GUN_ON);
                pos += cycle;
            }
        }

        // Ramka paska
        tft.drawRect(barX, by, barW, barH, COLOR_DIVIDER);
    }

    tft.setTextDatum(TL_DATUM);
}

// ============================================================
//  EKRAN MALOWANIA - landscape 320x240
//  Bez clear() - uzywa setTextPadding do nadpisywania tresci
//  Layout:
//    Gora-lewo:  Wzorzec FSB24 + flagi [ODW] [GAP]
//    Gora-prawo: Predkosc FSB24 + km/h + powierzchnia
//    Srodek-gora: Wizualizacja graficzna wzorca (paski)
//    Srodek-dol:  Status malowania (mniejsza czcionka, blizej pistoletow)
//    Dol:         6 prostokatow pistoletow (zielony/zolty miganie)
// ============================================================
void DisplayManager::drawPaintingScreen(MachineState state, const char* patCode,
                                        float speedKmh, float areaM2,
                                        const GunPatternCfg gunsCfg[6],
                                        const bool gunStates[6],
                                        bool reversed, bool gapStart) {
    char buf[48];
    bool paused = (state == STATE_PAUSED);

    // ---- LEWY GORNY: Wzorzec (duzy, FSB24) u samej gory ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(180);
    tft.drawString(patCode, 8, 2);
    tft.setTextPadding(0);

    // Flagi [ODW] [GAP] pod kodem wzorca
    tft.setFreeFont(FS9);
    tft.setTextPadding(180);
    if (reversed && gapStart) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[ODW] [GAP]", 8, 46);
    } else if (reversed) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[ODW]", 8, 46);
    } else if (gapStart) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[GAP]", 8, 46);
    } else {
        tft.setTextColor(COLOR_BG, COLOR_BG);
        tft.drawString(" ", 8, 46);
    }
    tft.setTextPadding(0);

    // ---- PRAWY GORNY: Predkosc (duza, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%.1f", speedKmh);
    tft.setTextPadding(140);
    tft.drawString(buf, TFT_SCREEN_W - 8, 2);
    tft.setTextPadding(0);

    // km/h
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(60);
    tft.drawString("km/h", TFT_SCREEN_W - 8, 46);
    tft.setTextPadding(0);

    // Powierzchnia
    tft.setFreeFont(FSB12);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextPadding(140);
    tft.drawString(buf, TFT_SCREEN_W - 8, 64);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- Separator ----
    tft.drawFastHLine(4, 86, TFT_SCREEN_W - 8, COLOR_DIVIDER);

    // ---- SRODEK-GORA: Wizualizacja graficzna wzorca ----
    drawPatternVisualization(92, 52, gunsCfg, reversed, gapStart);

    // ---- Cienki separator pod wizualizacja ----
    tft.drawFastHLine(20, 148, TFT_SCREEN_W - 40, COLOR_DIVIDER);

    // ---- SRODEK-DOL: Status pracy (mniejsza czcionka, blizej pistoletow) ----
    uint16_t sc = stateColor(state);
    tft.setFreeFont(FSB12);
    tft.setTextColor(sc, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(280);
    char statusBuf[32];
    if (gapStart) {
        snprintf(statusBuf, sizeof(statusBuf), "%s [GAP]", stateStr(state));
    } else {
        snprintf(statusBuf, sizeof(statusBuf), "%s", stateStr(state));
    }
    tft.drawString(statusBuf, TFT_SCREEN_W / 2, 160);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- DOL: 6 prostokatow pistoletow ----
    drawGunRects(TFT_SCREEN_H - 66, gunsCfg, gunStates, paused);
}

// ============================================================
//  MENU SERWISOWE  (4 pozycje) - landscape
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

    const int itemH  = 38;
    const int startY = 36;

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
            tft.drawString(">", 8, iy + itemH / 2);
        }
        tft.drawString(labels[i], 24, iy + itemH / 2);

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + itemH - 1, TFT_SCREEN_W, COLOR_DIVIDER);
    }

    // --- Podpowiedzi ---
    int y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=dalej STOP=cofnij SEL(1s)=wejdz STOP(1s)=powrot", 6, y);
}

// ============================================================
//  KALIBRACJA ENKODERA - landscape
// ============================================================
void DisplayManager::drawCalibrationScreen(bool active, float pulses, float ppm, bool calibrated) {
    clear();
    drawHeader("KALIBRACJA ENKODERA");

    int y = 38;
    char buf[48];

    tft.setFreeFont(FS9);

    // Instrukcje
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Odmierz 10 m i przejdz maszyna po prostej.", 12, y);
    y += 22;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, COLOR_DIVIDER);
    y += 10;

    // Status
    if (active) {
        tft.setFreeFont(FSB18);
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("POMIAR...", TFT_SCREEN_W / 2, y + 18);
        tft.setTextDatum(TL_DATUM);
        y += 42;

        tft.setFreeFont(FS9);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString("Impulsy:", 12, y);
        snprintf(buf, sizeof(buf), "%.0f", pulses);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(buf, TFT_SCREEN_W - 12, y + 7);
        tft.setTextDatum(TL_DATUM);
    } else {
        tft.setFreeFont(FSB9);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("GOTOWY", TFT_SCREEN_W / 2, y + 18);
        tft.setTextDatum(TL_DATUM);
        y += 42;
    }

    y += 10;
    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, COLOR_DIVIDER);
    y += 10;

    // PPM i status w jednej linii
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Imp/metr:", 12, y);
    snprintf(buf, sizeof(buf), "%.1f", ppm);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(buf, 110, y);

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Status:", 190, y);
    if (calibrated) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.drawString("OK", 260, y);
    } else {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("Domyslny", 260, y);
    }

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    if (active) {
        tft.drawString("START=zakoncz pomiar  STOP(1s)=powrot", 6, y);
    } else {
        tft.drawString("START=rozpocznij pomiar  STOP(1s)=powrot", 6, y);
    }
}

// ============================================================
//  POMIAR DYSTANSU - landscape
// ============================================================
void DisplayManager::drawDistanceMeter(float distanceM, bool measuring) {
    clear();
    drawHeader("POMIAR DYSTANSU");

    char buf[48];

    // Status
    int y = 40;
    tft.setFreeFont(FSB9);
    tft.setTextColor(measuring ? COLOR_ACCENT : COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(measuring ? "POMIAR..." : "GOTOWY", TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    y += 20;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 16;

    // Duzy wynik
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

    y += 46;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 12;

    // cm
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "= %.0f cm", distanceM * 100.0f);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    if (measuring) {
        tft.drawString("START=pauza STOP=reset STOP(1s)=powrot", 6, y);
    } else {
        tft.drawString("START=pomiar STOP=reset STOP(1s)=powrot", 6, y);
    }
}

// ============================================================
//  RAPORTY - landscape
// ============================================================
void DisplayManager::drawReportsScreen(bool sdReady, int fileCount, const char* lastReport) {
    clear();
    drawHeader("RAPORTY");

    int y = 38;
    char buf[48];

    tft.setFreeFont(FS9);

    // SD + liczba plikow w jednej linii
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SD:", 12, y);
    if (sdReady) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.drawString("OK", 38, y);
    } else {
        tft.setTextColor(COLOR_ERROR, COLOR_BG);
        tft.drawString("BRAK", 38, y);
    }

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Plikow:", 100, y);
    snprintf(buf, sizeof(buf), "%d", fileCount);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(buf, 170, y);
    y += 22;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, COLOR_DIVIDER);
    y += 8;

    // Ostatni wpis
    tft.setFreeFont(FSB9);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString("Ostatni wpis:", 12, y);
    y += 22;

    tft.setFreeFont(FM9);
    if (lastReport && lastReport[0]) {
        char tmp[128];
        strncpy(tmp, lastReport, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = 0;

        // Wyswietl pola CSV w dwoch kolumnach
        char* tok = strtok(tmp, ",");
        int col1x = 12, col2x = 170;

        if (tok) { // data
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Data:", col1x, y);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(tok, col1x + 60, y);
        }
        tok = strtok(NULL, ",");
        if (tok) { // godzina
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Godz:", col2x, y);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(tok, col2x + 60, y);
        }
        y += 18;

        tok = strtok(NULL, ",");
        if (tok) { // wzorzec
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Wzorzec:", col1x, y);
            tft.setTextColor(COLOR_ACCENT, COLOR_BG);
            tft.drawString(tok, col1x + 90, y);
        }
        y += 18;

        tok = strtok(NULL, ",");
        if (tok) { // dystans
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Dyst:", col1x, y);
            snprintf(buf, sizeof(buf), "%s m", tok);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(buf, col1x + 60, y);
        }
        tok = strtok(NULL, ",");
        if (tok) { // powierzchnia
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Pow:", col2x, y);
            snprintf(buf, sizeof(buf), "%s m2", tok);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.drawString(buf, col2x + 60, y);
        }
    } else {
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString("Brak wpisow", 12, y);
    }

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("STOP(1s)=powrot", 6, y);
}

// ============================================================
//  CZYSZCZENIE DYSZ - landscape
// ============================================================
void DisplayManager::drawNozzleClean(const char* patCode, const char* patName,
                                     const GunPatternCfg gunsCfg[6],
                                     const bool gunStates[6]) {
    clear();
    drawHeader("CZYSZCZENIE DYSZ");

    int y = 36;

    // Wzorzec - lewy gorny
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(patCode, 8, y);

    // Nazwa - pod kodem
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString(patName, 8, y + 32);

    y += 56;
    tft.drawFastHLine(4, y, TFT_SCREEN_W - 8, COLOR_DIVIDER);

    // Podpowiedzi
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SEL=wzorzec TRZYMAJ START STOP(1s)=powrot", 8, y + 4);

    // Legenda kolorow
    tft.setTextColor(COLOR_WARNING, COLOR_BG);
    tft.drawString("Zolty=wzorzec", 8, y + 20);
    tft.setTextColor(COLOR_GUN_ON, COLOR_BG);
    tft.drawString("Zielony=strzela", 160, y + 20);

    // 6 prostokatow pistoletow
    drawGunRects(TFT_SCREEN_H - 66, gunsCfg, gunStates, false);
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
