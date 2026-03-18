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

    // ---- Sledzenie aktualnego poziomu farby w zbiorniku ----

    // Aktualny poziom farby [litry]
    float getCurrentLevel() const { return currentLevelL; }

    // Tankowanie: dodaj farbe do zbiornika
    void refuel(float liters);

    // Odejmij zuzycie farby na podstawie przyrostu powierzchni [m2]
    void subtractUsage(float areaDeltaM2);

    // Reset poziomu do pelnego zbiornika
    void resetLevel();

    // Historia tankowan
    float getLastRefuelAmount() const { return lastRefuelAmountL; }
    uint32_t getTotalRefuelCount() const { return refuelCount; }
    float getTotalRefueledL() const { return totalRefueledL; }

private:
    float tankCapacityL = 200.0f;   // Domyslna pojemnosc zbiornika [L]
    float consumptionRateL = DEFAULT_CONSUMPTION_L_PER_M2;
    float currentLevelL = 200.0f;   // Aktualny poziom farby w zbiorniku [L]
    float lastRefuelAmountL = 0;    // Ostatnie tankowanie [L]
    uint32_t refuelCount = 0;       // Licznik tankowan
    float totalRefueledL = 0;       // Lacznie zatankowano [L]
};

extern PaintConsumption paintConsumption;
