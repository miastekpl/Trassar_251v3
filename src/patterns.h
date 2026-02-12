#pragma once
// ============================================================
// TrassarV3 - Zarządzanie wzorcami malowania
// ============================================================

#include "config.h"

class PatternManager {
public:
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

    static const PatternDef patterns[PAT_COUNT];
};

extern PatternManager patternMgr;
