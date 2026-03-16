#include "sys_log.h"
// ============================================================
// TrassarV3 - Sterowanie przekaźnikami pistoletów
// ============================================================

#include "guns.h"
#include "hal.h"
#include <soc/gpio_struct.h>

GunController guns;

void GunController::begin() {
    for (int i = 0; i < NUM_GUNS; i++) {
        hal::pinMode(GUN_PINS[i], OUTPUT);
        hal::digitalWrite(GUN_PINS[i], LOW);
        gunStates[i] = false;
    }
    DBG_PRINTLN("[GUNS] 6 przekaznikow zainicjalizowanych");
}

void GunController::setGun(GunID gun, bool on) {
    if (gun >= NUM_GUNS) return;
    taskENTER_CRITICAL(&gunMux);
    gunStates[gun] = on;
    hal::digitalWrite(GUN_PINS[gun], on ? HIGH : LOW);
    taskEXIT_CRITICAL(&gunMux);
}

void GunController::allOff() {
    taskENTER_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = false;
        hal::digitalWrite(GUN_PINS[i], LOW);
    }
    taskEXIT_CRITICAL(&gunMux);
}

void GunController::allOn() {
    taskENTER_CRITICAL(&gunMux);
    for (int i = 0; i < NUM_GUNS; i++) {
        gunStates[i] = true;
        hal::digitalWrite(GUN_PINS[i], HIGH);
    }
    taskEXIT_CRITICAL(&gunMux);
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
// Sprzetowy STOP awaryjny — ISR na PIN_BTN_STOP
// Natychmiastowe wylaczenie pistoletow bez debounce/menu.
// Bypass programowej obslugi przycisku — rejestr GPIO bezposrednio.
// ============================================================
void IRAM_ATTR GunController::emergencyStopISR() {
    // Bezposredni zapis do rejestrow GPIO — natychmiastowe LOW na pinach przekaznikow.
    // Nie uzywamy gunMux w ISR — portMUX_TYPE nie jest IRAM-safe w nested ISR.
    // Zamiast tego piszemy bezposrednio do GPIO output register.
    for (int i = 0; i < NUM_GUNS; i++) {
        uint8_t pin = GUN_PINS[i];
        if (pin < 32) {
            GPIO.out_w1tc = (1UL << pin);       // Clear bit = LOW
        } else {
            GPIO.out1_w1tc.val = (1UL << (pin - 32));
        }
    }
    guns.emergencyStopTriggered = true;
}

void GunController::beginEmergencyStop() {
    // ISR na FALLING edge przycisku STOP (aktywny LOW, INPUT_PULLUP)
    // Nie koliduje z button_handler — ISR wylacza pistolety natychmiast,
    // button_handler dalej generuje EVT_STOP_SHORT/LONG dla menu.
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_STOP), emergencyStopISR, FALLING);
    DBG_PRINTLN("[GUNS] Sprzetowy STOP awaryjny (ISR) aktywny na PIN_BTN_STOP");
}
