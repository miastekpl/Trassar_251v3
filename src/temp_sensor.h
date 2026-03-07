#pragma once
// ============================================================
// TrassarV3 - Czujnik temperatury (DS18B20 / opcjonalny)
// Ostrzezenie <5°C lub >35°C (wplyw na schnięcie farby)
// ============================================================

#include "config.h"

// Pin czujnika DS18B20 (OneWire)
#define PIN_TEMP_SENSOR   15    // Wolny GPIO (opcjonalny czujnik)
#define TEMP_READ_INTERVAL_MS  5000  // Odczyt co 5s

// Progi ostrzezen temperatury
#define TEMP_WARNING_LOW   5.0f   // <5°C - farba za zimna
#define TEMP_WARNING_HIGH  35.0f  // >35°C - farba za ciepla

class TempSensor {
public:
    void begin();
    void update();

    bool isAvailable() const { return sensorFound; }
    float getTemperature() const { return temperature; }
    bool isWarningLow() const { return sensorFound && temperature < TEMP_WARNING_LOW; }
    bool isWarningHigh() const { return sensorFound && temperature > TEMP_WARNING_HIGH; }
    bool hasWarning() const { return isWarningLow() || isWarningHigh(); }

private:
    bool sensorFound = false;
    float temperature = 20.0f;
    unsigned long lastReadMs = 0;

    bool readTemperature();
    // Prosty protokol OneWire dla DS18B20 (bez biblioteki)
    bool owReset();
    void owWrite(uint8_t data);
    uint8_t owRead();
};

extern TempSensor tempSensor;
