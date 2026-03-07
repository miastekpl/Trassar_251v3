#pragma once
// ============================================================
// TrassarV3 - Predykcja zuzycia farby
// Na podstawie wzorca, powierzchni i konfiguracji pistoletow
// ============================================================

#include "config.h"

class PaintConsumption {
public:
    // Domyslne zuzycie farby [litry/m2] - typowe dla farb drogowych
    static constexpr float DEFAULT_CONSUMPTION_L_PER_M2 = 0.60f;  // 0.6 l/m2

    void begin();

    // Pojemnosc zbiornika [litry]
    void setTankCapacity(float liters) { tankCapacityL = liters; }
    float getTankCapacity() const { return tankCapacityL; }

    // Zuzycie na m2 [litry]
    void setConsumptionRate(float lPerM2) { consumptionRateL = lPerM2; }
    float getConsumptionRate() const { return consumptionRateL; }

    // Oblicz zuzycie na podstawie pomalowanej powierzchni
    float getUsedLiters(float paintedAreaM2) const {
        return paintedAreaM2 * consumptionRateL;
    }

    // Ile zostalo w zbiorniku
    float getRemainingLiters(float paintedAreaM2) const {
        float used = getUsedLiters(paintedAreaM2);
        float remaining = tankCapacityL - used;
        return remaining > 0 ? remaining : 0;
    }

    // Procent zuzycia zbiornika
    int getUsedPercent(float paintedAreaM2) const {
        if (tankCapacityL <= 0) return 0;
        int pct = (int)(getUsedLiters(paintedAreaM2) * 100.0f / tankCapacityL);
        if (pct > 100) pct = 100;
        if (pct < 0) pct = 0;
        return pct;
    }

    // Predykcja: ile m2 jeszcze mozna pomalowac
    float getRemainingAreaM2(float paintedAreaM2) const {
        if (consumptionRateL <= 0) return 0;
        return getRemainingLiters(paintedAreaM2) / consumptionRateL;
    }

    // Predykcja: ile metrow liniowych przy danej szerokosci wzorca
    float getRemainingDistanceM(float paintedAreaM2, float patternWidthM) const {
        if (patternWidthM <= 0) return 0;
        return getRemainingAreaM2(paintedAreaM2) / patternWidthM;
    }

private:
    float tankCapacityL = 200.0f;   // Domyslna pojemnosc zbiornika [L]
    float consumptionRateL = DEFAULT_CONSUMPTION_L_PER_M2;
};

extern PaintConsumption paintConsumption;
