#pragma once
// ============================================================
// TrassarV3 - HAL (Hardware Abstraction Layer)
// Cienka warstwa nad GPIO/ADC pozwalajaca na:
//   - unit testy na PC (mock)
//   - latwiejszy port na inny MCU
// ============================================================

#include <stdint.h>

#ifdef UNIT_TEST
// ============ Mock HAL dla testow na PC ============

namespace hal {
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t val);
    int  digitalRead(uint8_t pin);
    int  analogRead(uint8_t pin);
    unsigned long millis();
}

#else
// ============ Prawdziwy HAL (ESP32 Arduino) ============

#include <Arduino.h>

namespace hal {
    inline void pinMode(uint8_t pin, uint8_t mode) { ::pinMode(pin, mode); }
    inline void digitalWrite(uint8_t pin, uint8_t val) { ::digitalWrite(pin, val); }
    inline int  digitalRead(uint8_t pin) { return ::digitalRead(pin); }
    inline int  analogRead(uint8_t pin) { return ::analogRead(pin); }
    inline unsigned long millis() { return ::millis(); }
}

#endif
