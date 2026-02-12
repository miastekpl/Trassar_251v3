#pragma once
// ============================================================
// TrassarV3 - Moduł obsługi przycisków i enkodera
// ============================================================

#include "config.h"

enum ButtonEvent {
    EVT_NONE = 0,
    EVT_START_SHORT,      // Krótkie naciśnięcie Start/Pauza
    EVT_STOP_SHORT,       // Krótkie naciśnięcie Stop
    EVT_STOP_LONG,        // Długie naciśnięcie Stop (wejście w menu)
    EVT_SELECT_SHORT,     // Krótkie naciśnięcie Selektor (przeskocz)
    EVT_SELECT_LONG,      // Długie naciśnięcie Selektor (wejdź)
    EVT_ENC_SHORT,        // Krótkie naciśnięcie enkodera
    EVT_ENC_CW,           // Obrót enkodera w prawo
    EVT_ENC_CCW           // Obrót enkodera w lewo
};

class ButtonHandler {
public:
    void begin();
    void update();
    ButtonEvent getEvent();

private:
    struct ButtonState {
        uint8_t pin;
        bool lastState;
        bool currentState;
        unsigned long pressStart;
        bool longFired;
        bool pendingShort;
        bool pendingLong;
    };

    ButtonState btnStart;
    ButtonState btnStop;
    ButtonState btnSelect;
    ButtonState btnEnc;

    // Enkoder
    volatile int encoderDelta;
    int lastEncoderClk;

    void initButton(ButtonState& btn, uint8_t pin);
    void processButton(ButtonState& btn);

    ButtonEvent pendingEvent;

    static ButtonHandler* instance;
    static void IRAM_ATTR encoderISR();
};

extern ButtonHandler buttons;
