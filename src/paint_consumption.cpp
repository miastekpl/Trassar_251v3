#include "sys_log.h"
// ============================================================
// TrassarV3 - Predykcja zuzycia farby
// ============================================================

#include "paint_consumption.h"
#include "storage.h"

PaintConsumption paintConsumption;

void PaintConsumption::begin() {
    tankCapacityL = storage.loadTankCapacity();
    consumptionRateL = storage.loadConsumptionRate();
    currentLevelL = storage.loadPaintLevel();
    refuelCount = storage.loadRefuelCount();
    totalRefueledL = storage.loadTotalRefueled();
    // Przy pierwszym uruchomieniu (brak zapisu) — ustaw na pojemnosc zbiornika
    if (currentLevelL < 0) currentLevelL = tankCapacityL;
    DBG_PRINTF("[PAINT] Zbiornik: %.0f L, zuzycie: %.2f l/m2, poziom: %.1f L, tankowan: %u\n",
                  tankCapacityL, consumptionRateL, currentLevelL, refuelCount);
}

void PaintConsumption::refuel(float liters) {
    if (liters <= 0) return;
    currentLevelL += liters;
    if (currentLevelL > tankCapacityL) currentLevelL = tankCapacityL;
    lastRefuelAmountL = liters;
    refuelCount++;
    totalRefueledL += liters;
    storage.savePaintLevel(currentLevelL);
    storage.saveRefuelCount(refuelCount);
    storage.saveTotalRefueled(totalRefueledL);
    DBG_PRINTF("[PAINT] Tankowanie: +%.1f L, poziom: %.1f L (tankowanie #%u)\n",
                  liters, currentLevelL, refuelCount);
}

void PaintConsumption::subtractUsage(float areaDeltaM2) {
    if (areaDeltaM2 <= 0) return;
    float used = areaDeltaM2 * consumptionRateL;
    currentLevelL -= used;
    if (currentLevelL < 0) currentLevelL = 0;
}

void PaintConsumption::resetLevel() {
    currentLevelL = tankCapacityL;
    storage.savePaintLevel(currentLevelL);
}
