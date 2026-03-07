// ============================================================
// TrassarV3 - Predykcja zuzycia farby
// ============================================================

#include "paint_consumption.h"
#include "storage.h"

PaintConsumption paintConsumption;

void PaintConsumption::begin() {
    tankCapacityL = storage.loadTankCapacity();
    consumptionRateL = storage.loadConsumptionRate();
    Serial.printf("[PAINT] Zbiornik: %.0f L, zuzycie: %.2f l/m2\n",
                  tankCapacityL, consumptionRateL);
}
