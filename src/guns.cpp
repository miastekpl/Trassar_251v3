// ============================================================
// TrassarV3 - Sterowanie przekaźnikami pistoletów
// Thread-safe: portMUX spinlock chroni gunStates[]
// ============================================================

#include "guns.h"

GunController guns;

// Makro do wymuszenia stanu LOW na pinie GPIO (rejestr sprzętowy)
#define FORCE_GPIO_LOW(pin) do { \
    if ((pin) < 32) GPIO.out_w1tc = (1U << (pin)); \
    else GPIO.out1_w1tc.val = (1U << ((pin) - 32)); \
} while(0)

void GunController::begin() {
    for (int i = 0; i < NUM_GUNS; i++) {
        pinMode(GUN_PINS[i], OUTPUT);
        digitalWrite(GUN_PINS[i], LOW);
        gunStates[i] = false;
    }
    Serial.println("[GUNS] 6 przekaznikow zainicjalizowanych (mutex ON)");
}

void GunController::setGun(GunID gun, bool on) {
    if (gun >= NUM_GUNS) return;
    taskENTER_CRITICAL(&gunMux);
    gunStates[gun] = on;
    taskEXIT_CRITICAL(&gunMux);
    digitalWrite(GUN_PINS[gun], on ? HIGH : LOW);
}

void GunController::allOff() {
    taskENTER_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = false;
    }
    taskEXIT_CRITICAL(&gunMux);
    // GPIO po wyjsciu z sekcji krytycznej (digitalWrite jest wolne)
    for (int i = 0; i < NUM_GUNS; i++) {
        digitalWrite(GUN_PINS[i], LOW);
    }
}

void GunController::allOn() {
    taskENTER_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = true;
    }
    taskEXIT_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        digitalWrite(GUN_PINS[i], HIGH);
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

// ============================================================
// Awaryjne wylaczenie — IRAM_ATTR, bezposredni zapis rejestrow
// Bezpieczne z: ISR, panic handler, watchdog callback
// NIE uzywa mutexa, digitalRead, Serial, ani RAM-u
// ============================================================
void IRAM_ATTR GunController::forceAllOffISR() {
    for (int i = 0; i < NUM_GUNS; i++) {
        FORCE_GPIO_LOW(GUN_PINS[i]);
    }
}
