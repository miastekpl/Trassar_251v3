// ============================================================
// TrassarV3 - Implementacja modułu przycisków i enkodera
// ============================================================

#include "button_handler.h"

ButtonHandler buttons;
ButtonHandler* ButtonHandler::instance = nullptr;

void ButtonHandler::begin() {
    instance = this;
    pendingEvent = EVT_NONE;
    encoderDelta = 0;

    initButton(btnStart, PIN_BTN_START);
    initButton(btnStop, PIN_BTN_STOP);
    initButton(btnSelect, PIN_BTN_SELECT);
    initButton(btnEnc, PIN_ENC_SW);

    // Enkoder - piny z pullup
    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    lastEncoderClk = digitalRead(PIN_ENC_CLK);

    // Przerwanie na pinie CLK enkodera
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, CHANGE);
}

void ButtonHandler::initButton(ButtonState& btn, uint8_t pin) {
    btn.pin = pin;
    btn.lastState = true;  // pullup = HIGH gdy nie naciśnięty
    btn.currentState = true;
    btn.pressStart = 0;
    btn.longFired = false;
    btn.pendingShort = false;
    btn.pendingLong = false;
    pinMode(pin, INPUT_PULLUP);
}

void IRAM_ATTR ButtonHandler::encoderISR() {
    if (!instance) return;

    int clkState = digitalRead(PIN_ENC_CLK);
    if (clkState != instance->lastEncoderClk) {
        int dtState = digitalRead(PIN_ENC_DT);
        if (dtState != clkState) {
            instance->encoderDelta++;   // CW
        } else {
            instance->encoderDelta--;   // CCW
        }
        instance->lastEncoderClk = clkState;
    }
}

void ButtonHandler::processButton(ButtonState& btn) {
    bool reading = digitalRead(btn.pin);

    // Debounce
    if (reading != btn.lastState) {
        btn.lastState = reading;
        return;  // Czekaj na stabilizację
    }

    if (reading == LOW && btn.currentState == true) {
        // Naciśnięcie (zbocze opadające - aktywny LOW)
        btn.currentState = false;
        btn.pressStart = millis();
        btn.longFired = false;
    }
    else if (reading == LOW && btn.currentState == false) {
        // Przytrzymanie
        if (!btn.longFired && (millis() - btn.pressStart >= BTN_LONG_PRESS_MS)) {
            btn.longFired = true;
            btn.pendingLong = true;
        }
    }
    else if (reading == HIGH && btn.currentState == false) {
        // Zwolnienie
        btn.currentState = true;
        if (!btn.longFired) {
            btn.pendingShort = true;
        }
    }
}

void ButtonHandler::update() {
    processButton(btnStart);
    processButton(btnStop);
    processButton(btnSelect);
    processButton(btnEnc);
}

ButtonEvent ButtonHandler::getEvent() {
    // Priorytet zdarzeń: długie > krótkie > enkoder

    // Start/Pauza - tylko krótkie naciśnięcie
    if (btnStart.pendingShort) {
        btnStart.pendingShort = false;
        return EVT_START_SHORT;
    }

    // Stop
    if (btnStop.pendingLong) {
        btnStop.pendingLong = false;
        return EVT_STOP_LONG;
    }
    if (btnStop.pendingShort) {
        btnStop.pendingShort = false;
        return EVT_STOP_SHORT;
    }

    // Selektor
    if (btnSelect.pendingLong) {
        btnSelect.pendingLong = false;
        return EVT_SELECT_LONG;
    }
    if (btnSelect.pendingShort) {
        btnSelect.pendingShort = false;
        return EVT_SELECT_SHORT;
    }

    // Przycisk enkodera
    if (btnEnc.pendingShort) {
        btnEnc.pendingShort = false;
        return EVT_ENC_SHORT;
    }

    // Obroty enkodera
    noInterrupts();
    int delta = encoderDelta;
    encoderDelta = 0;
    interrupts();

    if (delta > 0) return EVT_ENC_CW;
    if (delta < 0) return EVT_ENC_CCW;

    return EVT_NONE;
}
