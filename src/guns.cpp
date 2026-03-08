// ============================================================
// TrassarV3 - Sterowanie przekaźnikami pistoletów
// ============================================================

#include "guns.h"
#include "hal.h"

GunController guns;

void GunController::begin() {
    for (int i = 0; i < NUM_GUNS; i++) {
        hal::pinMode(GUN_PINS[i], OUTPUT);
        hal::digitalWrite(GUN_PINS[i], LOW);
        gunStates[i] = false;
    }
    Serial.println("[GUNS] 6 przekaznikow zainicjalizowanych");
}

void GunController::setGun(GunID gun, bool on) {
    if (gun >= NUM_GUNS) return;
    taskENTER_CRITICAL(&gunMux);
    gunStates[gun] = on;
    taskEXIT_CRITICAL(&gunMux);
    hal::digitalWrite(GUN_PINS[gun], on ? HIGH : LOW);
}

void GunController::allOff() {
    taskENTER_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = false;
    }
    taskEXIT_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        hal::digitalWrite(GUN_PINS[i], LOW);
    }
}

void GunController::allOn() {
    taskENTER_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = true;
    }
    taskEXIT_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        hal::digitalWrite(GUN_PINS[i], HIGH);
    }
}

bool GunController::isOn(GunID gun) const {
    if (gun >= NUM_GUNS) return false;
    taskENTER_CRITICAL(&gunMux);
    bool state = gunStates[gun];
    taskEXIT_CRITICAL(&gunMux);
    return state;
}

bool GunController::getState(int index) const {
    if (index < 0 || index >= NUM_GUNS) return false;
    taskENTER_CRITICAL(&gunMux);
    bool state = gunStates[index];
    taskEXIT_CRITICAL(&gunMux);
    return state;
}
