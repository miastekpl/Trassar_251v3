// ============================================================
// TrassarV3 - Rdzen modulu wyswietlacza ILI9341
// 320x240 landscape, podswietlenie LEDC PWM
// Inicjalizacja, elementy wspolne, funkcje pomocnicze
// Ekrany glowne: display_screens_main.cpp
// Ekrany serwisowe: display_screens_service.cpp
// ============================================================

#include "display_internal.h"

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
//  Kolory: szary=brak wzorca, zolty miganie=we wzorcu, zielony=maluje, zolty miganie=pauza/przerwa
// ============================================================
void DisplayManager::drawGunRects(int y, const GunPatternCfg gunsCfg[6],
                                   const bool gunStates[6], bool paused,
                                   bool waitingForMovement) {
    const int gunW = (TFT_SCREEN_W - (NUM_GUNS - 1) * GUN_GAP) / NUM_GUNS;
    const int totalW = NUM_GUNS * gunW + (NUM_GUNS - 1) * GUN_GAP;
    const int startX = (TFT_SCREEN_W - totalW) / 2;

    bool blinkOn = ((millis() / BLINK_SLOW_MS) % 2) == 0;

    tft.setFreeFont(FSB9);
    tft.setTextDatum(MC_DATUM);

    char lbl[4];
    for (int i = 0; i < NUM_GUNS; i++) {
        int rx = startX + i * (gunW + GUN_GAP);
        int ry = y;
        bool usedInPattern = (gunsCfg[i].mode != GUN_OFF);
        bool firing = gunStates ? gunStates[i] : false;

        uint16_t col;
        if (firing) {
            col = cGunOn;             // zielony - maluje
        } else if (waitingForMovement && usedInPattern) {
            // czeka na ruch po starcie - miganie zielony
            col = blinkOn ? cGunOn : cBg;
        } else if (usedInPattern) {
            // we wzorcu ale nie maluje (pauza/przerwa/oczekiwanie) - miganie zolty
            col = blinkOn ? cWarning : cBg;
        } else {
            col = cGunOff;            // szary - nieuzywany we wzorcu
        }

        tft.fillRect(rx, ry, gunW, GUN_H, col);
        tft.drawRect(rx, ry, gunW, GUN_H, cText);

        // Etykieta P1..P6
        snprintf(lbl, sizeof(lbl), "P%d", i + 1);
        // Tekst ciemny na jasnym tle lub jasny na ciemnym
        if (col == cBg || col == cGunOff) {
            tft.setTextColor(cText, col);
        } else {
            tft.setTextColor(cBg, col);
        }
        tft.drawString(lbl, rx + gunW / 2, ry + GUN_H / 2);
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
        case MODE_DEMO:      return "[DEMO]";
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
//  Formatowanie dystansu (>=1000 -> km, else -> m)
// ============================================================
void DisplayManager::fmtDist(float meters, char* buf, size_t len) {
    if (meters >= 1000.0f) {
        snprintf(buf, len, "%.2f km", meters / 1000.0f);
    } else {
        snprintf(buf, len, "%.1f m", meters);
    }
}

// ============================================================
//  Wiersz statystyk: label po lewej, value po prawej (TR_DATUM)
// ============================================================
void DisplayManager::drawStatRow(const char* label, const char* value, int y) {
    tft.setTextColor(cMenuTxt, cBg);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(label, MARGIN_X + 4, y);
    tft.setTextColor(cText, cBg);
    tft.setTextPadding(140);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(value, TFT_SCREEN_W - MARGIN_X, y);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
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
