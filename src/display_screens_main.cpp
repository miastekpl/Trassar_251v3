// ============================================================
// TrassarV3 - Ekrany glowne (HOME + PAINTING)
// Wydzielone z display_manager.cpp dla czytelnosci
// ============================================================

#include "display_internal.h"
#include "paint_consumption.h"

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
    // Nazwa wzorca (2 linie jesli dwuslowna)
    tft.setFreeFont(FS9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(COL_L_PAD);
    {
        const char* sp = strchr(patName, ' ');
        if (sp) {
            char line1[24];
            int len1 = sp - patName;
            if (len1 > (int)sizeof(line1) - 1) len1 = sizeof(line1) - 1;
            strncpy(line1, patName, len1);
            line1[len1] = '\0';
            tft.drawString(line1, MARGIN_X, ROW_NAME_Y);
            tft.drawString(sp + 1, MARGIN_X, ROW_NAME2_Y);
        } else {
            tft.drawString(patName, MARGIN_X, ROW_NAME_Y);
            tft.drawString(" ", MARGIN_X, ROW_NAME2_Y);
        }
    }
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

    // Poziom farby w zbiorniku
    {
        float paintLevel = paintConsumption.getCurrentLevel();
        float tankCap = paintConsumption.getTankCapacity();
        int paintPct = (tankCap > 0) ? (int)(paintLevel * 100.0f / tankCap) : 0;
        if (paintPct > 100) paintPct = 100;
        if (paintPct < 0) paintPct = 0;

        uint16_t paintColor = cText;
        if (paintPct <= LOW_PAINT_CRITICAL_PCT) paintColor = cError;
        else if (paintPct <= LOW_PAINT_WARNING_PCT) paintColor = cWarning;

        tft.setFreeFont(FS9);
        tft.setTextColor(paintColor, cBg);
        snprintf(buf, sizeof(buf), "%.0f L (%d%%)", paintLevel, paintPct);
        tft.setTextPadding(COL_R_PAD);
        tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, ROW_PAINT_Y);
        tft.setTextPadding(0);
    }

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
                                        float patternPosM,
                                        bool waitingForMovement) {
    char buf[48];
    bool paused = (state == STATE_PAUSED);

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

    // Status pracy (Malowanie / Pauza / Zatrzymany / Czekam na ruch)
    tft.setFreeFont(FSB9);
    tft.setTextPadding(COL_L_PAD);
    if (waitingForMovement) {
        bool blinkOn = ((millis() / BLINK_PERIOD_MS) % 2) == 0;
        tft.setTextColor(blinkOn ? cWarning : cBg, cBg);
        tft.drawString("Czekam na ruch", MARGIN_X, ROW_STATUS_Y);
    } else {
        uint16_t sc = stateColor(state);
        tft.setTextColor(sc, cBg);
        tft.drawString(stateStr(state), MARGIN_X, ROW_STATUS_Y);
    }
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
    fmtDist(sessionDistM, buf, sizeof(buf));
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
        bool blinkPhase = ((millis() / BLINK_PERIOD_MS) % 2) == 0;
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

    // Poziom farby w zbiorniku
    {
        float paintLevel = paintConsumption.getCurrentLevel();
        float tankCap = paintConsumption.getTankCapacity();
        int paintPct = (tankCap > 0) ? (int)(paintLevel * 100.0f / tankCap) : 0;
        if (paintPct > 100) paintPct = 100;
        if (paintPct < 0) paintPct = 0;

        uint16_t paintColor = cText;
        if (paintPct <= LOW_PAINT_CRITICAL_PCT) paintColor = cError;
        else if (paintPct <= LOW_PAINT_WARNING_PCT) paintColor = cWarning;

        tft.setFreeFont(FS9);
        tft.setTextColor(paintColor, cBg);
        snprintf(buf, sizeof(buf), "%.0f L (%d%%)", paintLevel, paintPct);
        tft.setTextPadding(COL_R_PAD);
        tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, ROW_PAINT_Y);
        tft.setTextPadding(0);
    }

    tft.setTextDatum(TL_DATUM);

    // ---- DOL: 6 prostokatow pistoletow ----
    drawGunRects(GUN_RECTS_Y, gunsCfg, gunStates, paused);
}
