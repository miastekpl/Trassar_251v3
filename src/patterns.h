#pragma once
// ============================================================
// TrassarV3 - Zarządzanie wzorcami malowania
// ============================================================

#include "config.h"

class PatternManager {
public:
    static const int PREDEFINED_PAT_COUNT = 15;

    void begin();

    void setPattern(PatternID id);
    void nextPattern();
    void prevPattern();
    void toggleReverse();

    PatternID getCurrentID() const { return g_state.currentPattern; }
    const PatternDef& getCurrent() const;
    const PatternDef& getPattern(PatternID id) const;
    bool isReversed() const { return g_state.patternReversed; }

    // Zwraca konfigurację pistoletu z uwzględnieniem odwrócenia
    GunPatternCfg getGunConfig(GunID gun) const;

    // Wzorzec wlasny (aktywny)
    void setCustomPattern(const CustomPatternCfg& cfg);
    void loadCustomFromStorage();
    bool isCustomValid() const { return customValid; }

    // 3 sloty wzorcow wlasnych
    void saveSlot(int slot, const CustomPatternCfg& cfg);
    CustomPatternCfg loadSlot(int slot);
    bool isSlotValid(int slot) const;
    void activateSlot(int slot);
    int getActiveSlot() const { return activeCustomSlot; }

    static const PatternDef patterns[PREDEFINED_PAT_COUNT];

    // Mutex chroniacy customPatDef (Core 0 pisze, Core 1 czyta)
    mutable portMUX_TYPE customMux = portMUX_INITIALIZER_UNLOCKED;

private:
    PatternDef customPatDef;
    bool customValid = false;
    int activeCustomSlot = 0;
    bool slotValid[NUM_CUSTOM_SLOTS] = {};
};

extern PatternManager patternMgr;
