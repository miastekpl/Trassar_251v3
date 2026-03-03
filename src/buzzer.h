#pragma once
// ============================================================
// TrassarV3 - Modul buzzera (sygnalizacja dzwiekowa)
// Pasywny buzzer na GPIO, sterowany LEDC PWM
// Non-blocking: update() w loop() obsluguje sekwencje tonow
// ============================================================

#include "config.h"

// Typy predefiniowanych sygnalow
enum BuzzerSignal : uint8_t {
    BUZ_NONE = 0,
    BUZ_PAINT_START,       // Start malowania - krotki potwierdzajacy
    BUZ_PAINT_STOP,        // Stop malowania - podwojny krotki
    BUZ_LOW_SPEED,         // Predkosc < 3 km/h podczas malowania
    BUZ_OVERSPEED,         // Przekroczenie predkosci maks.
    BUZ_GUN_ANOMALY,       // Pistolet nie strzela mimo aktywnej konfiguracji
    BUZ_ERROR,             // Blad (brak SD, RTC niedostepny)
    BUZ_PATTERN_CHANGE,    // Zmiana wzorca przyciskiem MCP23017
    BUZ_SD_WARNING,        // Ostrzezenie: brak karty SD przy starcie malowania
    BUZ_AUTO_PAUSE         // Auto-pauza przy zatrzymaniu
};

// Pojedynczy krok sekwencji tonowej
struct BuzzerStep {
    uint16_t freq;         // Czestotliwosc Hz (0 = cisza/pauza)
    uint16_t durationMs;   // Czas trwania [ms]
};

class BuzzerController {
public:
    void begin();
    void update();         // Wywolywana w loop() - obsluga sekwencji

    // Odtwarzanie predefiniowanych sygnalow
    void play(BuzzerSignal signal);

    // Bezposredni beep (nadpisuje biezaca sekwencje)
    void beep(uint16_t freq, uint16_t durationMs);

    // Zatrzymanie
    void stop();

    bool isPlaying() const { return playing; }

private:
    void toneOn(uint16_t freq);
    void toneOff();
    void startSequence(const BuzzerStep* seq, uint8_t len);

    bool playing = false;
    const BuzzerStep* currentSeq = nullptr;
    uint8_t seqLen = 0;
    uint8_t seqIdx = 0;
    unsigned long stepStartMs = 0;
};

extern BuzzerController buzzer;
