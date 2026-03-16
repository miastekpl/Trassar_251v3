#include "sys_log.h"
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
    STATE_LOCK();
    g_state.currentPattern = PAT_P1A;
    g_state.patternReversed = false;
    STATE_UNLOCK();
    // Zaladuj sloty wzorcow wlasnych z NVS
    for (int s = 0; s < NUM_CUSTOM_SLOTS; s++) {
        CustomPatternCfg cfg = storage.loadCustomPattern(s);
        slotValid[s] = cfg.valid;
    }
    // Domyslnie aktywuj slot 0
    loadCustomFromStorage();
}

void PatternManager::setPattern(PatternID id) {
    if (id == PAT_CUSTOM) {
        if (!customValid) return;  // Nie przelacz na niewazny wzorzec
        STATE_LOCK();
        g_state.currentPattern = PAT_CUSTOM;
        g_state.patternReversed = false;
        STATE_UNLOCK();
        return;
    }
    if (id < PREDEFINED_PAT_COUNT) {
        STATE_LOCK();
        g_state.currentPattern = id;
        // Wyłącz odwrócenie jeśli nowy wzorzec go nie obsługuje
        if (!patterns[id].hasReverse) {
            g_state.patternReversed = false;
        }
        STATE_UNLOCK();
    }
}

void PatternManager::nextPattern() {
    STATE_LOCK();
    int next = (int)g_state.currentPattern + 1;
    STATE_UNLOCK();
    // Pomin PAT_CUSTOM w cyklicznym przelaczaniu
    if (next >= PREDEFINED_PAT_COUNT) next = 0;
    setPattern((PatternID)next);
}

void PatternManager::prevPattern() {
    STATE_LOCK();
    PatternID cur = g_state.currentPattern;
    STATE_UNLOCK();
    int prev = (int)cur - 1;
    if (prev < 0) prev = PREDEFINED_PAT_COUNT - 1;
    // Jesli bylismy na PAT_CUSTOM, cofnij do ostatniego predefiniowanego
    if (cur == PAT_CUSTOM) prev = PREDEFINED_PAT_COUNT - 1;
    setPattern((PatternID)prev);
}

void PatternManager::toggleReverse() {
    const PatternDef& pat = getCurrent();
    if (pat.hasReverse) {
        STATE_LOCK();
        g_state.patternReversed = !g_state.patternReversed;
        STATE_UNLOCK();
    }
}

const PatternDef& PatternManager::getCurrent() const {
    STATE_LOCK();
    PatternID curPat = g_state.currentPattern;
    STATE_UNLOCK();
    if (curPat == PAT_CUSTOM) return customPatDef;
    if (curPat < PREDEFINED_PAT_COUNT)
        return patterns[curPat];
    return patterns[0];  // Fallback
}

const PatternDef& PatternManager::getPattern(PatternID id) const {
    if (id == PAT_CUSTOM) return customPatDef;
    if (id < PREDEFINED_PAT_COUNT) return patterns[id];
    return patterns[0];  // Fallback
}

// Walidacja konfiguracji pistoletu — ochrona przed smieci z NVS/PSRAM
static GunPatternCfg validateGunCfg(GunPatternCfg cfg) {
    // Walidacja trybu — jesli spoza enum, traktuj jako OFF
    if (cfg.mode != GUN_OFF && cfg.mode != GUN_CONTINUOUS && cfg.mode != GUN_DASHED) {
        cfg.mode = GUN_OFF;
        cfg.lineLen = 0;
        cfg.gapLen = 0;
        return cfg;
    }
    // DASHED wymaga dodatnich dlugosci linii i przerwy
    if (cfg.mode == GUN_DASHED) {
        if (cfg.lineLen <= 0 || cfg.lineLen > 100.0f) cfg.lineLen = 2.0f;
        if (cfg.gapLen <= 0 || cfg.gapLen > 100.0f)   cfg.gapLen = 2.0f;
    }
    // CONTINUOUS/OFF nie uzywaja lineLen/gapLen
    if (cfg.mode != GUN_DASHED) {
        cfg.lineLen = 0;
        cfg.gapLen = 0;
    }
    return cfg;
}

GunPatternCfg PatternManager::getGunConfig(GunID gun) const {
    // Walidacja indeksu pistoletu
    if (gun >= NUM_GUNS) {
        return {GUN_OFF, 0, 0};
    }

    STATE_LOCK();
    PatternID curPat = g_state.currentPattern;
    bool reversed = g_state.patternReversed;
    STATE_UNLOCK();

    GunPatternCfg cfg;

    // Dla wzorca wlasnego: odczyt pod customMux
    if (curPat == PAT_CUSTOM) {
        taskENTER_CRITICAL(&customMux);
        cfg = customPatDef.guns[gun];
        taskEXIT_CRITICAL(&customMux);
        return validateGunCfg(cfg);
    }

    const PatternDef& pat = (curPat < PREDEFINED_PAT_COUNT)
                            ? patterns[curPat] : patterns[0];

    // Dla wzorców z odwróceniem (P-3a, P-3b): zamień P1 <-> P3
    if (reversed && pat.hasReverse) {
        if (gun == GUN_P1) return pat.guns[GUN_P3];
        if (gun == GUN_P3) return pat.guns[GUN_P1];
    }

    return pat.guns[gun];
}

// ============================================================
// Wzorzec wlasny - budowanie PatternDef z CustomPatternCfg
// ============================================================
void PatternManager::setCustomPattern(const CustomPatternCfg& cfg) {
    // Buduj lokalna kopie PatternDef, potem atomowo podmien
    PatternDef newDef;
    newDef.code = "WLASNY";
    newDef.name = "Wzorzec wlasny";
    newDef.nominalWidth_cm = 12;
    newDef.hasReverse = false;

    for (int i = 0; i < NUM_GUNS; i++) {
        GunMode gm = (GunMode)cfg.gunModes[i];
        if (gm == GUN_DASHED) {
            newDef.guns[i] = {GUN_DASHED, cfg.lineLen[i], cfg.gapLen[i]};
        } else if (gm == GUN_CONTINUOUS) {
            newDef.guns[i] = {GUN_CONTINUOUS, 0, 0};
        } else {
            newDef.guns[i] = {GUN_OFF, 0, 0};
        }
        if (gm != GUN_OFF) {
            float w = GUN_WIDTHS_M[i] * 100.0f;  // cm
            if (w > newDef.nominalWidth_cm) newDef.nominalWidth_cm = w;
        }
    }

    // Atomowa podmiana pod mutexem (Core 1 czyta getCurrent/getGunConfig)
    taskENTER_CRITICAL(&customMux);
    customPatDef = newDef;
    customValid = cfg.valid;
    taskEXIT_CRITICAL(&customMux);

    DBG_PRINTF("[PAT] Wzorzec wlasny %s: %d pistoletow aktywnych\n",
                  cfg.valid ? "zapisany" : "niewazny",
                  (int)(cfg.gunModes[0]!=0)+(cfg.gunModes[1]!=0)+(cfg.gunModes[2]!=0)+
                  (cfg.gunModes[3]!=0)+(cfg.gunModes[4]!=0)+(cfg.gunModes[5]!=0));
}

void PatternManager::loadCustomFromStorage() {
    CustomPatternCfg cfg = storage.loadCustomPattern(activeCustomSlot);
    setCustomPattern(cfg);
}

void PatternManager::saveSlot(int slot, const CustomPatternCfg& cfg) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) return;
    storage.saveCustomPattern(cfg, slot);
    slotValid[slot] = cfg.valid;
    DBG_PRINTF("[PAT] Slot %d zapisany\n", slot);
}

CustomPatternCfg PatternManager::loadSlot(int slot) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) slot = 0;
    return storage.loadCustomPattern(slot);
}

bool PatternManager::isSlotValid(int slot) const {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) return false;
    return slotValid[slot];
}

void PatternManager::activateSlot(int slot) {
    if (slot < 0 || slot >= NUM_CUSTOM_SLOTS) return;
    activeCustomSlot = slot;
    CustomPatternCfg cfg = storage.loadCustomPattern(slot);
    setCustomPattern(cfg);
    DBG_PRINTF("[PAT] Aktywowano slot %d\n", slot);
}
