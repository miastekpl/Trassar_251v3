// ============================================================
// TrassarV3 - Ekrany serwisowe i konfiguracyjne
// Wydzielone z display_manager.cpp dla czytelnosci
// ============================================================

#include "display_internal.h"

// ============================================================
//  MENU SERWISOWE  (6 pozycji) - landscape, bez clear()
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
