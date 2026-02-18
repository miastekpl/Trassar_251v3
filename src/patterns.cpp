// ============================================================
// TrassarV3 - Implementacja wzorców malowania
// ============================================================

#include "patterns.h"
#include "storage.h"

PatternManager patternMgr;

//  Makra pomocnicze: OFF, CONT(inuous), DASH(ed)
#define G_OFF   {GUN_OFF,        0, 0}
#define G_CONT  {GUN_CONTINUOUS, 0, 0}
#define G_DASH(l,g) {GUN_DASHED, l, g}

// ============================================================
// Tablica 15 wzorców wg polskich norm oznakowania drogowego
// Indeks = GUN: P1    P2    P3    P4    P5    P6
// ============================================================
const PatternDef PatternManager::patterns[PREDEFINED_PAT_COUNT] = {
    // --- Grupa P-1: Przerywane ---
    { "P-1a", "Przerywana dluga",    12, false,
      { G_OFF, G_DASH(4.0f,8.0f), G_OFF, G_OFF, G_OFF, G_OFF } },

    { "P-1b", "Przerywana krotka",   12, false,
      { G_OFF, G_DASH(2.0f,4.0f), G_OFF, G_OFF, G_OFF, G_OFF } },

    { "P-1c", "Wydzielajaca",        12, false,
      { G_OFF, G_DASH(2.0f,2.0f), G_OFF, G_OFF, G_OFF, G_OFF } },

    { "P-1d", "Prowadzaca waska",    12, false,
      { G_OFF, G_DASH(1.0f,1.0f), G_OFF, G_OFF, G_OFF, G_OFF } },

    { "P-1e", "Prowadzaca szeroka",  24, false,
      { G_OFF, G_OFF, G_OFF, G_DASH(1.0f,1.0f), G_OFF, G_OFF } },

    // --- Grupa P-2: Ciągłe ---
    { "P-2a", "Ciagla waska",        12, false,
      { G_OFF, G_CONT, G_OFF, G_OFF, G_OFF, G_OFF } },

    { "P-2b", "Ciagla szeroka",      24, false,
      { G_OFF, G_OFF, G_OFF, G_CONT, G_OFF, G_OFF } },

    // --- Grupa P-3: Przekraczalne (odwracalne!) ---
    //  Domyślnie: P1=ciągła (lewa), P3=przerywana (prawa)
    //  Po odwróceniu: P1=przerywana, P3=ciągła
    { "P-3a", "Przekraczalna dluga", 12, true,
      { G_CONT, G_OFF, G_DASH(4.0f,2.0f), G_OFF, G_OFF, G_OFF } },

    { "P-3b", "Przekraczalna krotka",12, true,
      { G_CONT, G_OFF, G_DASH(1.0f,1.0f), G_OFF, G_OFF, G_OFF } },

    // --- P-4: Podwójna ciągła ---
    { "P-4",  "Podwojna ciagla",     24, false,
      { G_CONT, G_OFF, G_CONT, G_OFF, G_OFF, G_OFF } },

    // --- P-6: Ostrzegawcza ---
    { "P-6",  "Ostrzegawcza",        12, false,
      { G_OFF, G_OFF, G_OFF, G_OFF, G_DASH(4.0f,2.0f), G_OFF } },

    // --- Grupa P-7: Krawędziowe ---
    { "P-7a", "Krawedz przeryw.szer",24, false,
      { G_OFF, G_OFF, G_OFF, G_OFF, G_OFF, G_DASH(1.0f,1.0f) } },

    { "P-7b", "Krawedz ciagla szer", 24, false,
      { G_OFF, G_OFF, G_OFF, G_OFF, G_OFF, G_CONT } },

    { "P-7c", "Krawedz przeryw.wask",12, false,
      { G_OFF, G_OFF, G_OFF, G_OFF, G_DASH(1.0f,1.0f), G_OFF } },

    { "P-7d", "Krawedz ciagla wask", 12, false,
      { G_OFF, G_OFF, G_OFF, G_OFF, G_CONT, G_OFF } },
};

#undef G_OFF
#undef G_CONT
#undef G_DASH

void PatternManager::begin() {
    g_state.currentPattern = PAT_P1A;
    g_state.patternReversed = false;
    // Zaladuj wzorzec wlasny z NVS
    loadCustomFromStorage();
}

void PatternManager::setPattern(PatternID id) {
    if (id == PAT_CUSTOM) {
        if (!customValid) return;  // Nie przelacz na niewazny wzorzec
        g_state.currentPattern = PAT_CUSTOM;
        g_state.patternReversed = false;
        return;
    }
    if (id < PREDEFINED_PAT_COUNT) {
        g_state.currentPattern = id;
        // Wyłącz odwrócenie jeśli nowy wzorzec go nie obsługuje
        if (!patterns[id].hasReverse) {
            g_state.patternReversed = false;
        }
    }
}

void PatternManager::nextPattern() {
    int next = (int)g_state.currentPattern + 1;
    // Pomin PAT_CUSTOM w cyklicznym przelaczaniu
    if (next >= PREDEFINED_PAT_COUNT) next = 0;
    setPattern((PatternID)next);
}

void PatternManager::prevPattern() {
    int prev = (int)g_state.currentPattern - 1;
    if (prev < 0) prev = PREDEFINED_PAT_COUNT - 1;
    // Jesli bylismy na PAT_CUSTOM, cofnij do ostatniego predefiniowanego
    if (g_state.currentPattern == PAT_CUSTOM) prev = PREDEFINED_PAT_COUNT - 1;
    setPattern((PatternID)prev);
}

void PatternManager::toggleReverse() {
    const PatternDef& pat = getCurrent();
    if (pat.hasReverse) {
        g_state.patternReversed = !g_state.patternReversed;
    }
}

const PatternDef& PatternManager::getCurrent() const {
    if (g_state.currentPattern == PAT_CUSTOM) return customPatDef;
    if (g_state.currentPattern < PREDEFINED_PAT_COUNT)
        return patterns[g_state.currentPattern];
    return patterns[0];  // Fallback
}

const PatternDef& PatternManager::getPattern(PatternID id) const {
    if (id == PAT_CUSTOM) return customPatDef;
    if (id < PREDEFINED_PAT_COUNT) return patterns[id];
    return patterns[0];  // Fallback
}

GunPatternCfg PatternManager::getGunConfig(GunID gun) const {
    const PatternDef& pat = getCurrent();

    // Dla wzorców z odwróceniem (P-3a, P-3b): zamień P1 <-> P3
    if (g_state.patternReversed && pat.hasReverse) {
        if (gun == GUN_P1) return pat.guns[GUN_P3];
        if (gun == GUN_P3) return pat.guns[GUN_P1];
    }

    return pat.guns[gun];
}

// ============================================================
// Wzorzec wlasny - budowanie PatternDef z CustomPatternCfg
// ============================================================
void PatternManager::setCustomPattern(const CustomPatternCfg& cfg) {
    customValid = cfg.valid;
    customPatDef.code = "WLASNY";
    customPatDef.name = "Wzorzec wlasny";
    customPatDef.nominalWidth_cm = 12;
    customPatDef.hasReverse = false;

    for (int i = 0; i < NUM_GUNS; i++) {
        GunMode gm = (GunMode)cfg.gunModes[i];
        if (gm == GUN_DASHED) {
            customPatDef.guns[i] = {GUN_DASHED, cfg.lineLen, cfg.gapLen};
        } else if (gm == GUN_CONTINUOUS) {
            customPatDef.guns[i] = {GUN_CONTINUOUS, 0, 0};
        } else {
            customPatDef.guns[i] = {GUN_OFF, 0, 0};
        }
        // Aktualizuj szerokosc nominalna
        if (gm != GUN_OFF) {
            float w = GUN_WIDTHS_M[i] * 100.0f;  // cm
            if (w > customPatDef.nominalWidth_cm) customPatDef.nominalWidth_cm = w;
        }
    }

    Serial.printf("[PAT] Wzorzec wlasny %s: linia=%.1fm przerwa=%.1fm\n",
                  cfg.valid ? "zapisany" : "niewazny", cfg.lineLen, cfg.gapLen);
}

void PatternManager::loadCustomFromStorage() {
    CustomPatternCfg cfg = storage.loadCustomPattern();
    setCustomPattern(cfg);
}
