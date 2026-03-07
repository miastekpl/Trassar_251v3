// ============================================================
// TrassarV3 - Predykcja zuzycia farby
// ============================================================

#include "paint_consumption.h"

PaintConsumption paintConsumption;

void PaintConsumption::begin() {
    tankCapacityL = 200.0f;
    consumptionRateL = DEFAULT_CONSUMPTION_L_PER_M2;
}
