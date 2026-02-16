// ============================================================
// TrassarV3 - Modul buzzera (sygnalizacja dzwiekowa)
// Pasywny buzzer sterowany LEDC PWM, non-blocking
// ============================================================

#include "buzzer.h"

BuzzerController buzzer;

// ============ Predefiniowane sekwencje ============

// Start malowania: krotki beep 2kHz 100ms
static const BuzzerStep SEQ_PAINT_START[] = {
    {2000, 100}
};

// Stop malowania: podwojny krotki beep
static const BuzzerStep SEQ_PAINT_STOP[] = {
    {2000, 80},
    {0,    80},   // pauza
    {2000, 80}
};

// Niska predkosc: wolny puls 1.5kHz
static const BuzzerStep SEQ_LOW_SPEED[] = {
    {1500, 150},
    {0,    100},
    {1500, 150}
};

// Przekroczenie predkosci: szybki trojkowy alarm 3kHz
static const BuzzerStep SEQ_OVERSPEED[] = {
    {3000, 60},
    {0,    40},
    {3000, 60},
    {0,    40},
    {3000, 60}
};

// Anomalia pistoletu: dlugi niski ton + krotki wysoki (uwaga mechaniczna)
static const BuzzerStep SEQ_GUN_ANOMALY[] = {
    {800,  300},
    {0,    100},
    {1200, 150},
    {0,    100},
    {800,  300}
};

// Blad: niski ton opadajacy
static const BuzzerStep SEQ_ERROR[] = {
    {1000, 200},
    {0,    50},
    {800,  200},
    {0,    50},
    {600,  300}
};

// ============ Implementacja ============

void BuzzerController::begin() {
    ledcSetup(BUZZER_LEDC_CH, 2000, 8);
    ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CH);
    ledcWrite(BUZZER_LEDC_CH, 0);  // Cisza
    Serial.println("[BUZZER] Zainicjalizowany na GPIO " + String(PIN_BUZZER));
}

void BuzzerController::update() {
    if (!playing || currentSeq == nullptr) return;

    unsigned long now = millis();
    if (now - stepStartMs >= currentSeq[seqIdx].durationMs) {
        // Przejdz do nastepnego kroku
        seqIdx++;
        if (seqIdx >= seqLen) {
            // Koniec sekwencji
            toneOff();
            playing = false;
            currentSeq = nullptr;
            return;
        }
        // Nowy krok
        stepStartMs = now;
        if (currentSeq[seqIdx].freq > 0) {
            toneOn(currentSeq[seqIdx].freq);
        } else {
            toneOff();
        }
    }
}

void BuzzerController::play(BuzzerSignal signal) {
    switch (signal) {
        case BUZ_PAINT_START:
            startSequence(SEQ_PAINT_START, sizeof(SEQ_PAINT_START) / sizeof(BuzzerStep));
            break;
        case BUZ_PAINT_STOP:
            startSequence(SEQ_PAINT_STOP, sizeof(SEQ_PAINT_STOP) / sizeof(BuzzerStep));
            break;
        case BUZ_LOW_SPEED:
            startSequence(SEQ_LOW_SPEED, sizeof(SEQ_LOW_SPEED) / sizeof(BuzzerStep));
            break;
        case BUZ_OVERSPEED:
            startSequence(SEQ_OVERSPEED, sizeof(SEQ_OVERSPEED) / sizeof(BuzzerStep));
            break;
        case BUZ_GUN_ANOMALY:
            startSequence(SEQ_GUN_ANOMALY, sizeof(SEQ_GUN_ANOMALY) / sizeof(BuzzerStep));
            break;
        case BUZ_ERROR:
            startSequence(SEQ_ERROR, sizeof(SEQ_ERROR) / sizeof(BuzzerStep));
            break;
        default:
            break;
    }
}

void BuzzerController::beep(uint16_t freq, uint16_t durationMs) {
    static BuzzerStep singleStep;
    singleStep = {freq, durationMs};
    startSequence(&singleStep, 1);
}

void BuzzerController::stop() {
    toneOff();
    playing = false;
    currentSeq = nullptr;
}

void BuzzerController::toneOn(uint16_t freq) {
    ledcWriteTone(BUZZER_LEDC_CH, freq);
    ledcWrite(BUZZER_LEDC_CH, 128);  // 50% duty cycle
}

void BuzzerController::toneOff() {
    ledcWrite(BUZZER_LEDC_CH, 0);
}

void BuzzerController::startSequence(const BuzzerStep* seq, uint8_t len) {
    currentSeq = seq;
    seqLen = len;
    seqIdx = 0;
    stepStartMs = millis();
    playing = true;

    if (seq[0].freq > 0) {
        toneOn(seq[0].freq);
    } else {
        toneOff();
    }
}
