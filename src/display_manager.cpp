// ============================================================
// TrassarV3 - Implementacja modulu wyswietlacza ILI9341
// 320x240 landscape, podswietlenie LEDC PWM
// v2.12.0 - Tryby pracy (AUTO/SEMI/MANUAL), wskaznik trybu
//           Anti-flicker na WSZYSTKICH ekranach (setTextPadding)
//           Pionowa wizualizacja wzorca (kolumny jak na drodze)
//           Layout 3-kolumnowy: info | viz | predkosc
//           Czas sesji na ekranie malowania
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
//  Layout 3-kolumnowy (identyczny ze Screen Painting):
//    Lewa (x=0..99):    Wzorzec FSB24, nazwa, [ODW], "Gotowy"
//    Srodek (x=100..219): Pionowa wizualizacja wzorca (kolumny)
//    Prawa (x=220..319):  Predkosc FSB24, km/h, powierzchnia
//    Dol:        6 prostokatow pistoletow
// ============================================================
void DisplayManager::drawHomeScreen(const char* patCode, const char* patName,
                                    float speedKmh, float areaM2,
                                    const GunPatternCfg gunsCfg[6],
                                    bool reversed, bool hasReverse) {
    char buf[48];

    // ---- SRODEK: Wizualizacja pionowa wzorca (rysuj NAJPIERW) ----
    drawPatternVisualization(100, 2, 120, 166, gunsCfg, reversed, false);

    // ---- LEWY GORNY: Wzorzec (duzy, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(92);
    tft.drawString(patCode, 8, 2);
    tft.setTextPadding(0);

    // ---- LEWA KOLUMNA: Nazwa, [ODW], Status pod kodem wzorca ----
    // Nazwa wzorca
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(92);
    tft.drawString(patName, 8, 38);
    tft.setTextPadding(0);

    // [ODW] jesli odwrocony
    tft.setFreeFont(FS9);
    tft.setTextPadding(92);
    if (reversed) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[ODW]", 8, 54);
    } else {
        tft.setTextColor(COLOR_BG, COLOR_BG);
        tft.drawString(" ", 8, 54);
    }
    tft.setTextPadding(0);

    // Status "Gotowy"
    tft.setFreeFont(FSB9);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextPadding(92);
    tft.drawString("Gotowy", 8, 72);
    tft.setTextPadding(0);

    // Tryb pracy
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(92);
    tft.drawString(modeStr(g_state.machineMode), 8, 92);
    tft.setTextPadding(0);

    // ---- PRAWY GORNY: Predkosc (duza, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%.1f", speedKmh);
    tft.setTextPadding(100);
    tft.drawString(buf, TFT_SCREEN_W - 8, 2);
    tft.setTextPadding(0);

    // km/h
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(100);
    tft.drawString("km/h", TFT_SCREEN_W - 8, 38);
    tft.setTextPadding(0);

    // Powierzchnia
    tft.setFreeFont(FSB12);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextPadding(100);
    tft.drawString(buf, TFT_SCREEN_W - 8, 56);
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
void DisplayManager::drawPatternVisualization(int vizX, int vizY, int vizW, int vizH,
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
    if (activeCount == 0) {
        tft.fillRect(vizX, vizY, vizW, vizH, COLOR_BG);
        return;
    }

    // Wymiary kolumn pionowych
    const int colGap = 16;     // odstep miedzy kolumnami
    const int labelH = 18;    // wysokosc etykiet nad kolumnami
    const int colY = vizY + labelH;
    const int colH = vizH - labelH;

    // Szerokosc kolumny: 36px dla szerokich (P4,P6), 20px dla waskich
    int colWidths[NUM_GUNS];
    int totalColW = 0;
    for (int a = 0; a < activeCount; a++) {
        int gi = activeIdx[a];
        colWidths[a] = (gi == GUN_P4 || gi == GUN_P6) ? 36 : 20;
        totalColW += colWidths[a];
    }
    totalColW += (activeCount - 1) * colGap;

    // Centrowanie kolumn w obszarze wizualizacji
    int colStartX = vizX + (vizW - totalColW) / 2;

    // Czyszczenie calego obszaru wizualizacji (unika artefaktow)
    tft.fillRect(vizX, vizY, vizW, vizH, COLOR_BG);

    tft.setFreeFont(FM9);

    int cx = colStartX;
    for (int a = 0; a < activeCount; a++) {
        int gi = activeIdx[a];
        int cw = colWidths[a];

        // Etykieta pistoletu (P1..P6) nad kolumna
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "P%d", gi + 1);
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(lbl, cx + cw / 2, vizY);

        if (cfg[gi].mode == GUN_CONTINUOUS) {
            // Ciagla linia - pelna kolumna zielona
            tft.fillRect(cx, colY, cw, colH, COLOR_GUN_ON);
        } else if (cfg[gi].mode == GUN_DASHED) {
            float lineLen = cfg[gi].lineLen;
            float gapLen  = cfg[gi].gapLen;
            float cycle   = lineLen + gapLen;
            if (cycle <= 0) { cx += cw + colGap; continue; }

            // Tlo = kolor przerwy
            tft.fillRect(cx, colY, cw, colH, COLOR_GUN_OFF);

            // Ile cykli pokazac (2-4 zaleznie od dlugosci cyklu)
            int numCycles = 2;
            if (cycle <= 3.0f) numCycles = 3;
            if (cycle <= 1.5f) numCycles = 4;
            float totalLen = cycle * numCycles;
            float scale = (float)colH / totalLen;

            // Rysuj segmenty linii pionowo (gora->dol = kierunek jazdy)
            float offset = gapStart ? gapLen : 0;
            float pos = offset;
            while (pos < totalLen) {
                int y1 = colY + (int)(pos * scale);
                int y2 = colY + (int)((pos + lineLen) * scale);
                if (y1 >= colY + colH) break;
                if (y2 > colY + colH) y2 = colY + colH;
                tft.fillRect(cx, y1, cw, y2 - y1, COLOR_GUN_ON);
                pos += cycle;
            }
        }

        // Ramka kolumny
        tft.drawRect(cx, colY, cw, colH, COLOR_DIVIDER);

        cx += cw + colGap;
    }

    tft.setTextDatum(TL_DATUM);
}

// ============================================================
//  EKRAN MALOWANIA - landscape 320x240
//  Bez clear() - uzywa setTextPadding do nadpisywania tresci
//  Layout 3-kolumnowy:
//    Lewa (x=0..99):    Wzorzec FSB24, [GAP], [ODW], Status
//    Srodek (x=100..219): Pionowa wizualizacja wzorca (kolumny)
//    Prawa (x=220..319):  Predkosc FSB24, km/h, powierzchnia
//    Dol:         6 prostokatow pistoletow (zielony/zolty miganie)
// ============================================================
void DisplayManager::drawPaintingScreen(MachineState state, const char* patCode,
                                        float speedKmh, float areaM2,
                                        const GunPatternCfg gunsCfg[6],
                                        const bool gunStates[6],
                                        bool reversed, bool gapStart,
                                        bool overspeed, bool lowSpeed,
                                        unsigned long sessionTimeSec,
                                        float sessionDistM) {
    char buf[48];
    bool paused = (state == STATE_PAUSED);

    // ============================================================
    //  Layout 3-kolumnowy bez separatorow:
    //   Lewa (x=0..99):   P-3a, [GAP], [ODW], Malowanie, czas, dystans
    //   Srodek (x=100..219): wizualizacja pionowa kolumn
    //   Prawa (x=220..319):  predkosc, km/h, powierzchnia
    //   Dol (y=174..240):    6 prostokatow pistoletow
    // ============================================================

    // ---- SRODEK: Wizualizacja pionowa wzorca (rysuj NAJPIERW) ----
    drawPatternVisualization(100, 2, 120, 166, gunsCfg, reversed, gapStart);

    // ---- LEWY GORNY: Wzorzec (duzy, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(92);
    tft.drawString(patCode, 8, 2);
    tft.setTextPadding(0);

    // ---- LEWA KOLUMNA: Flagi i status pod kodem wzorca ----
    tft.setFreeFont(FS9);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(92);

    // [GAP]
    if (gapStart) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[GAP]", 8, 38);
    } else {
        tft.setTextColor(COLOR_BG, COLOR_BG);
        tft.drawString(" ", 8, 38);
    }

    // [ODW]
    if (reversed) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("[ODW]", 8, 54);
    } else {
        tft.setTextColor(COLOR_BG, COLOR_BG);
        tft.drawString(" ", 8, 54);
    }
    tft.setTextPadding(0);

    // Status pracy (Malowanie / Pauza / Zatrzymany)
    uint16_t sc = stateColor(state);
    tft.setFreeFont(FSB9);
    tft.setTextColor(sc, COLOR_BG);
    tft.setTextPadding(92);
    tft.drawString(stateStr(state), 8, 72);
    tft.setTextPadding(0);

    // Czas sesji
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    fmtTime(sessionTimeSec, buf, sizeof(buf));
    tft.setTextPadding(92);
    tft.drawString(buf, 8, 94);
    tft.setTextPadding(0);

    // Dystans sesji
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    if (sessionDistM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", sessionDistM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", sessionDistM);
    }
    tft.setTextPadding(92);
    tft.drawString(buf, 8, 112);
    tft.setTextPadding(0);

    // Tryb pracy
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(92);
    tft.drawString(modeStr(g_state.machineMode), 8, 132);
    tft.setTextPadding(0);

    // ---- PRAWY GORNY: Predkosc (duza, FSB24) ----
    // Kolor predkosci: czerwony migajacy = overspeed, zolty = low speed, bialy = OK
    uint16_t speedColor = COLOR_TEXT;
    if (overspeed) {
        bool blinkPhase = ((millis() / 300) % 2) == 0;
        speedColor = blinkPhase ? COLOR_ERROR : COLOR_TEXT;
    } else if (lowSpeed) {
        speedColor = COLOR_WARNING;
    }

    tft.setFreeFont(FSB24);
    tft.setTextColor(speedColor, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%.1f", speedKmh);
    tft.setTextPadding(100);
    tft.drawString(buf, TFT_SCREEN_W - 8, 2);
    tft.setTextPadding(0);

    // km/h - etykieta tez migajaca przy overspeed
    tft.setFreeFont(FS9);
    tft.setTextColor(overspeed ? speedColor : COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(100);
    tft.drawString("km/h", TFT_SCREEN_W - 8, 38);
    tft.setTextPadding(0);

    // Powierzchnia
    tft.setFreeFont(FSB12);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextPadding(100);
    tft.drawString(buf, TFT_SCREEN_W - 8, 56);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- DOL: 6 prostokatow pistoletow ----
    drawGunRects(TFT_SCREEN_H - 66, gunsCfg, gunStates, paused);
}

// ============================================================
//  MENU SERWISOWE  (4 pozycje) - landscape, bez clear()
//  fillRect na kazdy item eliminuje miganie
// ============================================================
void DisplayManager::drawServiceMenu(int selectedIndex) {
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

    // Wyczysc reszte ekranu pod menu (unikniecie artefaktow)
    int bottomY = startY + 4 * itemH;
    tft.fillRect(0, bottomY, TFT_SCREEN_W, TFT_SCREEN_H - bottomY - 28, COLOR_BG);

    // --- Podpowiedzi ---
    int y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("SEL=dalej STOP=cofnij SEL(1s)=wejdz STOP(1s)=powrot", 6, y);
    tft.setTextPadding(0);
}

// ============================================================
//  KALIBRACJA ENKODERA - landscape, bez clear()
//  Stale pozycje Y + setTextPadding eliminuja miganie
// ============================================================
void DisplayManager::drawCalibrationScreen(bool active, float pulses, float ppm, bool calibrated) {
    drawHeader("KALIBRACJA ENKODERA");

    char buf[48];
    const int Y_INSTR = 38;
    const int Y_LINE1 = 60;
    const int Y_STATUS = 74;
    const int Y_PULSES = 118;
    const int Y_LINE2 = 138;
    const int Y_PPM = 148;

    tft.setFreeFont(FS9);

    // Instrukcje (staly tekst)
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 24);
    tft.drawString("Odmierz 10 m i przejdz maszyna po prostej.", 12, Y_INSTR);
    tft.setTextPadding(0);

    tft.drawFastHLine(12, Y_LINE1, TFT_SCREEN_W - 24, COLOR_DIVIDER);

    // Status - stala pozycja, nadpisywany tekst
    tft.setFreeFont(FSB18);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(200);
    if (active) {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("POMIAR...", TFT_SCREEN_W / 2, Y_STATUS + 18);
    } else {
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.drawString("GOTOWY", TFT_SCREEN_W / 2, Y_STATUS + 18);
    }
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // Impulsy - stala pozycja, nadpisywana wartosc
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(80);
    tft.drawString(active ? "Impulsy:" : " ", 12, Y_PULSES);
    tft.setTextPadding(0);

    if (active) {
        snprintf(buf, sizeof(buf), "%.0f", pulses);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MR_DATUM);
        tft.setTextPadding(120);
        tft.drawString(buf, TFT_SCREEN_W - 12, Y_PULSES + 7);
        tft.setTextPadding(0);
        tft.setTextDatum(TL_DATUM);
    } else {
        // Wyczysc obszar impulsow
        tft.fillRect(100, Y_PULSES, TFT_SCREEN_W - 112, 20, COLOR_BG);
    }

    tft.drawFastHLine(12, Y_LINE2, TFT_SCREEN_W - 24, COLOR_DIVIDER);

    // PPM i status w jednej linii
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Imp/metr:", 12, Y_PPM);
    snprintf(buf, sizeof(buf), "%.1f", ppm);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextPadding(70);
    tft.drawString(buf, 110, Y_PPM);
    tft.setTextPadding(0);

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Status:", 190, Y_PPM);
    tft.setTextPadding(60);
    if (calibrated) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.drawString("OK", 260, Y_PPM);
    } else {
        tft.setTextColor(COLOR_WARNING, COLOR_BG);
        tft.drawString("Domyslny", 260, Y_PPM);
    }
    tft.setTextPadding(0);

    // --- Podpowiedzi ---
    int y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    if (active) {
        tft.drawString("START=zakoncz pomiar  STOP(1s)=powrot", 6, y);
    } else {
        tft.drawString("START=rozpocznij pomiar  STOP(1s)=powrot", 6, y);
    }
    tft.setTextPadding(0);
}

// ============================================================
//  POMIAR DYSTANSU - landscape, bez clear()
// ============================================================
void DisplayManager::drawDistanceMeter(float distanceM, bool measuring) {
    drawHeader("POMIAR DYSTANSU");

    char buf[48];

    // Status - stala pozycja
    int y = 40;
    tft.setFreeFont(FSB9);
    tft.setTextColor(measuring ? COLOR_ACCENT : COLOR_MENU_TXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(160);
    tft.drawString(measuring ? "POMIAR..." : "GOTOWY", TFT_SCREEN_W / 2, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    y += 20;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 16;

    // Duzy wynik - nadpisywany z padding
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(240);
    if (distanceM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", distanceM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.2f m", distanceM);
    }
    tft.drawString(buf, TFT_SCREEN_W / 2, y + 16);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    y += 46;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, COLOR_DIVIDER);
    y += 12;

    // cm
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    snprintf(buf, sizeof(buf), "= %.0f cm", distanceM * 100.0f);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(200);
    tft.drawString(buf, TFT_SCREEN_W / 2, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    if (measuring) {
        tft.drawString("START=pauza STOP=reset STOP(1s)=powrot", 6, y);
    } else {
        tft.drawString("START=pomiar STOP=reset STOP(1s)=powrot", 6, y);
    }
    tft.setTextPadding(0);
}

// ============================================================
//  RAPORTY - landscape, bez clear()
//  Ekran statyczny - rysowany raz po forceFullRedraw
// ============================================================
void DisplayManager::drawReportsScreen(bool sdReady, int fileCount, const char* lastReport) {
    drawHeader("RAPORTY");

    int y = 38;
    char buf[48];

    tft.setFreeFont(FS9);

    // SD + liczba plikow w jednej linii
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("SD:", 12, y);
    tft.setTextPadding(50);
    if (sdReady) {
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.drawString("OK", 38, y);
    } else {
        tft.setTextColor(COLOR_ERROR, COLOR_BG);
        tft.drawString("BRAK", 38, y);
    }
    tft.setTextPadding(0);

    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.drawString("Plikow:", 100, y);
    snprintf(buf, sizeof(buf), "%d", fileCount);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextPadding(60);
    tft.drawString(buf, 170, y);
    tft.setTextPadding(0);
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
            tft.setTextPadding(100);
            tft.drawString(tok, col1x + 60, y);
            tft.setTextPadding(0);
        }
        tok = strtok(NULL, ",");
        if (tok) { // godzina
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Godz:", col2x, y);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.setTextPadding(80);
            tft.drawString(tok, col2x + 60, y);
            tft.setTextPadding(0);
        }
        y += 18;

        tok = strtok(NULL, ",");
        if (tok) { // wzorzec
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Wzorzec:", col1x, y);
            tft.setTextColor(COLOR_ACCENT, COLOR_BG);
            tft.setTextPadding(100);
            tft.drawString(tok, col1x + 90, y);
            tft.setTextPadding(0);
        }
        y += 18;

        tok = strtok(NULL, ",");
        if (tok) { // dystans
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Dyst:", col1x, y);
            snprintf(buf, sizeof(buf), "%s m", tok);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.setTextPadding(100);
            tft.drawString(buf, col1x + 60, y);
            tft.setTextPadding(0);
        }
        tok = strtok(NULL, ",");
        if (tok) { // powierzchnia
            tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
            tft.drawString("Pow:", col2x, y);
            snprintf(buf, sizeof(buf), "%s m2", tok);
            tft.setTextColor(COLOR_TEXT, COLOR_BG);
            tft.setTextPadding(80);
            tft.drawString(buf, col2x + 60, y);
            tft.setTextPadding(0);
        }
    } else {
        tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
        tft.setTextPadding(200);
        tft.drawString("Brak wpisow", 12, y);
        tft.setTextPadding(0);
    }

    // --- Podpowiedzi ---
    y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("STOP(1s)=powrot", 6, y);
    tft.setTextPadding(0);
}

// ============================================================
//  CZYSZCZENIE DYSZ - landscape, bez clear()
//  Dynamiczne: kod wzorca, nazwa, stany pistoletow
// ============================================================
void DisplayManager::drawNozzleClean(const char* patCode, const char* patName,
                                     const GunPatternCfg gunsCfg[6],
                                     const bool gunStates[6]) {
    drawHeader("CZYSZCZENIE DYSZ");

    int y = 36;

    // Wzorzec - lewy gorny (nadpisywany z padding)
    tft.setFreeFont(FSB18);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(200);
    tft.drawString(patCode, 8, y);
    tft.setTextPadding(0);

    // Nazwa - pod kodem
    tft.setFreeFont(FS9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(300);
    tft.drawString(patName, 8, y + 32);
    tft.setTextPadding(0);

    y += 56;
    tft.drawFastHLine(4, y, TFT_SCREEN_W - 8, COLOR_DIVIDER);

    // Podpowiedzi
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 16);
    tft.drawString("SEL=wzorzec TRZYMAJ START STOP(1s)=powrot", 8, y + 4);
    tft.setTextPadding(0);

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

const char* DisplayManager::modeStr(MachineMode m) {
    switch (m) {
        case MODE_AUTO:      return "[AUTO]";
        case MODE_SEMI_AUTO: return "[SEMI]";
        case MODE_MANUAL:    return "[RECZNY]";
        default:             return "[?]";
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

// ============================================================
//  EKRAN WYBORU TRYBU PRACY - landscape
//  3 tryby: Automatyczny, Polautomatyczny, Reczny
//  START(krotki) = zmiana opcji, START(dlugi) = zatwierdzenie
// ============================================================
void DisplayManager::drawModeSelect(int selectedMode, MachineMode currentMode) {
    drawHeader("WYBOR TRYBU PRACY");

    static const char* labels[3] = {
        "Automatyczny",
        "Polautomatyczny",
        "Reczny"
    };
    static const char* descs[3] = {
        "Pelna automatyka - dystans steruje pistoletami",
        "Auto linia, reczna przerwa (START = nast. linia)",
        "Trzymaj START = strzal (jak czyszczenie dysz)"
    };

    const int itemH  = 48;
    const int startY = 36;

    for (int i = 0; i < 3; i++) {
        int iy = startY + i * itemH;
        bool sel = (i == selectedMode);
        bool cur = (i == (int)currentMode);
        uint16_t bg = sel ? COLOR_MENU_SEL : COLOR_BG;
        uint16_t fg = sel ? COLOR_TEXT      : COLOR_MENU_TXT;

        tft.fillRect(0, iy, TFT_SCREEN_W, itemH, bg);

        // Wskaznik zaznaczenia
        tft.setFreeFont(FSB9);
        tft.setTextColor(fg, bg);
        tft.setTextDatum(ML_DATUM);
        if (sel) {
            tft.drawString(">", 8, iy + itemH / 2 - 6);
        }

        // Nazwa trybu
        tft.setFreeFont(FSB9);
        tft.setTextColor(fg, bg);
        tft.drawString(labels[i], 24, iy + itemH / 2 - 6);

        // Aktualny tryb - znacznik
        if (cur) {
            tft.setTextColor(COLOR_ACCENT, bg);
            tft.drawString("*", TFT_SCREEN_W - 24, iy + itemH / 2 - 6);
        }

        // Opis
        tft.setFreeFont(FM9);
        tft.setTextColor(sel ? COLOR_MENU_TXT : COLOR_DIVIDER, bg);
        tft.drawString(descs[i], 24, iy + itemH / 2 + 10);

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + itemH - 1, TFT_SCREEN_W, COLOR_DIVIDER);
    }

    // Wyczysc reszte pod menu
    int bottomY = startY + 3 * itemH;
    tft.fillRect(0, bottomY, TFT_SCREEN_W, TFT_SCREEN_H - bottomY - 28, COLOR_BG);

    // --- Podpowiedzi ---
    int y = TFT_SCREEN_H - 22;
    tft.setFreeFont(FM9);
    tft.setTextColor(COLOR_MENU_TXT, COLOR_BG);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START=zmien  START(1s)=zatwierdz  STOP=powrot", 6, y);
    tft.setTextPadding(0);
}
