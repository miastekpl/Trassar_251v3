// ============================================================
// TrassarV3 - Implementacja wzorców malowania
// ============================================================

#include "patterns.h"

PatternManager patternMgr;

//  Makra pomocnicze: OFF, CONT(inuous), DASH(ed)
#define G_OFF   {GUN_OFF,        0, 0}
#define G_CONT  {GUN_CONTINUOUS, 0, 0}
#define G_DASH(l,g) {GUN_DASHED, l, g}

// ============================================================
// Tablica 15 wzorców wg polskich norm oznakowania drogowego
// Indeks = GUN: P1    P2    P3    P4    P5    P6
// ============================================================
const PatternDef PatternManager::patterns[PAT_COUNT] = {
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
}

void PatternManager::setPattern(PatternID id) {
    if (id < PAT_COUNT) {
        g_state.currentPattern = id;
        // Wyłącz odwrócenie jeśli nowy wzorzec go nie obsługuje
        if (!patterns[id].hasReverse) {
            g_state.patternReversed = false;
        }
    }
}

void PatternManager::nextPattern() {
    int next = (int)g_state.currentPattern + 1;
    if (next >= PAT_COUNT) next = 0;
    setPattern((PatternID)next);
}

void PatternManager::prevPattern() {
    int prev = (int)g_state.currentPattern - 1;
    if (prev < 0) prev = PAT_COUNT - 1;
    setPattern((PatternID)prev);
}

void PatternManager::toggleReverse() {
    const PatternDef& pat = getCurrent();
    if (pat.hasReverse) {
        g_state.patternReversed = !g_state.patternReversed;
    }
}

const PatternDef& PatternManager::getCurrent() const {
    return patterns[g_state.currentPattern];
}

const PatternDef& PatternManager::getPattern(PatternID id) const {
    return patterns[id];
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
