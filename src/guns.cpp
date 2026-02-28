// ============================================================
// TrassarV3 - Sterowanie przekaźnikami pistoletów
// ============================================================

#include "guns.h"

GunController guns;

void GunController::begin() {
    for (int i = 0; i < NUM_GUNS; i++) {
        pinMode(GUN_PINS[i], OUTPUT);
        digitalWrite(GUN_PINS[i], LOW);
        gunStates[i] = false;
    }
    Serial.println("[GUNS] 6 przekaznikow zainicjalizowanych");
}

void GunController::setGun(GunID gun, bool on) {
    if (gun >= NUM_GUNS) return;
    gunStates[gun] = on;
    digitalWrite(GUN_PINS[gun], on ? HIGH : LOW);
}

void GunController::allOff() {
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = false;
        digitalWrite(GUN_PINS[i], LOW);
    }
}

void GunController::allOn() {
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = true;
        digitalWrite(GUN_PINS[i], HIGH);
    }
}

bool GunController::isOn(GunID gun) const {
    if (gun >= NUM_GUNS) return false;
    return gunStates[gun];
}
