// ============================================================
// TrassarV3 - Enkoder: dystans, prędkość, kalibracja
// v2.12.0 - Bezposredni odczyt rejestru GPIO w ISR (~50ns vs ~2us digitalRead)
// ============================================================

#include "encoder_distance.h"
#include "storage.h"
#include <soc/gpio_struct.h>

EncoderDistance encoderDist;
EncoderDistance* EncoderDistance::instance = nullptr;
volatile long EncoderDistance::totalPulses = 0;

// Makra do szybkiego odczytu GPIO (piny 0-31 -> GPIO.in, piny 32-39 -> GPIO.in1.val)
#define FAST_GPIO_READ(pin) \
    (((pin) < 32) ? ((GPIO.in >> (pin)) & 1) : ((GPIO.in1.val >> ((pin) - 32)) & 1))

void IRAM_ATTR EncoderDistance::encoderISR() {
    if (!instance) return;

    // Debounce: odrzuc impulsy szybsze niz ENC_ISR_DEBOUNCE_US
    unsigned long nowUs = micros();
    if (nowUs - instance->lastISRMicros < ENC_ISR_DEBOUNCE_US) return;
    instance->lastISRMicros = nowUs;

    // Bezposredni odczyt rejestru GPIO - ~50ns zamiast ~2-3us (digitalRead)
    int clk = FAST_GPIO_READ(PIN_ENC_CLK);
    int dt  = FAST_GPIO_READ(PIN_ENC_DT);
    if (clk != instance->lastClkState) {
        if (dt != clk) {
            totalPulses++;
        } else {
            totalPulses--;
        }
        instance->lastClkState = clk;
    }
}

void EncoderDistance::begin() {
    instance = this;
    totalPulses = 0;
    currentSpeed = 0;
    lastSpeedPulses = 0;
    lastSpeedTime = millis();

    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    lastClkState = digitalRead(PIN_ENC_CLK);

    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, CHANGE);

    loadCalibration();
    Serial.printf("[ENC] Impulsy/metr: %.1f  Skalibrowany: %s\n",
                  pulsesPerMeter, calibrated ? "TAK" : "NIE");
}

void EncoderDistance::update() {
    unsigned long now = millis();
    if (now - lastSpeedTime >= SPEED_CALC_INTERVAL_MS) {
        noInterrupts();
        long currentPulses = totalPulses;
        interrupts();

        long dPulses = currentPulses - lastSpeedPulses;
        float dt = (now - lastSpeedTime) / 1000.0f;

        float instantSpeed = 0;
        if (dt > 0 && pulsesPerMeter > 0) {
            instantSpeed = (float)abs(dPulses) / pulsesPerMeter / dt;
        }

        // Filtr dolnoprzepustowy
        currentSpeed = SPEED_FILTER_ALPHA * instantSpeed +
                       (1.0f - SPEED_FILTER_ALPHA) * currentSpeed;

        lastSpeedPulses = currentPulses;
        lastSpeedTime = now;
    }
}

float EncoderDistance::getDistanceMeters() const {
    noInterrupts();
    long p = totalPulses;
    interrupts();
    return (pulsesPerMeter > 0) ? (float)abs(p) / pulsesPerMeter : 0;
}

float EncoderDistance::getSpeedMps() const {
    return currentSpeed;
}

float EncoderDistance::getSpeedKmh() const {
    return currentSpeed * 3.6f;
}

long EncoderDistance::getTotalPulses() const {
    noInterrupts();
    long p = totalPulses;
    interrupts();
    return p;
}

void EncoderDistance::resetDistance() {
    noInterrupts();
    totalPulses = 0;
    interrupts();
    lastSpeedPulses = 0;
    currentSpeed = 0;
}

// ============ Kalibracja ============

void EncoderDistance::startCalibration() {
    noInterrupts();
    calStartPulses = totalPulses;
    interrupts();
    calibrating = true;
    Serial.println("[CAL] Kalibracja rozpoczeta - przejedz 10m");
}

void EncoderDistance::finishCalibration() {
    if (!calibrating) return;
    noInterrupts();
    long endPulses = totalPulses;
    interrupts();

    long diff = abs(endPulses - calStartPulses);
    if (diff > 10) { // Minimum sensownych impulsów
        pulsesPerMeter = (float)diff / CALIBRATION_DISTANCE_M;
        calibrated = true;
        saveCalibration();
        Serial.printf("[CAL] Kalibracja zakonczona: %ld impulsow / 10m = %.1f imp/m\n",
                      diff, pulsesPerMeter);
    } else {
        Serial.println("[CAL] Za malo impulsow - kalibracja anulowana");
    }
    calibrating = false;
}

void EncoderDistance::cancelCalibration() {
    calibrating = false;
    Serial.println("[CAL] Kalibracja anulowana");
}

float EncoderDistance::getCalibrationPulses() const {
    if (!calibrating) return 0;
    noInterrupts();
    long current = totalPulses;
    interrupts();
    return (float)abs(current - calStartPulses);
}

void EncoderDistance::loadCalibration() {
    bool cal = false;
    float ppm = storage.loadCalibration(cal);
    if (cal && ppm > 1.0f) {
        pulsesPerMeter = ppm;
        calibrated = true;
    }
}

void EncoderDistance::saveCalibration() {
    storage.saveCalibration(pulsesPerMeter);
}
