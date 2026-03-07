#pragma once
// ============================================================
// TrassarV3 - Silnik malowania (maszyna stanów)
// ============================================================

#include "config.h"

class PaintingEngine {
public:
    void begin();
    void update();  // Wywoływana w loop()

    void start(float offsetDist = 0.0f);
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

    // Progi predkosci (konfigurowalne z WWW)
    void setMaxSpeed(float kmh) {
        if (kmh < 1.0f) kmh = 1.0f;
        if (kmh > 50.0f) kmh = 50.0f;
        maxSpeedKmh = kmh;
        if (minSpeedKmh > maxSpeedKmh) minSpeedKmh = maxSpeedKmh;
    }
    float getMaxSpeed() const { return maxSpeedKmh; }
    void setMinSpeed(float kmh) {
        if (kmh < 0.0f) kmh = 0.0f;
        if (kmh > 50.0f) kmh = 50.0f;
        minSpeedKmh = kmh;
        if (maxSpeedKmh < minSpeedKmh) maxSpeedKmh = minSpeedKmh;
    }
    float getMinSpeed() const { return minSpeedKmh; }
    bool isOverspeed() const { return overspeedActive; }
    bool isLowSpeed() const { return lowSpeedActive; }

    // Przelaczanie wzorcow (smart/instant)
    bool isPatternChangePending() const { return patternChangePending; }
    PatternID getPendingPattern() const { return pendingPattern; }
    void setSmartSwitch(bool smart) { smartSwitch = smart; }
    bool isSmartSwitch() const { return smartSwitch; }

    // Tryb polautomatyczny - wyzwolenie kolejnej linii
    void semiNextLine();
    bool isSemiLineComplete() const { return semiLineComplete; }

    // Dystans od startu wzorca (do podgladu na TFT)
    float getPatternDistance() const;

private:
    float lastEncoderDist = 0;
    float patternStartDist = 0;  // Dystans przy zmianie wzorca
    bool  gapStartActive = false; // Czy aktywny "start od przerwy"

    // Inteligentne przelaczanie wzorcow - czekaj na koniec cyklu
    PatternID pendingPattern = PAT_P1A;
    bool  patternChangePending = false;
    int   pendingCycleCount = 0;

    float getPrimaryCycle() const;
    void  applyPendingPattern();

    // Tryb polautomatyczny (SEMI_AUTO)
    float semiLineDist = 0;        // Dystans w biezacej linii
    bool  semiLineComplete = false; // Linia zakonczona, czeka na START

    // Gun keepalive
    unsigned long lastGunUpdateMs = 0;

    // Alarm predkosci
    float maxSpeedKmh = DEFAULT_MAX_PAINT_SPEED_KMH;
    float minSpeedKmh = DEFAULT_MIN_PAINT_SPEED_KMH;
    bool overspeedActive = false;
    bool lowSpeedActive = false;
    bool smartSwitch = true;  // true=inteligentne, false=natychmiastowe
    unsigned long lastLowSpeedBuzMs = 0;   // Throttle buzzera niskiej predkosci
    unsigned long lastOverspeedBuzMs = 0;  // Throttle buzzera przekroczenia
};

extern PaintingEngine paintEngine;
