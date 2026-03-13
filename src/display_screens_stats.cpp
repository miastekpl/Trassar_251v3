// ============================================================
// TrassarV3 - Ekrany statystyk i resetow
// Wydzielone z display_screens_service.cpp:
// podsumowanie etapu, statystyki lifetime, eksport,
// reset etapu, reset licznikow
// ============================================================

#include "display_internal.h"

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
    fmtDist(distM, buf, sizeof(buf));
    drawStatRow("Dystans:", buf, y);
    y += 20;

    // Powierzchnia
    snprintf(buf, sizeof(buf), "%.2f m2", areaM2);
    drawStatRow("Powierzchnia:", buf, y);
    y += 20;

    // Czas
    fmtTime(timeSec, buf, sizeof(buf));
    drawStatRow("Czas:", buf, y);
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

    fmtDist(ltDistM, buf, sizeof(buf));
    drawStatRow("Dystans:", buf, y);
    y += 18;

    snprintf(buf, sizeof(buf), "%.2f m2", ltAreaM2);
    drawStatRow("Powierzchnia:", buf, y);
    y += 18;

    fmtTime(ltTimeSec, buf, sizeof(buf));
    drawStatRow("Czas malowania:", buf, y);
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

    fmtDist(distM, buf, sizeof(buf));
    drawStatRow("Dystans:", buf, y);
    y += 18;

    snprintf(buf, sizeof(buf), "%.2f m2", areaM2);
    drawStatRow("Powierzchnia:", buf, y);
    y += 18;

    fmtTime(timeSec, buf, sizeof(buf));
    drawStatRow("Czas:", buf, y);
    y += 18;

    snprintf(buf, sizeof(buf), "%.1f km/h", speedAvg);
    drawStatRow("Sred. predkosc:", buf, y);
    y += 18;

    // GPS (jesli dostepny)
    if (hasGps) {
        snprintf(buf, sizeof(buf), "%.6f, %.6f", lat, lon);
        drawStatRow("GPS:", buf, y);
    }

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START=kontynuuj  STOP=nowy etap  STOP(1s)=HOME", HINT_X, HINT_Y);
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

    fmtDist(ltDistM, buf, sizeof(buf));
    drawStatRow("Dystans:", buf, y);
    y += 18;

    snprintf(buf, sizeof(buf), "%.2f m2", ltAreaM2);
    drawStatRow("Powierzchnia:", buf, y);
    y += 18;

    fmtTime(ltTimeSec, buf, sizeof(buf));
    drawStatRow("Czas malowania:", buf, y);
    y += 18;

    fmtMTH(mthSec, buf, sizeof(buf));
    drawStatRow("Motogodziny:", buf, y);
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
