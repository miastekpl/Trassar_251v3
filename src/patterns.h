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

    // Wzorzec wlasny
    void setCustomPattern(const CustomPatternCfg& cfg);
    void loadCustomFromStorage();
    bool isCustomValid() const { return customValid; }

    static const PatternDef patterns[PREDEFINED_PAT_COUNT];

private:
    PatternDef customPatDef;
    bool customValid = false;
};

extern PatternManager patternMgr;
