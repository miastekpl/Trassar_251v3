// ============================================================
// TrassarV3 - Czujnik temperatury DS18B20
// Prosty sterownik OneWire bez zewnetrznej biblioteki
// ============================================================

#include "temp_sensor.h"

TempSensor tempSensor;

void TempSensor::begin() {
    pinMode(PIN_TEMP_SENSOR, INPUT_PULLUP);

    // Sprawdz czy czujnik jest obecny
    sensorFound = owReset();
    if (sensorFound) {
        Serial.println("[TEMP] Czujnik DS18B20 wykryty");
        // Pierwszy odczyt
        readTemperature();
    } else {
        Serial.println("[TEMP] Brak czujnika temperatury (opcjonalny)");
    }
}

void TempSensor::update() {
    if (!sensorFound) return;

    unsigned long now = millis();
    if (now - lastReadMs >= TEMP_READ_INTERVAL_MS) {
        lastReadMs = now;
        readTemperature();
    }
}

bool TempSensor::readTemperature() {
    // Krok 1: Skip ROM + Convert T
    if (!owReset()) { sensorFound = false; return false; }
    owWrite(0xCC);  // Skip ROM (jeden czujnik na magistrali)
    owWrite(0x44);  // Convert T

    // Czekaj na konwersje (750ms dla 12-bit, ale czytamy wynik przy nastepnym update)
    delay(1);  // Minimalny delay

    // Krok 2: Skip ROM + Read Scratchpad
    if (!owReset()) return false;
    owWrite(0xCC);  // Skip ROM
    owWrite(0xBE);  // Read Scratchpad

    uint8_t data[9];
    for (int i = 0; i < 9; i++) {
        data[i] = owRead();
    }

    // CRC check (prosty — sprawdz czy dane nie sa same 0xFF)
    if (data[0] == 0xFF && data[1] == 0xFF) return false;

    // Konwersja na temperature (12-bit)
    int16_t raw = (data[1] << 8) | data[0];
    temperature = raw / 16.0f;

    // Sanity check
    if (temperature < -55.0f || temperature > 125.0f) return false;

    return true;
}

// ============ OneWire - niskopoziomowe operacje ============

bool TempSensor::owReset() {
    pinMode(PIN_TEMP_SENSOR, OUTPUT);
    digitalWrite(PIN_TEMP_SENSOR, LOW);
    delayMicroseconds(480);
    pinMode(PIN_TEMP_SENSOR, INPUT_PULLUP);
    delayMicroseconds(70);
    bool presence = (digitalRead(PIN_TEMP_SENSOR) == LOW);
    delayMicroseconds(410);
    return presence;
}

void TempSensor::owWrite(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x01) {
            // Write 1
            pinMode(PIN_TEMP_SENSOR, OUTPUT);
            digitalWrite(PIN_TEMP_SENSOR, LOW);
            delayMicroseconds(6);
            pinMode(PIN_TEMP_SENSOR, INPUT_PULLUP);
            delayMicroseconds(64);
        } else {
            // Write 0
            pinMode(PIN_TEMP_SENSOR, OUTPUT);
            digitalWrite(PIN_TEMP_SENSOR, LOW);
            delayMicroseconds(60);
            pinMode(PIN_TEMP_SENSOR, INPUT_PULLUP);
            delayMicroseconds(10);
        }
        data >>= 1;
    }
}

uint8_t TempSensor::owRead() {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        pinMode(PIN_TEMP_SENSOR, OUTPUT);
        digitalWrite(PIN_TEMP_SENSOR, LOW);
        delayMicroseconds(6);
        pinMode(PIN_TEMP_SENSOR, INPUT_PULLUP);
        delayMicroseconds(9);
        if (digitalRead(PIN_TEMP_SENSOR)) {
            data |= (1 << i);
        }
        delayMicroseconds(55);
    }
    return data;
}
