// ============================================================
// TrassarV3 - Implementacja joysticka analogowego KY-023
//
// Mapowanie osi na zdarzenia menu:
//   UP    (VRy < prog)  -> EVT_STOP_SHORT    (poprzednia pozycja)
//   DOWN  (VRy > prog)  -> EVT_SELECT_SHORT  (nastepna pozycja)
//   RIGHT (VRx > prog)  -> EVT_SELECT_LONG   (wejdz / zmien wartosc)
//   LEFT  (VRx < prog)  -> EVT_STOP_LONG     (cofnij / powrot)
//   SW krotki            -> EVT_SELECT_LONG   (potwierdz)
//   SW dlugi (1s)        -> EVT_START_LONG    (SETUP z HOME)
//
// Auto-repeat: tylko UP i DOWN (nawigacja listy)
// Bez auto-repeat: LEFT i RIGHT (akcje jednorazowe)
// ============================================================

#include "joystick.h"

JoystickHandler joystick;

static const int JOY_CENTER      = 2048;   // Srodek 12-bit ADC
static const int JOY_THRESH_HI   = JOY_CENTER + JOY_DEAD_ZONE;  // 2548
static const int JOY_THRESH_LO   = JOY_CENTER - JOY_DEAD_ZONE;  // 1548

void JoystickHandler::begin() {
    pinMode(PIN_JOY_SW, INPUT_PULLUP);
    // Piny ADC nie wymagaja pinMode dla analogRead
    analogReadResolution(12);  // 12-bit (0-4095)
}

JoystickHandler::JoyDir JoystickHandler::readDirection() {
    int vrx = analogRead(PIN_JOY_VRX);
    int vry = analogRead(PIN_JOY_VRY);

    int dx = abs(vrx - JOY_CENTER);
    int dy = abs(vry - JOY_CENTER);

    // Priorytet dla osi z wieksza wychyleniem
    if (dx > dy) {
        if (vrx > JOY_THRESH_HI) return JOY_RIGHT;
        if (vrx < JOY_THRESH_LO) return JOY_LEFT;
    } else {
        if (vry < JOY_THRESH_LO) return JOY_UP;
        if (vry > JOY_THRESH_HI) return JOY_DOWN;
    }

    return JOY_NONE;
}

ButtonEvent JoystickHandler::dirToEvent(JoyDir dir) {
    switch (dir) {
        case JOY_UP:    return EVT_STOP_SHORT;     // Poprzedni element
        case JOY_DOWN:  return EVT_SELECT_SHORT;   // Nastepny element
        case JOY_RIGHT: return EVT_SELECT_LONG;    // Wejdz / zmien
        case JOY_LEFT:  return EVT_STOP_LONG;      // Cofnij / powrot
        default:        return EVT_NONE;
    }
}

bool JoystickHandler::dirRepeatable(JoyDir dir) {
    // Auto-repeat tylko dla gora/dol (nawigacja listy)
    return (dir == JOY_UP || dir == JOY_DOWN);
}

void JoystickHandler::update() {
    unsigned long now = millis();

    // --- Przycisk SW (cyfrowy, aktywny LOW) ---
    bool reading = digitalRead(PIN_JOY_SW);

    if (reading == LOW && !swPressed) {
        swPressed = true;
        swPressStart = now;
        swLongFired = false;
    } else if (reading == LOW && swPressed) {
        if (!swLongFired && (now - swPressStart >= BTN_LONG_PRESS_MS)) {
            swLongFired = true;
            swPendingLong = true;
        }
    } else if (reading == HIGH && swPressed) {
        swPressed = false;
        if (!swLongFired) {
            swPendingShort = true;
        }
    }

    // --- Osie analogowe ---
    JoyDir dir = readDirection();

    if (dir != currentDir) {
        // Zmiana kierunku
        currentDir = dir;
        dirStartMs = now;
        lastRepeatMs = now;
        firstEventFired = false;

        if (dir != JOY_NONE) {
            // Natychmiastowe pierwsze zdarzenie
            pendingEvent = dirToEvent(dir);
            firstEventFired = true;
        }
    } else if (dir != JOY_NONE && firstEventFired && dirRepeatable(dir)) {
        // Trzymanie w tym samym kierunku — auto-repeat (tylko UP/DOWN)
        unsigned long elapsed = now - dirStartMs;
        if (elapsed >= JOY_INITIAL_DELAY_MS) {
            if (now - lastRepeatMs >= JOY_REPEAT_MS) {
                lastRepeatMs = now;
                pendingEvent = dirToEvent(dir);
            }
        }
    }
}

ButtonEvent JoystickHandler::getEvent() {
    // Priorytet: przycisk SW > osie
    if (swPendingLong) {
        swPendingLong = false;
        return EVT_START_LONG;      // Dlugie SW = START long (np. otwarcie SETUP)
    }
    if (swPendingShort) {
        swPendingShort = false;
        return EVT_SELECT_LONG;     // Krotkie SW = wejdz/potwierdz
    }

    if (pendingEvent != EVT_NONE) {
        ButtonEvent evt = pendingEvent;
        pendingEvent = EVT_NONE;
        return evt;
    }

    return EVT_NONE;
}
