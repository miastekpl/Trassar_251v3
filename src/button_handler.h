#pragma once
// ============================================================
// TrassarV3 - Obsługa przycisków
// ============================================================

#include "config.h"

enum ButtonEvent : uint8_t {
    EVT_NONE = 0,
    EVT_START_SHORT,
    EVT_STOP_SHORT,
    EVT_STOP_LONG,
    EVT_SELECT_SHORT,
    EVT_SELECT_LONG
};

class ButtonHandler {
public:
    void begin();
    void update();
    ButtonEvent getEvent();

private:
    struct BtnState {
        uint8_t pin;
        bool lastReading;
        bool pressed;
        unsigned long pressStart;
        bool longFired;
        bool pendingShort;
        bool pendingLong;
    };

    BtnState btnStart;
    BtnState btnStop;
    BtnState btnSelect;

    void initBtn(BtnState& b, uint8_t pin);
    void processBtn(BtnState& b);

public:
    // Surowy stan przycisku START (dla czyszczenia dysz)
    bool isStartHeld() const { return btnStart.pressed; }
};

extern ButtonHandler buttons;
