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
    void pause();
    void resume();
    void stop();

    void setPattern(PatternID pat);
    void toggleReverse();

    bool shouldGunFire(GunID gun, float distFromPatternStart) const;

private:
    float lastEncoderDist = 0;
    float patternStartDist = 0;  // Dystans przy zmianie wzorca
};

extern PaintingEngine paintEngine;
