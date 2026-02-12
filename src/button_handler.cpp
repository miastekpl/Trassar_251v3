// ============================================================
// TrassarV3 - Implementacja obsługi przycisków
// Enkoder ISR jest w encoder_distance - tu pobieramy delta.
// ============================================================

#include "button_handler.h"

ButtonHandler buttons;

void ButtonHandler::begin() {
    initBtn(btnStart, PIN_BTN_START);
    initBtn(btnStop, PIN_BTN_STOP);
    initBtn(btnSelect, PIN_BTN_SELECT);
}

void ButtonHandler::initBtn(BtnState& b, uint8_t pin) {
    b.pin = pin;
    b.lastReading = true;
    b.pressed = false;
    b.pressStart = 0;
    b.longFired = false;
    b.pendingShort = false;
    b.pendingLong = false;
    pinMode(pin, INPUT_PULLUP);
}

void ButtonHandler::processBtn(BtnState& b) {
    bool reading = digitalRead(b.pin);

    if (reading != b.lastReading) {
        b.lastReading = reading;
        return;
    }

    if (reading == LOW && !b.pressed) {
        b.pressed = true;
        b.pressStart = millis();
        b.longFired = false;
    }
    else if (reading == LOW && b.pressed) {
        if (!b.longFired && (millis() - b.pressStart >= BTN_LONG_PRESS_MS)) {
            b.longFired = true;
            b.pendingLong = true;
        }
    }
    else if (reading == HIGH && b.pressed) {
        b.pressed = false;
        if (!b.longFired) {
            b.pendingShort = true;
        }
    }
}

void ButtonHandler::update() {
    processBtn(btnStart);
    processBtn(btnStop);
    processBtn(btnSelect);
}

ButtonEvent ButtonHandler::getEvent() {
    if (btnStart.pendingShort)  { btnStart.pendingShort = false; return EVT_START_SHORT; }
    if (btnStop.pendingLong)    { btnStop.pendingLong = false;   return EVT_STOP_LONG; }
    if (btnStop.pendingShort)   { btnStop.pendingShort = false;  return EVT_STOP_SHORT; }
    if (btnSelect.pendingLong)  { btnSelect.pendingLong = false; return EVT_SELECT_LONG; }
    if (btnSelect.pendingShort) { btnSelect.pendingShort = false;return EVT_SELECT_SHORT; }

    return EVT_NONE;
}
