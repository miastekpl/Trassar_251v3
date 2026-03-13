// ============================================================
// TrassarV3 - Ekrany serwisowe (menu + narzedzia serwisowe)
// Wydzielone: menu serwisowe, kalibracja, pomiar dystansu,
// raporty, czyszczenie dysz
// ============================================================

#include "display_internal.h"

// ============================================================
//  MENU SERWISOWE  (9 pozycji, scrollowane) - landscape, bez clear()
//  fillRect na kazdy item eliminuje miganie
// ============================================================
void DisplayManager::drawServiceMenu(int selectedIndex) {
    drawHeader("SERWIS");

    static const char* labels[SMENU_COUNT] = {
        "Kalibracja enkodera",
        "Pomiar dystansu",
        "Raporty",
        "Czyszczenie dysz",
        "Statystyki lifetime",
        "Wzorzec wlasny",
        "Eksport statystyk",
        "Reset etapu",
        "Reset licznikow",
        "Factory reset"
    };

    // Oblicz okno przewijania (viewport)
    int scrollTop = 0;
    if (selectedIndex >= SMENU_VISIBLE) {
        scrollTop = selectedIndex - SMENU_VISIBLE + 1;
    }
    if (scrollTop > SMENU_COUNT - SMENU_VISIBLE) {
        scrollTop = SMENU_COUNT - SMENU_VISIBLE;
    }

    tft.setFreeFont(FS9);

    for (int v = 0; v < SMENU_VISIBLE; v++) {
        int i = scrollTop + v;
        if (i >= SMENU_COUNT) break;

        int iy = SMENU_START_Y + v * SMENU_ITEM_H;
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

        // Wskazniki przewijania
        if (v == 0 && scrollTop > 0) {
            tft.setTextColor(cAccent, bg);
            tft.drawString("^", TFT_SCREEN_W - 20, iy + SMENU_ITEM_H / 2);
            tft.setTextColor(fg, bg);
        }
        if (v == SMENU_VISIBLE - 1 && scrollTop + SMENU_VISIBLE < SMENU_COUNT) {
            tft.setTextColor(cAccent, bg);
            tft.drawString("v", TFT_SCREEN_W - 20, iy + SMENU_ITEM_H / 2);
            tft.setTextColor(fg, bg);
        }

        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(0, iy + SMENU_ITEM_H - 1, TFT_SCREEN_W, cDivider);
    }

    // Wyczysc reszte ekranu pod menu (unikniecie artefaktow)
    int bottomY = SMENU_START_Y + SMENU_VISIBLE * SMENU_ITEM_H;
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
    fmtDist(distanceM, buf, sizeof(buf));
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
