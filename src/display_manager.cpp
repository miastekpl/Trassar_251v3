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

// ============ Stale layoutu wyswietlacza ============

// Ogolne marginesy
#define MARGIN_X            8       // Margines boczny lewy/prawy [px]
#define HINT_X              6       // X paska podpowiedzi

// Naglowek
#define HDR_H               30      // Wysokosc naglowka [px]
#define HDR_TEXT_CY         15      // Y srodka tekstu naglowka

// Layout 3-kolumnowy (HOME / PAINTING) — padding tekstu
#define COL_L_PAD           92      // setTextPadding lewej kolumny
#define COL_R_PAD           100     // setTextPadding prawej kolumny

// Wizualizacja wzorca (srodkowa kolumna)
#define VIZ_X               100     // X poczatek obszaru wizualizacji
#define VIZ_Y               2       // Y poczatek
#define VIZ_W               120     // Szerokosc
#define VIZ_H               166     // Wysokosc

// Pozycje Y elementow — lewa kolumna
#define ROW_PAT_Y           2       // Kod wzorca (FSB24)
#define ROW_NAME_Y          38      // Nazwa wzorca / flaga [GAP]
#define ROW_FLAG_Y          54      // Flaga [ODW]
#define ROW_STATUS_Y        72      // Status "Gotowy" / "Malowanie"
#define ROW_MODE_Y          92      // Tryb pracy na ekranie HOME
#define ROW_TIME_Y          94      // Czas sesji (PAINTING)
#define ROW_DIST_Y          112     // Dystans sesji (PAINTING)
#define ROW_MODE2_Y         132     // Tryb pracy na ekranie PAINTING

// Pozycje Y elementow — prawa kolumna
#define ROW_SPEED_Y         2       // Predkosc (FSB24)
#define ROW_UNIT_Y          38      // Etykieta "km/h"
#define ROW_AREA_Y          56      // Powierzchnia (FSB12)

// Dol ekranu
#define GUN_RECTS_Y         (TFT_SCREEN_H - 66)    // Y prostokatow pistoletow
#define HINT_Y              (TFT_SCREEN_H - 22)     // Y paska podpowiedzi

// Prostokaty pistoletow
#define GUN_W               42      // Szerokosc prostokata
#define GUN_H               50      // Wysokosc prostokata
#define GUN_GAP             6       // Odstep miedzy prostokatami

// Wizualizacja wzorca — parametry kolumn
#define VIZ_COL_GAP         16      // Odstep miedzy kolumnami
#define VIZ_LABEL_H         18      // Wysokosc etykiety nad kolumna
#define VIZ_COL_WIDE        36      // Szerokosc kolumny szerokich pistoletow (P4, P6)
#define VIZ_COL_NARROW      20      // Szerokosc kolumny waskich pistoletow

// Menu serwisowe
#define SMENU_ITEM_H        30      // Wysokosc pozycji menu [px]
#define SMENU_START_Y       34      // Y pierwszej pozycji
#define SMENU_INDENT        24      // X wciecie tekstu
#define SMENU_MARKER_X      8       // X wskaznika ">"
#define SMENU_COUNT         6       // Liczba pozycji menu

// Ekran splasha
#define SPLASH_TITLE_OFS    (-40)   // Offset Y tytulu od srodka ekranu
#define SPLASH_SUB_OFS      10      // Offset Y podtytulu
#define SPLASH_VER_OFS      40      // Offset Y wersji
#define SPLASH_INIT_OFS     70      // Offset Y "Inicjalizacja..."

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
    tft.fillScreen(cBg);
    tft.setTextColor(cText, cBg);

    // ----- Ekran powitalny (splash) -----
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    tft.setFreeFont(FSB24);
    tft.drawString("TrassarV3", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + SPLASH_TITLE_OFS);

    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Malowarka drogowa", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + SPLASH_SUB_OFS);

    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "v%s", FW_VERSION);
    tft.setTextColor(cAccent, cBg);
    tft.drawString(verBuf, TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + SPLASH_VER_OFS);

    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Inicjalizacja...", TFT_SCREEN_W / 2, TFT_SCREEN_H / 2 + SPLASH_INIT_OFS);

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
    tft.fillScreen(cBg);
}

// ============================================================
//  Elementy pomocnicze
// ============================================================

void DisplayManager::drawHeader(const char* title) {
    tft.fillRect(0, 0, TFT_SCREEN_W, HDR_H, cHeaderBg);
    tft.setFreeFont(FSB9);
    tft.setTextColor(cHeaderTxt, cHeaderBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, TFT_SCREEN_W / 2, HDR_TEXT_CY);
    tft.setTextDatum(TL_DATUM);
}

// ============================================================
//  drawGunRects - 6 prostokatow pistoletow na dole ekranu
//  Kolory: zolty=we wzorcu, zielony=strzela, zolty miganie=pauza, szary=nieuzywany
// ============================================================
void DisplayManager::drawGunRects(int y, const GunPatternCfg gunsCfg[6],
                                   const bool gunStates[6], bool paused) {
    const int totalW = NUM_GUNS * GUN_W + (NUM_GUNS - 1) * GUN_GAP;
    const int startX = (TFT_SCREEN_W - totalW) / 2;

    bool blinkOn = ((millis() / 500) % 2) == 0;

    tft.setFreeFont(FSB9);
    tft.setTextDatum(MC_DATUM);

    char lbl[4];
    for (int i = 0; i < NUM_GUNS; i++) {
        int rx = startX + i * (GUN_W + GUN_GAP);
        int ry = y;
        bool usedInPattern = (gunsCfg[i].mode != GUN_OFF);
        bool firing = gunStates ? gunStates[i] : false;

        uint16_t col;
        if (firing) {
            col = cGunOn;             // zielony - strzela
        } else if (paused && usedInPattern) {
            // pauza: miganie zolty/czarny
            col = blinkOn ? cWarning : cBg;
        } else if (usedInPattern) {
            col = cWarning;            // zolty - uzyty we wzorcu
        } else {
            col = cGunOff;            // szary - nieuzywany
        }

        tft.fillRect(rx, ry, GUN_W, GUN_H, col);
        tft.drawRect(rx, ry, GUN_W, GUN_H, cText);

        // Etykieta P1..P6
        snprintf(lbl, sizeof(lbl), "P%d", i + 1);
        // Tekst ciemny na jasnym tle lub jasny na ciemnym
        if (col == cBg || col == cGunOff) {
            tft.setTextColor(cText, col);
        } else {
            tft.setTextColor(cBg, col);
        }
        tft.drawString(lbl, rx + GUN_W / 2, ry + GUN_H / 2);
    }
    tft.setTextDatum(TL_DATUM);
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
    if (percent < 0)   percent = 0;
    if (percent > 100)  percent = 100;

    tft.drawRect(x, y, w, h, cDivider);
    int fillW = (w - 2) * percent / 100;
    if (fillW > 0) {
        tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
    }
    if (fillW < w - 2) {
        tft.fillRect(x + 1 + fillW, y + 1, w - 2 - fillW, h - 2, cBg);
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
    drawPatternVisualization(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, gunsCfg, reversed, false);

    // ---- LEWY GORNY: Wzorzec (duzy, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(cAccent, cBg);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(patCode, MARGIN_X, ROW_PAT_Y);
    tft.setTextPadding(0);

    // ---- LEWA KOLUMNA: Nazwa, [ODW], Status pod kodem wzorca ----
    // Nazwa wzorca
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(patName, MARGIN_X, ROW_NAME_Y);
    tft.setTextPadding(0);

    // [ODW] jesli odwrocony
    tft.setFreeFont(FS9);
    tft.setTextPadding(COL_L_PAD);
    if (reversed) {
        tft.setTextColor(cWarning, cBg);
        tft.drawString("[ODW]", MARGIN_X, ROW_FLAG_Y);
    } else {
        tft.setTextColor(cBg, cBg);
        tft.drawString(" ", MARGIN_X, ROW_FLAG_Y);
    }
    tft.setTextPadding(0);

    // Status "Gotowy"
    tft.setFreeFont(FSB9);
    tft.setTextColor(cAccent, cBg);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString("Gotowy", MARGIN_X, ROW_STATUS_Y);
    tft.setTextPadding(0);

    // Tryb pracy
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(modeStr(g_state.machineMode), MARGIN_X, ROW_MODE_Y);
    tft.setTextPadding(0);

    // ---- PRAWY GORNY: Predkosc (duza, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(cText, cBg);
    tft.setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%.1f", speedKmh);
    tft.setTextPadding(COL_R_PAD);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, ROW_SPEED_Y);
    tft.setTextPadding(0);

    // km/h
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(COL_R_PAD);
    tft.drawString("km/h", TFT_SCREEN_W - MARGIN_X, ROW_UNIT_Y);
    tft.setTextPadding(0);

    // Powierzchnia
    tft.setFreeFont(FSB12);
    tft.setTextColor(cText, cBg);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextPadding(COL_R_PAD);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, ROW_AREA_Y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- DOL: 6 prostokatow pistoletow ----
    bool homeGunStates[6] = {false, false, false, false, false, false};
    drawGunRects(GUN_RECTS_Y, gunsCfg, homeGunStates, false);
}

// ============================================================
//  Wizualizacja wzorca - graficzne paski linia/przerwa
//  Rysuje 1 pasek na kazdy aktywny pistolet we wzorcu
//  Ciagle = pelny zielony pasek, Przerywane = segmenty
// ============================================================
void DisplayManager::drawPatternVisualization(int vizX, int vizY, int vizW, int vizH,
                                              const GunPatternCfg gunsCfg[6],
                                              bool reversed, bool gapStart,
                                              float positionM) {
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
        tft.fillRect(vizX, vizY, vizW, vizH, cBg);
        return;
    }

    // Wymiary kolumn pionowych
    const int colY = vizY + VIZ_LABEL_H;
    const int colH = vizH - VIZ_LABEL_H;

    // Szerokosc kolumny: szerokie (P4,P6) vs waskie
    int colWidths[NUM_GUNS];
    int totalColW = 0;
    for (int a = 0; a < activeCount; a++) {
        int gi = activeIdx[a];
        colWidths[a] = (gi == GUN_P4 || gi == GUN_P6) ? VIZ_COL_WIDE : VIZ_COL_NARROW;
        totalColW += colWidths[a];
    }
    totalColW += (activeCount - 1) * VIZ_COL_GAP;

    // Centrowanie kolumn w obszarze wizualizacji
    int colStartX = vizX + (vizW - totalColW) / 2;

    // Czyszczenie calego obszaru wizualizacji (unika artefaktow)
    tft.fillRect(vizX, vizY, vizW, vizH, cBg);

    tft.setFreeFont(FM9);

    int cx = colStartX;
    for (int a = 0; a < activeCount; a++) {
        int gi = activeIdx[a];
        int cw = colWidths[a];

        // Etykieta pistoletu (P1..P6) nad kolumna
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "P%d", gi + 1);
        tft.setTextColor(cMenuTxt, cBg);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(lbl, cx + cw / 2, vizY);

        if (cfg[gi].mode == GUN_CONTINUOUS) {
            // Ciagla linia - pelna kolumna zielona
            tft.fillRect(cx, colY, cw, colH, cGunOn);
        } else if (cfg[gi].mode == GUN_DASHED) {
            float lineLen = cfg[gi].lineLen;
            float gapLen  = cfg[gi].gapLen;
            float cycle   = lineLen + gapLen;
            if (cycle <= 0) { cx += cw + VIZ_COL_GAP; continue; }

            // Tlo = kolor przerwy
            tft.fillRect(cx, colY, cw, colH, cGunOff);

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
                tft.fillRect(cx, y1, cw, y2 - y1, cGunOn);
                pos += cycle;
            }
        }

        // Ramka kolumny
        tft.drawRect(cx, colY, cw, colH, cDivider);

        cx += cw + VIZ_COL_GAP;
    }

    // --- Wskaznik pozycji na zywo (pozioma linia) ---
    if (positionM >= 0 && activeCount > 0) {
        // Znajdz najdluzszy cykl wsrod aktywnych pistoletow DASHED
        float maxCycle = 0;
        for (int a = 0; a < activeCount; a++) {
            int gi = activeIdx[a];
            if (cfg[gi].mode == GUN_DASHED) {
                float c = cfg[gi].lineLen + cfg[gi].gapLen;
                if (c > maxCycle) maxCycle = c;
            }
        }
        if (maxCycle > 0) {
            int numCycles = 2;
            if (maxCycle <= 3.0f) numCycles = 3;
            if (maxCycle <= 1.5f) numCycles = 4;
            float totalLen = maxCycle * numCycles;
            float scale = (float)colH / totalLen;

            // Pozycja w cyklu (zawijanie)
            float posInViz = fmodf(positionM, totalLen);
            int markerY = colY + (int)(posInViz * scale);

            // Rysuj poziomy marker na calej szerokosci wizualizacji
            if (markerY >= colY && markerY < colY + colH - 1) {
                tft.drawFastHLine(vizX + 2, markerY, vizW - 4, cError);
                tft.drawFastHLine(vizX + 2, markerY + 1, vizW - 4, cError);
                // Maly trojkat po lewej jako wskaznik
                tft.fillTriangle(vizX, markerY - 3, vizX, markerY + 3, vizX + 5, markerY, cError);
            }
        }
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
                                        float sessionDistM,
                                        float patternPosM) {
    char buf[48];
    bool paused = (state == STATE_PAUSED);

    // ============================================================
    //  Layout 3-kolumnowy bez separatorow:
    //   Lewa (x=0..99):   P-3a, [GAP], [ODW], Malowanie, czas, dystans
    //   Srodek (x=100..219): wizualizacja pionowa kolumn
    //   Prawa (x=220..319):  predkosc, km/h, powierzchnia
    //   Dol (y=174..240):    6 prostokatow pistoletow
    // ============================================================

    // ---- SRODEK: Wizualizacja pionowa wzorca z podgladem pozycji ----
    drawPatternVisualization(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, gunsCfg, reversed, gapStart, patternPosM);

    // ---- LEWY GORNY: Wzorzec (duzy, FSB24) ----
    tft.setFreeFont(FSB24);
    tft.setTextColor(cAccent, cBg);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(patCode, MARGIN_X, ROW_PAT_Y);
    tft.setTextPadding(0);

    // ---- LEWA KOLUMNA: Flagi i status pod kodem wzorca ----
    tft.setFreeFont(FS9);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(COL_L_PAD);

    // [GAP]
    if (gapStart) {
        tft.setTextColor(cWarning, cBg);
        tft.drawString("[GAP]", MARGIN_X, ROW_NAME_Y);
    } else {
        tft.setTextColor(cBg, cBg);
        tft.drawString(" ", MARGIN_X, ROW_NAME_Y);
    }

    // [ODW]
    if (reversed) {
        tft.setTextColor(cWarning, cBg);
        tft.drawString("[ODW]", MARGIN_X, ROW_FLAG_Y);
    } else {
        tft.setTextColor(cBg, cBg);
        tft.drawString(" ", MARGIN_X, ROW_FLAG_Y);
    }
    tft.setTextPadding(0);

    // Status pracy (Malowanie / Pauza / Zatrzymany)
    uint16_t sc = stateColor(state);
    tft.setFreeFont(FSB9);
    tft.setTextColor(sc, cBg);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(stateStr(state), MARGIN_X, ROW_STATUS_Y);
    tft.setTextPadding(0);

    // Czas sesji
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    fmtTime(sessionTimeSec, buf, sizeof(buf));
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(buf, MARGIN_X, ROW_TIME_Y);
    tft.setTextPadding(0);

    // Dystans sesji
    tft.setFreeFont(FS9);
    tft.setTextColor(cText, cBg);
    if (sessionDistM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", sessionDistM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", sessionDistM);
    }
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(buf, MARGIN_X, ROW_DIST_Y);
    tft.setTextPadding(0);

    // Tryb pracy
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(COL_L_PAD);
    tft.drawString(modeStr(g_state.machineMode), MARGIN_X, ROW_MODE2_Y);
    tft.setTextPadding(0);

    // ---- PRAWY GORNY: Predkosc (duza, FSB24) ----
    // Kolor predkosci: czerwony migajacy = overspeed, zolty = low speed, bialy = OK
    uint16_t speedColor = cText;
    if (overspeed) {
        bool blinkPhase = ((millis() / 300) % 2) == 0;
        speedColor = blinkPhase ? cError : cText;
    } else if (lowSpeed) {
        speedColor = cWarning;
    }

    tft.setFreeFont(FSB24);
    tft.setTextColor(speedColor, cBg);
    tft.setTextDatum(TR_DATUM);
    snprintf(buf, sizeof(buf), "%.1f", speedKmh);
    tft.setTextPadding(COL_R_PAD);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, ROW_SPEED_Y);
    tft.setTextPadding(0);

    // km/h - etykieta tez migajaca przy overspeed
    tft.setFreeFont(FS9);
    tft.setTextColor(overspeed ? speedColor : cMenuTxt, cBg);
    tft.setTextPadding(COL_R_PAD);
    tft.drawString("km/h", TFT_SCREEN_W - MARGIN_X, ROW_UNIT_Y);
    tft.setTextPadding(0);

    // Powierzchnia
    tft.setFreeFont(FSB12);
    tft.setTextColor(cText, cBg);
    snprintf(buf, sizeof(buf), "%.1f m2", areaM2);
    tft.setTextPadding(COL_R_PAD);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, ROW_AREA_Y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // ---- DOL: 6 prostokatow pistoletow ----
    drawGunRects(GUN_RECTS_Y, gunsCfg, gunStates, paused);
}

// ============================================================
//  MENU SERWISOWE  (4 pozycje) - landscape, bez clear()
//  fillRect na kazdy item eliminuje miganie
// ============================================================
void DisplayManager::drawServiceMenu(int selectedIndex) {
    drawHeader("SERWIS");

    static const char* labels[SMENU_COUNT] = {
        "Kalibracja enkodera",
        "Pomiar dystansu",
        "Raporty",
        "Czyszczenie dysz",
        "Reset etapu",
        "Reset licznikow"
    };

    tft.setFreeFont(FS9);

    for (int i = 0; i < SMENU_COUNT; i++) {
        int iy = SMENU_START_Y + i * SMENU_ITEM_H;
        bool sel = (i == selectedIndex);
        uint16_t bg = sel ? cMenuSel : cBg;
        uint16_t fg = sel ? cText      : cMenuTxt;

        tft.fillRect(0, iy, TFT_SCREEN_W, SMENU_ITEM_H, bg);
        tft.setTextColor(fg, bg);
        tft.setTextDatum(ML_DATUM);

        if (sel) {
            tft.drawString(">", SMENU_MARKER_X, iy + SMENU_ITEM_H / 2);
        }
        tft.drawString(labels[i], SMENU_INDENT, iy + SMENU_ITEM_H / 2);

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + SMENU_ITEM_H - 1, TFT_SCREEN_W, cDivider);
    }

    // Wyczysc reszte ekranu pod menu (unikniecie artefaktow)
    int bottomY = SMENU_START_Y + SMENU_COUNT * SMENU_ITEM_H;
    if (bottomY < HINT_Y) {
        tft.fillRect(0, bottomY, TFT_SCREEN_W, HINT_Y - bottomY, cBg);
    }

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("SEL=dalej STOP=cofnij SEL(1s)=wejdz STOP(1s)=powrot", HINT_X, HINT_Y);
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
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 24);
    tft.drawString("Odmierz 10 m i przejdz maszyna po prostej.", 12, Y_INSTR);
    tft.setTextPadding(0);

    tft.drawFastHLine(12, Y_LINE1, TFT_SCREEN_W - 24, cDivider);

    // Status - stala pozycja, nadpisywany tekst
    tft.setFreeFont(FSB18);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(200);
    if (active) {
        tft.setTextColor(cWarning, cBg);
        tft.drawString("POMIAR...", TFT_SCREEN_W / 2, Y_STATUS + 18);
    } else {
        tft.setTextColor(cMenuTxt, cBg);
        tft.drawString("GOTOWY", TFT_SCREEN_W / 2, Y_STATUS + 18);
    }
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // Impulsy - stala pozycja, nadpisywana wartosc
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(80);
    tft.drawString(active ? "Impulsy:" : " ", 12, Y_PULSES);
    tft.setTextPadding(0);

    if (active) {
        snprintf(buf, sizeof(buf), "%.0f", pulses);
        tft.setTextColor(cAccent, cBg);
        tft.setTextDatum(MR_DATUM);
        tft.setTextPadding(120);
        tft.drawString(buf, TFT_SCREEN_W - 12, Y_PULSES + 7);
        tft.setTextPadding(0);
        tft.setTextDatum(TL_DATUM);
    } else {
        // Wyczysc obszar impulsow
        tft.fillRect(100, Y_PULSES, TFT_SCREEN_W - 112, 20, cBg);
    }

    tft.drawFastHLine(12, Y_LINE2, TFT_SCREEN_W - 24, cDivider);

    // PPM i status w jednej linii
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Imp/metr:", 12, Y_PPM);
    snprintf(buf, sizeof(buf), "%.1f", ppm);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(70);
    tft.drawString(buf, 110, Y_PPM);
    tft.setTextPadding(0);

    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Status:", 190, Y_PPM);
    tft.setTextPadding(60);
    if (calibrated) {
        tft.setTextColor(cAccent, cBg);
        tft.drawString("OK", 260, Y_PPM);
    } else {
        tft.setTextColor(cWarning, cBg);
        tft.drawString("Domyslny", 260, Y_PPM);
    }
    tft.setTextPadding(0);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    if (active) {
        tft.drawString("START=zakoncz pomiar  STOP(1s)=powrot", HINT_X, HINT_Y);
    } else {
        tft.drawString("START=rozpocznij pomiar  STOP(1s)=powrot", HINT_X, HINT_Y);
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
    tft.setTextColor(measuring ? cAccent : cMenuTxt, cBg);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(160);
    tft.drawString(measuring ? "POMIAR..." : "GOTOWY", TFT_SCREEN_W / 2, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    y += 20;
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, cDivider);
    y += 16;

    // Duzy wynik - nadpisywany z padding
    tft.setFreeFont(FSB18);
    tft.setTextColor(cText, cBg);
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
    tft.drawFastHLine(20, y, TFT_SCREEN_W - 40, cDivider);
    y += 12;

    // cm
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    snprintf(buf, sizeof(buf), "= %.0f cm", distanceM * 100.0f);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(200);
    tft.drawString(buf, TFT_SCREEN_W / 2, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    if (measuring) {
        tft.drawString("START=pauza STOP=reset STOP(1s)=powrot", HINT_X, HINT_Y);
    } else {
        tft.drawString("START=pomiar STOP=reset STOP(1s)=powrot", HINT_X, HINT_Y);
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
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("SD:", 12, y);
    tft.setTextPadding(50);
    if (sdReady) {
        tft.setTextColor(cAccent, cBg);
        tft.drawString("OK", 38, y);
    } else {
        tft.setTextColor(cError, cBg);
        tft.drawString("BRAK", 38, y);
    }
    tft.setTextPadding(0);

    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Plikow:", 100, y);
    snprintf(buf, sizeof(buf), "%d", fileCount);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(60);
    tft.drawString(buf, 170, y);
    tft.setTextPadding(0);
    y += 22;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 8;

    // Ostatni wpis
    tft.setFreeFont(FSB9);
    tft.setTextColor(cAccent, cBg);
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
            tft.setTextColor(cMenuTxt, cBg);
            tft.drawString("Data:", col1x, y);
            tft.setTextColor(cText, cBg);
            tft.setTextPadding(100);
            tft.drawString(tok, col1x + 60, y);
            tft.setTextPadding(0);
        }
        tok = strtok(NULL, ",");
        if (tok) { // godzina
            tft.setTextColor(cMenuTxt, cBg);
            tft.drawString("Godz:", col2x, y);
            tft.setTextColor(cText, cBg);
            tft.setTextPadding(80);
            tft.drawString(tok, col2x + 60, y);
            tft.setTextPadding(0);
        }
        y += 18;

        tok = strtok(NULL, ",");
        if (tok) { // wzorzec
            tft.setTextColor(cMenuTxt, cBg);
            tft.drawString("Wzorzec:", col1x, y);
            tft.setTextColor(cAccent, cBg);
            tft.setTextPadding(100);
            tft.drawString(tok, col1x + 90, y);
            tft.setTextPadding(0);
        }
        y += 18;

        tok = strtok(NULL, ",");
        if (tok) { // dystans
            tft.setTextColor(cMenuTxt, cBg);
            tft.drawString("Dyst:", col1x, y);
            snprintf(buf, sizeof(buf), "%s m", tok);
            tft.setTextColor(cText, cBg);
            tft.setTextPadding(100);
            tft.drawString(buf, col1x + 60, y);
            tft.setTextPadding(0);
        }
        tok = strtok(NULL, ",");
        if (tok) { // powierzchnia
            tft.setTextColor(cMenuTxt, cBg);
            tft.drawString("Pow:", col2x, y);
            snprintf(buf, sizeof(buf), "%s m2", tok);
            tft.setTextColor(cText, cBg);
            tft.setTextPadding(80);
            tft.drawString(buf, col2x + 60, y);
            tft.setTextPadding(0);
        }
    } else {
        tft.setTextColor(cMenuTxt, cBg);
        tft.setTextPadding(200);
        tft.drawString("Brak wpisow", 12, y);
        tft.setTextPadding(0);
    }

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("STOP(1s)=powrot", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  CZYSZCZENIE DYSZ - landscape, bez clear()
//  Layout 2-kolumnowy: info lewa + wizualizacja prawa
//  Dynamiczne: kod wzorca, nazwa, podglad wzorca, stany pistoletow
// ============================================================
void DisplayManager::drawNozzleClean(const char* patCode, const char* patName,
                                     const GunPatternCfg gunsCfg[6],
                                     const bool gunStates[6]) {
    drawHeader("CZYSZCZENIE DYSZ");

    // ---- LEWA KOLUMNA: wzorzec + podpowiedzi ----
    int y = 36;

    // Wzorzec - lewy gorny (nadpisywany z padding)
    tft.setFreeFont(FSB18);
    tft.setTextColor(cAccent, cBg);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(150);
    tft.drawString(patCode, 8, y);
    tft.setTextPadding(0);

    // Nazwa - pod kodem
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(150);
    tft.drawString(patName, 8, y + 32);
    tft.setTextPadding(0);

    y += 56;
    tft.drawFastHLine(4, y, 152, cDivider);

    // Podpowiedzi (pionowo, lewa strona)
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(148);
    tft.drawString("SEL=wzorzec", MARGIN_X, y + 4);
    tft.drawString("TRZYMAJ START", MARGIN_X, y + 20);
    tft.drawString("STOP(1s)=powrot", MARGIN_X, y + 36);
    tft.setTextPadding(0);

    // Legenda kolorow
    tft.setTextColor(cWarning, cBg);
    tft.setTextPadding(70);
    tft.drawString("Zol=wz", MARGIN_X, y + 56);
    tft.setTextPadding(0);
    tft.setTextColor(cGunOn, cBg);
    tft.setTextPadding(78);
    tft.drawString("Ziel=strz", 80, y + 56);
    tft.setTextPadding(0);

    // ---- PRAWA KOLUMNA: podglad wzorca ----
    const int CLEAN_VIZ_X = 164;
    const int CLEAN_VIZ_Y = 32;
    const int CLEAN_VIZ_W = 152;
    const int CLEAN_VIZ_H = 136;
    drawPatternVisualization(CLEAN_VIZ_X, CLEAN_VIZ_Y, CLEAN_VIZ_W, CLEAN_VIZ_H,
                             gunsCfg, false, false);

    // ---- DOL: 6 prostokatow pistoletow ----
    drawGunRects(GUN_RECTS_Y, gunsCfg, gunStates, false);
}

// ============================================================
//  EKRAN RESETU ETAPU - potwierdzenie zerowania licznikow sesji
// ============================================================
void DisplayManager::drawSessionResetScreen(float distM, float areaM2, unsigned long timeSec) {
    drawHeader("RESET ETAPU");

    char buf[48];
    int y = SMENU_START_Y + 4;

    // Opis
    tft.setFreeFont(FS9);
    tft.setTextColor(cText, cBg);
    tft.drawString("Biezacy etap pracy:", MARGIN_X + 4, y);
    y += 24;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 10;

    // Dystans
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Dystans:", MARGIN_X + 4, y);
    if (distM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", distM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", distM);
    }
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(120);
    tft.drawString(buf, 110, y);
    tft.setTextPadding(0);
    y += 20;

    // Powierzchnia
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Powierzchnia:", MARGIN_X + 4, y);
    snprintf(buf, sizeof(buf), "%.2f m2", areaM2);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(120);
    tft.drawString(buf, 130, y);
    tft.setTextPadding(0);
    y += 20;

    // Czas
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Czas:", MARGIN_X + 4, y);
    fmtTime(timeSec, buf, sizeof(buf));
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(120);
    tft.drawString(buf, 110, y);
    tft.setTextPadding(0);
    y += 24;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 14;

    // Pytanie
    tft.setFreeFont(FSB9);
    tft.setTextColor(cWarning, cBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Wyzerowac liczniki sesji?", TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START = TAK    STOP = NIE", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  RESET WSZYSTKICH LICZNIKOW - potwierdzenie
//  Wyswietla statystyki lifetime i pyta o potwierdzenie
// ============================================================
void DisplayManager::drawCounterResetScreen(float ltDistM, float ltAreaM2,
                                            uint32_t ltTimeSec,
                                            const uint32_t gunShots[6]) {
    drawHeader("RESET LICZNIKOW");

    char buf[64];
    int y = SMENU_START_Y + 2;

    // Ostrzezenie
    tft.setFreeFont(FSB9);
    tft.setTextColor(cError, cBg);
    tft.drawString("UWAGA! Zerowanie WSZYSTKICH licznikow.", MARGIN_X, y);
    y += 18;
    tft.setFreeFont(FS9);
    tft.setTextColor(cAccent, cBg);
    tft.drawString("Kalibracja NIE zostanie zmieniona.", MARGIN_X, y);
    y += 20;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 8;

    // Statystyki lifetime
    tft.setFreeFont(FS9);

    // Dystans
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Dystans:", MARGIN_X + 4, y);
    if (ltDistM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", ltDistM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", ltDistM);
    }
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Powierzchnia
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Powierzchnia:", MARGIN_X + 4, y);
    snprintf(buf, sizeof(buf), "%.2f m2", ltAreaM2);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Czas
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Czas malowania:", MARGIN_X + 4, y);
    fmtTime(ltTimeSec, buf, sizeof(buf));
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Strzaly pistoletow
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Strzaly:", MARGIN_X + 4, y);
    snprintf(buf, sizeof(buf), "%u %u %u %u %u %u",
             gunShots[0], gunShots[1], gunShots[2],
             gunShots[3], gunShots[4], gunShots[5]);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(200);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 22;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 10;

    // Pytanie
    tft.setFreeFont(FSB9);
    tft.setTextColor(cWarning, cBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Wyzerowac wszystkie liczniki?", TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START = TAK    STOP = NIE", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  Ekran podsumowania etapu (SCREEN_SUMMARY)
// ============================================================

void DisplayManager::drawSummaryScreen(const char* patCode, float distM, float areaM2,
                                        unsigned long timeSec, float speedAvg,
                                        bool hasGps, float lat, float lon) {
    drawHeader("PODSUMOWANIE ETAPU");

    char buf[64];
    int y = SMENU_START_Y + 2;

    // Wzorzec
    tft.setFreeFont(FSB12);
    tft.setTextColor(cAccent, cBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(patCode, TFT_SCREEN_W / 2, y + 10);
    tft.setTextDatum(TL_DATUM);
    y += 28;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 6;

    // Tabela statystyk
    tft.setFreeFont(FS9);

    // Dystans
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Dystans:", MARGIN_X + 4, y);
    if (distM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", distM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", distM);
    }
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Powierzchnia
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Powierzchnia:", MARGIN_X + 4, y);
    snprintf(buf, sizeof(buf), "%.2f m2", areaM2);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Czas
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Czas:", MARGIN_X + 4, y);
    fmtTime(timeSec, buf, sizeof(buf));
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Srednia predkosc
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Sred. predkosc:", MARGIN_X + 4, y);
    snprintf(buf, sizeof(buf), "%.1f km/h", speedAvg);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // GPS (jesli dostepny)
    if (hasGps) {
        tft.setTextColor(cMenuTxt, cBg);
        tft.drawString("GPS:", MARGIN_X + 4, y);
        snprintf(buf, sizeof(buf), "%.6f, %.6f", lat, lon);
        tft.setTextColor(cAccent, cBg);
        tft.setTextPadding(200);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
        tft.setTextPadding(0);
        tft.setTextDatum(TL_DATUM);
    }

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START=kontynuuj  STOP=nowy etap  STOP(1s)=HOME", HINT_X, HINT_Y);
    tft.setTextPadding(0);
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
        case STATE_IDLE:     return cAccent;
        case STATE_PAINTING: return cAccent;
        case STATE_PAUSED:   return cWarning;
        case STATE_STOPPED:  return cError;
        default:             return cMenuTxt;
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
//  EKRAN PRZYGOTOWANIA (SETUP) - landscape
//  3 opcje: Tryb pracy, Przelaczanie, Start
//  SEL=dalej, STOP=cofnij, SEL(1s)=zmien, START=maluj
// ============================================================
void DisplayManager::drawSetupScreen(int cursor, MachineMode mode,
                                     bool smartSwitch, bool gapStart) {
    drawHeader("PRZYGOTOWANIE");

    // Etykiety i wartosci
    static const char* labels[3] = {
        "Tryb pracy:",
        "Przelaczanie:",
        "Start:"
    };
    const char* modeVals[3] = { "AUTO", "SEMI-AUTO", "RECZNY" };
    const char* values[3] = {
        modeVals[(int)mode],
        smartSwitch ? "Smart" : "Instant",
        gapStart    ? "Od przerwy" : "Normalny"
    };
    // Kolor wartosci
    uint16_t valColors[3] = {
        cAccent,
        smartSwitch ? cAccent : cWarning,
        gapStart    ? cWarning : cAccent
    };

    const int SETUP_ITEM_H = 48;
    const int SETUP_START_Y = 36;

    for (int i = 0; i < 3; i++) {
        int iy = SETUP_START_Y + i * SETUP_ITEM_H;
        bool sel = (i == cursor);
        uint16_t bg = sel ? cMenuSel : cBg;
        uint16_t fg = sel ? cText      : cMenuTxt;

        tft.fillRect(0, iy, TFT_SCREEN_W, SETUP_ITEM_H, bg);

        tft.setTextDatum(ML_DATUM);

        // Wskaznik ">"
        tft.setFreeFont(FSB9);
        tft.setTextColor(fg, bg);
        if (sel) {
            tft.drawString(">", SMENU_MARKER_X, iy + SETUP_ITEM_H / 2);
        }

        // Etykieta
        tft.setFreeFont(FS9);
        tft.setTextColor(fg, bg);
        tft.drawString(labels[i], SMENU_INDENT, iy + SETUP_ITEM_H / 2);

        // Wartosc — wyrownana do prawej
        tft.setFreeFont(FSB12);
        tft.setTextColor(valColors[i], bg);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(values[i], TFT_SCREEN_W - SMENU_INDENT, iy + SETUP_ITEM_H / 2);

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + SETUP_ITEM_H - 1, TFT_SCREEN_W, cDivider);
    }

    // Wyczysc reszte pod opcjami
    int bottomY = SETUP_START_Y + 3 * SETUP_ITEM_H;
    if (bottomY < HINT_Y) {
        tft.fillRect(0, bottomY, TFT_SCREEN_W, HINT_Y - bottomY, cBg);
    }

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("SEL=dalej SEL(1s)=zmien START=maluj STOP(1s)=wroc", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  Tryb nocny - przelaczanie palety kolorow
// ============================================================
void DisplayManager::applyNightMode(bool night) {
    if (night) {
        cBg       = NIGHT_COLOR_BG;
        cText     = NIGHT_COLOR_TEXT;
        cHeaderBg = NIGHT_COLOR_HEADER_BG;
        cHeaderTxt= NIGHT_COLOR_HEADER_TXT;
        cAccent   = NIGHT_COLOR_ACCENT;
        cWarning  = NIGHT_COLOR_WARNING;
        cError    = NIGHT_COLOR_ERROR;
        cMenuSel  = NIGHT_COLOR_MENU_SEL;
        cMenuTxt  = NIGHT_COLOR_MENU_TXT;
        cDivider  = NIGHT_COLOR_DIVIDER;
        cGunOn    = NIGHT_COLOR_GUN_ON;
        cGunOff   = NIGHT_COLOR_GUN_OFF;
    } else {
        cBg       = COLOR_BG;
        cText     = COLOR_TEXT;
        cHeaderBg = COLOR_HEADER_BG;
        cHeaderTxt= COLOR_HEADER_TXT;
        cAccent   = COLOR_ACCENT;
        cWarning  = COLOR_WARNING;
        cError    = COLOR_ERROR;
        cMenuSel  = COLOR_MENU_SEL;
        cMenuTxt  = COLOR_MENU_TXT;
        cDivider  = COLOR_DIVIDER;
        cGunOn    = COLOR_GUN_ON;
        cGunOff   = COLOR_GUN_OFF;
    }
}

// ============================================================
//  Formatowanie motogodzin (MTH)
// ============================================================
void DisplayManager::fmtMTH(uint32_t sec, char* buf, size_t len) {
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    snprintf(buf, len, "%u.%u h", h, m / 6);  // X.Y h (0.1h = 6min)
}

// ============================================================
//  EKRAN STATYSTYK LIFETIME
// ============================================================
void DisplayManager::drawLifetimeStatsScreen(float ltDistM, float ltAreaM2,
                                              uint32_t ltTimeSec,
                                              const uint32_t gunShots[6],
                                              uint32_t mthSec) {
    drawHeader("STATYSTYKI LIFETIME");

    char buf[64];
    int y = SMENU_START_Y + 2;

    tft.setFreeFont(FS9);

    // Dystans
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Dystans:", MARGIN_X + 4, y);
    if (ltDistM >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f km", ltDistM / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.1f m", ltDistM);
    }
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Powierzchnia
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Powierzchnia:", MARGIN_X + 4, y);
    snprintf(buf, sizeof(buf), "%.2f m2", ltAreaM2);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // Czas malowania
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Czas malowania:", MARGIN_X + 4, y);
    fmtTime(ltTimeSec, buf, sizeof(buf));
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 18;

    // MTH
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Motogodziny:", MARGIN_X + 4, y);
    fmtMTH(mthSec, buf, sizeof(buf));
    tft.setTextColor(cAccent, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 22;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 8;

    // Strzaly per pistolet
    tft.setFreeFont(FSB9);
    tft.setTextColor(cAccent, cBg);
    tft.drawString("Strzaly pistoletow:", MARGIN_X + 4, y);
    y += 22;

    tft.setFreeFont(FS9);
    for (int i = 0; i < NUM_GUNS; i++) {
        int col = i < 3 ? 0 : 1;
        int row = i % 3;
        int px = MARGIN_X + 8 + col * 150;
        int py = y + row * 18;

        snprintf(buf, sizeof(buf), "P%d: %u", i + 1, gunShots[i]);
        tft.setTextColor(cMenuTxt, cBg);
        tft.setTextPadding(140);
        tft.drawString(buf, px, py);
        tft.setTextPadding(0);
    }

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("STOP(1s)=powrot", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  EKRAN EDYCJI WZORCA WLASNEGO
//  cursor: 0=pistolet, 1=tryb, 2=linia, 3=przerwa, 4=zapisz
//  gunIdx: ktory pistolet edytujemy (0..5)
// ============================================================
void DisplayManager::drawCustomPatternScreen(int cursor, int gunIdx,
                                              const CustomPatternCfg& cfg) {
    drawHeader("WZORZEC WLASNY");

    char buf[48];
    int y = SMENU_START_Y + 2;

    // Wybrany pistolet
    tft.setFreeFont(FSB12);
    tft.setTextColor(cAccent, cBg);
    snprintf(buf, sizeof(buf), "Pistolet: P%d", gunIdx + 1);
    tft.setTextPadding(200);
    tft.drawString(buf, MARGIN_X + 4, y);
    tft.setTextPadding(0);
    if (cursor == 0) {
        tft.setTextColor(cWarning, cBg);
        tft.drawString("<", TFT_SCREEN_W - 30, y);
    }
    y += 26;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 8;

    // Wyswietl konfiguracje 6 pistoletow (kompaktowo)
    tft.setFreeFont(FM9);
    static const char* modeNames[] = {"OFF", "CIAG", "PRZERYW"};
    for (int i = 0; i < NUM_GUNS; i++) {
        uint16_t fg = (i == gunIdx) ? cAccent : cMenuTxt;
        tft.setTextColor(fg, cBg);

        GunMode gm = (GunMode)cfg.gunModes[i];
        if (gm == GUN_DASHED) {
            snprintf(buf, sizeof(buf), "P%d: %s L=%.1f G=%.1f",
                     i + 1, modeNames[gm], cfg.lineLen[i], cfg.gapLen[i]);
        } else {
            snprintf(buf, sizeof(buf), "P%d: %s", i + 1, modeNames[gm]);
        }
        tft.setTextPadding(TFT_SCREEN_W - 24);
        tft.drawString(buf, MARGIN_X + 4, y);
        tft.setTextPadding(0);
        y += 16;
    }

    y += 4;
    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 6;

    // Opcje edycji aktualnego pistoletu
    tft.setFreeFont(FS9);

    // Tryb
    bool selMode = (cursor == 1);
    tft.setTextColor(selMode ? cText : cMenuTxt, selMode ? cMenuSel : cBg);
    if (selMode) tft.fillRect(0, y - 2, TFT_SCREEN_W, 18, cMenuSel);
    snprintf(buf, sizeof(buf), "Tryb: %s", modeNames[cfg.gunModes[gunIdx]]);
    tft.drawString(buf, MARGIN_X + 4, y);
    y += 18;

    // Linia
    bool selLine = (cursor == 2);
    tft.setTextColor(selLine ? cText : cMenuTxt, selLine ? cMenuSel : cBg);
    if (selLine) tft.fillRect(0, y - 2, TFT_SCREEN_W, 18, cMenuSel);
    snprintf(buf, sizeof(buf), "Linia: %.1f m", cfg.lineLen[gunIdx]);
    tft.drawString(buf, MARGIN_X + 4, y);
    y += 18;

    // Przerwa
    bool selGap = (cursor == 3);
    tft.setTextColor(selGap ? cText : cMenuTxt, selGap ? cMenuSel : cBg);
    if (selGap) tft.fillRect(0, y - 2, TFT_SCREEN_W, 18, cMenuSel);
    snprintf(buf, sizeof(buf), "Przerwa: %.1f m", cfg.gapLen[gunIdx]);
    tft.drawString(buf, MARGIN_X + 4, y);
    y += 18;

    // Zapisz
    bool selSave = (cursor == 4);
    tft.setFreeFont(FSB9);
    tft.setTextColor(selSave ? cBg : cAccent, selSave ? cAccent : cBg);
    if (selSave) tft.fillRect(80, y, 160, 20, cAccent);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("ZAPISZ", TFT_SCREEN_W / 2, y + 10);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("JOY:nawiguj SEL(1s)=zmien STOP(1s)=powrot", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  EKRAN EKSPORTU STATYSTYK NA SD
// ============================================================
void DisplayManager::drawStatsExportScreen(bool exporting, bool success) {
    drawHeader("EKSPORT STATYSTYK");

    int y = SMENU_START_Y + 20;

    tft.setFreeFont(FSB12);
    tft.setTextDatum(MC_DATUM);

    if (exporting) {
        tft.setTextColor(cWarning, cBg);
        tft.setTextPadding(250);
        tft.drawString("Eksportowanie...", TFT_SCREEN_W / 2, y + 30);
        tft.setTextPadding(0);
    } else if (success) {
        tft.setTextColor(cAccent, cBg);
        tft.setTextPadding(250);
        tft.drawString("Zapisano!", TFT_SCREEN_W / 2, y + 20);
        tft.setTextPadding(0);
        tft.setFreeFont(FS9);
        tft.setTextColor(cMenuTxt, cBg);
        tft.setTextPadding(280);
        tft.drawString("/stats/lifetime_stats.csv", TFT_SCREEN_W / 2, y + 48);
        tft.setTextPadding(0);
    } else {
        tft.setTextColor(cError, cBg);
        tft.setTextPadding(250);
        tft.drawString("Blad zapisu!", TFT_SCREEN_W / 2, y + 30);
        tft.setTextPadding(0);
    }

    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START=eksportuj  STOP(1s)=powrot", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  Ikona ostrzezenia SD na ekranie malowania (prawy dolny rog)
// ============================================================
void DisplayManager::drawSdWarningIcon() {
    // Trojkat ostrzegawczy z "SD" w prawym dolnym rogu
    const int ix = TFT_SCREEN_W - 36;
    const int iy = GUN_RECTS_Y - 22;
    tft.fillTriangle(ix, iy + 16, ix + 8, iy, ix + 16, iy + 16, cWarning);
    tft.setFreeFont(FM9);
    tft.setTextColor(cWarning, cBg);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("SD", ix + 19, iy + 2);
}
