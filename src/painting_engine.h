#pragma once
// ============================================================
// TrassarV3 - Silnik malowania (maszyna stanów)
// ============================================================

#include "config.h"

class PaintingEngine {
public:
    void begin();
    void update();  // Wywoływana w loop()

    void start();
    void startFromGap();  // Start od przerwy
    void pause();
    void resume();
    void stop();

    void setPattern(PatternID pat);
    void toggleReverse();

    bool shouldGunFire(GunID gun, float distFromPatternStart) const;
    bool isGapStart() const { return gapStartActive; }

private:
    float lastEncoderDist = 0;
    float patternStartDist = 0;  // Dystans przy zmianie wzorca
    bool  gapStartActive = false; // Czy aktywny "start od przerwy"
};

extern PaintingEngine paintEngine;
