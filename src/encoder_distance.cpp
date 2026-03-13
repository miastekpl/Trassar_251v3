// ============================================================
// TrassarV3 - Enkoder kwadraturowy: dystans, prędkość, kalibracja
// v2.21.0 - Pelne dekodowanie kwadraturowe x4 (oba kanaly A+B)
//           ISR na CLK(A) i DT(B) CHANGE — 4x rozdzielczosc
//           Tablica stanow (4x4) do niezawodnego dekodowania kierunku
//           Bezposredni odczyt rejestru GPIO (~50ns)
// ============================================================

#include "encoder_distance.h"
#include "storage.h"
#include <soc/gpio_struct.h>

EncoderDistance encoderDist;
EncoderDistance* EncoderDistance::instance = nullptr;
volatile long EncoderDistance::totalPulses = 0;
volatile uint8_t EncoderDistance::quadState = 0;
portMUX_TYPE EncoderDistance::encMux = portMUX_INITIALIZER_UNLOCKED;

// Makra do szybkiego odczytu GPIO (piny 0-31 -> GPIO.in, piny 32-39 -> GPIO.in1.val)
#define FAST_GPIO_READ(pin) \
    (((pin) < 32) ? ((GPIO.in >> (pin)) & 1) : ((GPIO.in1.val >> ((pin) - 32)) & 1))

// ============================================================
// Tablica dekodowania kwadraturowego x4
// Stan = (A << 1) | B  →  wartosci 0..3
// Przejscia stanow:
//   Do przodu (CW):  00→01→11→10→00  (+1 na kazde przejscie)
//   Do tylu  (CCW):  00→10→11→01→00  (-1 na kazde przejscie)
//   Brak zmiany lub nieprawidlowe: 0
// ============================================================
const int8_t EncoderDistance::QUAD_TABLE[4][4] = {
    //  00  01  10  11   ← nowy stan
    {  0, +1, -1,  0 }, // prev = 00
    { -1,  0,  0, +1 }, // prev = 01
    { +1,  0,  0, -1 }, // prev = 10
    {  0, -1, +1,  0 }  // prev = 11
};

void IRAM_ATTR EncoderDistance::encoderISR() {
    if (!instance) return;

    // Debounce: odrzuc impulsy szybsze niz ENC_ISR_DEBOUNCE_US
    unsigned long nowUs = micros();
    if (nowUs - instance->lastISRMicros < ENC_ISR_DEBOUNCE_US) return;
    instance->lastISRMicros = nowUs;

    // Bezposredni odczyt rejestru GPIO - ~50ns zamiast ~2-3us (digitalRead)
    uint8_t a = FAST_GPIO_READ(PIN_ENC_CLK);
    uint8_t b = FAST_GPIO_READ(PIN_ENC_DT);
    uint8_t newState = (a << 1) | b;

    // Dekodowanie kierunku z tablicy stanow
    int8_t delta = QUAD_TABLE[quadState][newState];
    taskENTER_CRITICAL_ISR(&encMux);
    if (delta != 0) {
        totalPulses += delta;
    }
    quadState = newState;
    taskEXIT_CRITICAL_ISR(&encMux);
}

void EncoderDistance::begin() {
    instance = this;
    totalPulses = 0;
    currentSpeed = 0;
    lastSpeedPulses = 0;
    lastSpeedTime = millis();

    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);

    // Inicjalizacja stanu kwadraturowego
    uint8_t a = digitalRead(PIN_ENC_CLK);
    uint8_t b = digitalRead(PIN_ENC_DT);
    quadState = (a << 1) | b;

    // ISR na obu kanalach — x4 rozdzielczosc
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT),  encoderISR, CHANGE);

    loadCalibration();
    Serial.printf("[ENC] Kwadraturowy x4 | Impulsy/metr: %.1f  Skalibrowany: %s\n",
                  pulsesPerMeter, calibrated ? "TAK" : "NIE");
}

void EncoderDistance::update() {
    unsigned long now = millis();
    if (now - lastSpeedTime >= SPEED_CALC_INTERVAL_MS) {
        taskENTER_CRITICAL(&encMux);
        long currentPulses = totalPulses;
        taskEXIT_CRITICAL(&encMux);

        long dPulses = currentPulses - lastSpeedPulses;
        float dt = (now - lastSpeedTime) / 1000.0f;

        float instantSpeed = 0;
        if (dt > 0 && pulsesPerMeter > 0) {
            instantSpeed = (float)abs(dPulses) / pulsesPerMeter / dt;
        }

        // Detekcja calkowitego zatrzymania: jesli brak impulsow w calym oknie,
        // natychmiast zeruj predkosc (bypass filtra dolnoprzepustowego).
        // Filtr EMA z alpha=0.3 potrzebuje ~5 cykli zeby opasc do zera,
        // co maskuje krotkie zatrzymania (np. kraweznik, skrzyzowanie).
        if (dPulses == 0) {
            // Zero impulsow = calkowite zatrzymanie — natychmiastowy zero
            currentSpeed = 0;
            zeroSpeedCount++;
        } else {
            zeroSpeedCount = 0;
            // Filtr dolnoprzepustowy (EMA) — tylko gdy pojazd jedzie
            currentSpeed = SPEED_FILTER_ALPHA * instantSpeed +
                           (1.0f - SPEED_FILTER_ALPHA) * currentSpeed;
        }

        lastSpeedPulses = currentPulses;
        lastSpeedTime = now;
    }
}

float EncoderDistance::getDistanceMeters() const {
    taskENTER_CRITICAL(&encMux);
    long p = totalPulses;
    taskEXIT_CRITICAL(&encMux);
    return (pulsesPerMeter > 0) ? (float)abs(p) / pulsesPerMeter : 0;
}

float EncoderDistance::getSpeedMps() const {
    return currentSpeed;
}

float EncoderDistance::getSpeedKmh() const {
    return currentSpeed * 3.6f;
}

long EncoderDistance::getTotalPulses() const {
    taskENTER_CRITICAL(&encMux);
    long p = totalPulses;
    taskEXIT_CRITICAL(&encMux);
    return p;
}

void EncoderDistance::resetDistance() {
    taskENTER_CRITICAL(&encMux);
    totalPulses = 0;
    taskEXIT_CRITICAL(&encMux);
    lastSpeedPulses = 0;
    currentSpeed = 0;
}

// ============ Kalibracja ============

void EncoderDistance::startCalibration() {
    taskENTER_CRITICAL(&encMux);
    calStartPulses = totalPulses;
    taskEXIT_CRITICAL(&encMux);
    calibrating = true;
    Serial.println("[CAL] Kalibracja rozpoczeta - przejedz 10m");
}

void EncoderDistance::finishCalibration() {
    if (!calibrating) return;
    taskENTER_CRITICAL(&encMux);
    long endPulses = totalPulses;
    taskEXIT_CRITICAL(&encMux);

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
    taskENTER_CRITICAL(&encMux);
    long current = totalPulses;
    taskEXIT_CRITICAL(&encMux);
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
