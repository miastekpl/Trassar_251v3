#pragma once
// ============================================================
// TrassarV3 - Enkoder: dystans, prędkość, kalibracja
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
    long  getTotalPulses() const;
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
    static volatile long totalPulses;

    float pulsesPerMeter = DEFAULT_PULSES_PER_METER;
    bool  calibrated = false;

    // Kalibracja
    bool  calibrating = false;
    long  calStartPulses = 0;

    // Prędkość
    float currentSpeed = 0;
    long  lastSpeedPulses = 0;
    unsigned long lastSpeedTime = 0;

    int lastClkState = 0;
    volatile unsigned long lastISRMicros = 0;
};

extern EncoderDistance encoderDist;
