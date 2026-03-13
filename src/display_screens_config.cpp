// ============================================================
// TrassarV3 - Ekrany konfiguracji i diagnostyki
// Wydzielone z display_screens_service.cpp:
// setup, wzorzec wlasny, factory reset, POST, ikony ostrzezen
// ============================================================

#include "display_internal.h"
#include "temp_sensor.h"

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
    const char* modeVals[4] = { "AUTO", "SEMI-AUTO", "RECZNY", "DEMO" };
    const char* values[3] = {
        modeVals[(int)mode < 4 ? (int)mode : 0],
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

// ============================================================
//  POST (Power-On Self-Test) - ekran diagnostyczny na starcie
// ============================================================
void DisplayManager::drawPostScreen(const PostResult& r, bool done) {
    drawHeader("DIAGNOSTYKA (POST)");

    int y = SMENU_START_Y + 4;
    tft.setFreeFont(FS9);

    auto drawItem = [&](const char* label, bool ok) {
        tft.setTextColor(cMenuTxt, cBg);
        tft.drawString(label, MARGIN_X + 4, y);
        tft.setTextPadding(60);
        tft.setTextDatum(TR_DATUM);
        if (ok) {
            tft.setTextColor(cAccent, cBg);
            tft.drawString("OK", TFT_SCREEN_W - MARGIN_X, y);
        } else {
            tft.setTextColor(cError, cBg);
            tft.drawString("BRAK", TFT_SCREEN_W - MARGIN_X, y);
        }
        tft.setTextPadding(0);
        tft.setTextDatum(TL_DATUM);
        y += 22;
    };

    drawItem("Karta SD:", r.sdOk);
    drawItem("Zegar RTC:", r.rtcOk);
    drawItem("GPS NEO-6M:", r.gpsOk);
    drawItem("MCP23017 (I2C):", r.mcpOk);
    drawItem("Enkoder:", r.encOk);

    // Temperatura
    tft.setTextColor(cMenuTxt, cBg);
    tft.drawString("Temperatura:", MARGIN_X + 4, y);
    tft.setTextPadding(80);
    tft.setTextDatum(TR_DATUM);
    if (r.tempOk) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f C", r.temperature);
        bool tempWarn = (r.temperature < TEMP_WARNING_LOW || r.temperature > TEMP_WARNING_HIGH);
        tft.setTextColor(tempWarn ? cWarning : cAccent, cBg);
        tft.drawString(buf, TFT_SCREEN_W - MARGIN_X, y);
    } else {
        tft.setTextColor(cMenuTxt, cBg);
        tft.drawString("--", TFT_SCREEN_W - MARGIN_X, y);
    }
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
    y += 30;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 12;

    // Status ogolny
    bool allOk = r.sdOk && r.rtcOk && r.encOk;
    tft.setFreeFont(FSB12);
    tft.setTextDatum(MC_DATUM);
    if (allOk) {
        tft.setTextColor(cAccent, cBg);
        tft.setTextPadding(200);
        tft.drawString("System gotowy", TFT_SCREEN_W / 2, y + 10);
    } else {
        tft.setTextColor(cWarning, cBg);
        tft.setTextPadding(200);
        tft.drawString("Uwaga: brak modulow", TFT_SCREEN_W / 2, y + 10);
    }
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);

    if (done) {
        tft.setFreeFont(FM9);
        tft.setTextColor(cMenuTxt, cBg);
        tft.setTextPadding(TFT_SCREEN_W - 12);
        tft.drawString("START=kontynuuj", HINT_X, HINT_Y);
        tft.setTextPadding(0);
    }
}

// ============================================================
//  FACTORY RESET NVS - potwierdzenie
// ============================================================
void DisplayManager::drawFactoryResetScreen() {
    drawHeader("FACTORY RESET");

    int y = SMENU_START_Y + 10;

    tft.setFreeFont(FSB12);
    tft.setTextColor(cError, cBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("UWAGA!", TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);
    y += 28;

    tft.setFreeFont(FS9);
    tft.setTextColor(cText, cBg);
    tft.drawString("Wszystkie ustawienia zostana", MARGIN_X + 4, y);
    y += 18;
    tft.drawString("TRWALE USUNIETE z pamieci NVS:", MARGIN_X + 4, y);
    y += 22;

    tft.setTextColor(cWarning, cBg);
    tft.drawString("- Kalibracja enkodera", MARGIN_X + 12, y); y += 16;
    tft.drawString("- Statystyki lifetime", MARGIN_X + 12, y); y += 16;
    tft.drawString("- Wzorce wlasne", MARGIN_X + 12, y); y += 16;
    tft.drawString("- Liczniki pistoletow", MARGIN_X + 12, y); y += 16;
    tft.drawString("- Motogodziny", MARGIN_X + 12, y); y += 22;

    tft.drawFastHLine(12, y, TFT_SCREEN_W - 24, cDivider);
    y += 12;

    tft.setFreeFont(FSB9);
    tft.setTextColor(cError, cBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Przywrocic ustawienia fabryczne?", TFT_SCREEN_W / 2, y);
    tft.setTextDatum(TL_DATUM);

    // --- Podpowiedzi ---
    tft.setFreeFont(FM9);
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextPadding(TFT_SCREEN_W - 12);
    tft.drawString("START(3s) = TAK    STOP = NIE", HINT_X, HINT_Y);
    tft.setTextPadding(0);
}

// ============================================================
//  Ikona ostrzezenia GPS overflow na ekranie malowania
// ============================================================
void DisplayManager::drawGpsOverflowIcon() {
    const int ix = TFT_SCREEN_W - 68;
    const int iy = GUN_RECTS_Y - 22;
    tft.fillTriangle(ix, iy + 16, ix + 8, iy, ix + 16, iy + 16, cWarning);
    tft.setFreeFont(FM9);
    tft.setTextColor(cWarning, cBg);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("GPS", ix + 19, iy + 2);
}
