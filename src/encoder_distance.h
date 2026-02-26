#pragma once
// ============================================================
// TrassarV3 - Enkoder kwadraturowy: dystans, prędkość, kalibracja
// x4 dekodowanie — ISR na obu kanalach A+B (CLK+DT)
// ============================================================

#include "config.h"

class EncoderDistance {
public:
    void begin();
    void update();

    // Dystans i prędkość
    float getDistanceMeters() const;
    float getSpeedMps() const;
    float getSpeedKmh() const;
    int64_t getTotalPulses() const;
    void  resetDistance();

    // Kalibracja
    void startCalibration();
    void finishCalibration();
    void cancelCalibration();
    bool isCalibrating() const { return calibrating; }
    float getCalibrationPulses() const;
    float getPulsesPerMeter() const { return pulsesPerMeter; }
    bool  isCalibrated() const { return calibrated; }

    // Zapis/odczyt z NVS
    void loadCalibration();
    void saveCalibration();

    static void IRAM_ATTR encoderISR();

private:
    static EncoderDistance* instance;
    static volatile int64_t totalPulses;

    float pulsesPerMeter = DEFAULT_PULSES_PER_METER;
    bool  calibrated = false;

    // Kalibracja
    bool  calibrating = false;
    int64_t calStartPulses = 0;

    // Prędkość
    float currentSpeed = 0;
    int64_t lastSpeedPulses = 0;
    unsigned long lastSpeedTime = 0;

    // Stan kwadraturowy (2-bit: bit1=A, bit0=B)
    static volatile uint8_t quadState;
    volatile unsigned long lastISRMicros = 0;

    // Tablica dekodowania kwadraturowego x4
    static const int8_t QUAD_TABLE[4][4];
};

extern EncoderDistance encoderDist;
