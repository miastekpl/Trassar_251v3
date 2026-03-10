// ============================================================
// TrassarV3 - Implementacja obsługi przycisków
// 4 przyciski: START, STOP, SELEKTOR, START OD PRZERWY
// ============================================================

#include "button_handler.h"

ButtonHandler buttons;

void ButtonHandler::begin() {
    initBtn(btnStart, PIN_BTN_START);
    initBtn(btnStop, PIN_BTN_STOP);
    initBtn(btnSelect, PIN_BTN_SELECT);
    initBtn(btnGap, PIN_BTN_GAP);
}

void ButtonHandler::initBtn(BtnState& b, uint8_t pin) {
    b.pin = pin;
    b.lastReading = true;
    b.stableState = true;
    b.pressed = false;
    b.lastChangeMs = 0;
    b.pressStart = 0;
    b.longFired = false;
    b.pendingShort = false;
    b.pendingLong = false;
    pinMode(pin, INPUT_PULLUP);
}

void ButtonHandler::processBtn(BtnState& b) {
    bool reading = digitalRead(b.pin);
    unsigned long now = millis();

    // Restart debounce timer przy kazdej zmianie odczytu
    if (reading != b.lastReading) {
        b.lastChangeMs = now;
        b.lastReading = reading;
    }

    // Akceptuj nowy stan dopiero po BTN_DEBOUNCE_MS stabilnego odczytu
    if ((now - b.lastChangeMs) < BTN_DEBOUNCE_MS) return;

    // Odczyt stabilny — sprawdz czy stan sie zmienil
    if (reading == b.stableState) {
        // Stan sie nie zmienil — sprawdz long press
        if (reading == LOW && b.pressed) {
            if (!b.longFired && (now - b.pressStart >= BTN_LONG_PRESS_MS)) {
                b.longFired = true;
                b.pendingLong = true;
            }
        }
        return;
    }

    // Nowy stabilny stan
    b.stableState = reading;

    if (reading == LOW && !b.pressed) {
        b.pressed = true;
        b.pressStart = now;
        b.longFired = false;
    }
    else if (reading == HIGH && b.pressed) {
        b.pressed = false;
        if (!b.longFired) {
            b.pendingShort = true;
        }
    }
}

void ButtonHandler::processCombo() {
    bool bothHeld = btnStart.pressed && btnStop.pressed;

    if (bothHeld && !comboActive) {
        // Oba przyciski wlasnie wcisniete razem
        comboActive = true;
        comboStart = millis();
        comboFired = false;
    } else if (bothHeld && comboActive && !comboFired) {
        if (millis() - comboStart >= BTN_LONG_PRESS_MS) {
            comboFired = true;
            pendingCombo = true;
            // Anuluj indywidualne long events — combo ma priorytet
            btnStart.longFired = true;
            btnStart.pendingLong = false;
            btnStop.longFired = true;
            btnStop.pendingLong = false;
        }
    } else if (!bothHeld) {
        if (comboActive && comboFired) {
            // Combo bylo aktywne — anuluj pendingShort z obu przyciskow
            btnStart.pendingShort = false;
            btnStop.pendingShort = false;
        }
        comboActive = false;
    }
}

void ButtonHandler::update() {
    processBtn(btnStart);
    processBtn(btnStop);
    processBtn(btnSelect);
    processBtn(btnGap);
    processCombo();
}

ButtonEvent ButtonHandler::getEvent() {
    // Combo START+STOP ma najwyzszy priorytet
    if (pendingCombo) { pendingCombo = false; return EVT_START_STOP_COMBO; }

    if (btnStart.pendingLong)   { btnStart.pendingLong = false;  return EVT_START_LONG; }
    if (btnStart.pendingShort)  { btnStart.pendingShort = false; return EVT_START_SHORT; }
    if (btnStop.pendingLong)    { btnStop.pendingLong = false;   return EVT_STOP_LONG; }
    if (btnStop.pendingShort)   { btnStop.pendingShort = false;  return EVT_STOP_SHORT; }
    if (btnSelect.pendingLong)  { btnSelect.pendingLong = false; return EVT_SELECT_LONG; }
    if (btnSelect.pendingShort) { btnSelect.pendingShort = false;return EVT_SELECT_SHORT; }
    if (btnGap.pendingShort)    { btnGap.pendingShort = false;   return EVT_GAP_START; }

    return EVT_NONE;
}
