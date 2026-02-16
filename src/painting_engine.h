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

    // Gun keepalive - awaryjne wylaczenie jesli update() nie dziala
    void checkGunKeepAlive();
    unsigned long getLastGunUpdateMs() const { return lastGunUpdateMs; }

    // Prog predkosci maks. (konfigurowalny z WWW)
    void setMaxSpeed(float kmh) { maxSpeedKmh = kmh; }
    float getMaxSpeed() const { return maxSpeedKmh; }
    bool isOverspeed() const { return overspeedActive; }
    bool isLowSpeed() const { return lowSpeedActive; }

private:
    float lastEncoderDist = 0;
    float patternStartDist = 0;  // Dystans przy zmianie wzorca
    bool  gapStartActive = false; // Czy aktywny "start od przerwy"

    // Gun keepalive
    unsigned long lastGunUpdateMs = 0;

    // Alarm predkosci
    float maxSpeedKmh = DEFAULT_MAX_PAINT_SPEED_KMH;
    bool overspeedActive = false;
    bool lowSpeedActive = false;
    unsigned long lastLowSpeedBuzMs = 0;   // Throttle buzzera niskiej predkosci
    unsigned long lastOverspeedBuzMs = 0;  // Throttle buzzera przekroczenia
};

extern PaintingEngine paintEngine;
