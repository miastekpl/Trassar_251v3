#include "sys_log.h"
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
static const int JOY_THRESH_HI   = JOY_CENTER + JOY_DEAD_ZONE;  // 2548 — prog powrotu do centrum
static const int JOY_THRESH_LO   = JOY_CENTER - JOY_DEAD_ZONE;  // 1548 — prog powrotu do centrum
// Progi z histereza — wymagane do wejscia w nowy kierunek (wyzsze niz powrot)
static const int JOY_ENTER_HI    = JOY_CENTER + JOY_DEAD_ZONE + JOY_HYSTERESIS;  // 2698
static const int JOY_ENTER_LO    = JOY_CENTER - JOY_DEAD_ZONE - JOY_HYSTERESIS;  // 1398

void JoystickHandler::begin() {
    // GPIO 46 jest strap pinem ESP32-S3 (boot mode select).
    // NIE moze byc LOW przy starcie — ryzyko blednego trybu boot.
    // Mitigacja:
    //   1. Opoznienie 200ms — boot w pelni zakonczony zanim ruszamy pin
    //   2. Konfiguracja jako INPUT (floating) — nie wplywamy na stan pinu
    //   3. Dopiero po weryfikacji stanu — wlaczamy pull-up
    delay(200);
    pinMode(PIN_JOY_SW, INPUT);  // Floating — bezpieczne dla strap pinu

    // Sprawdz czy pin nie jest zwarty do GND (np. wcisniety przycisk)
    if (digitalRead(PIN_JOY_SW) == LOW) {
        // Pin jest LOW — prawdopodobnie przycisk wcisniety lub zwarcie.
        // Nie wlaczamy pull-up, logujemy ostrzezenie.
        DBG_PRINTLN("[JOY] UWAGA: GPIO 46 (SW) = LOW przy starcie! Sprawdz joystick.");
        _strapPinError = true;
    } else {
        pinMode(PIN_JOY_SW, INPUT_PULLUP);
        _strapPinError = false;
    }

    // Piny ADC nie wymagaja pinMode dla analogRead
    analogReadResolution(12);  // 12-bit (0-4095)
}

void JoystickHandler::requireCenter() {
    _centerRequired = true;
    pendingEvent = EVT_NONE;
    currentDir = JOY_NONE;
    firstEventFired = false;
}

JoystickHandler::JoyDir JoystickHandler::readDirection() {
    // Multi-sample: srednia z 3 odczytow — redukcja szumu ADC2 (interferencja WiFi)
    int vrx = 0, vry = 0;
    for (int i = 0; i < 3; i++) {
        vrx += analogRead(PIN_JOY_VRX);
        vry += analogRead(PIN_JOY_VRY);
    }
    vrx /= 3;
    vry /= 3;

    int dx = abs(vrx - JOY_CENTER);
    int dy = abs(vry - JOY_CENTER);

    // Histereza: aby WEJSC w kierunek — prog wyzszy (ENTER),
    //            aby UTRZYMAC kierunek — prog nizszy (THRESH).
    //            Eliminuje oscylacje na granicy strefy martwej.
    // Priorytet dla osi z wieksza wychyleniem
    if (dx > dy) {
        int hiThresh = (currentDir == JOY_RIGHT) ? JOY_THRESH_HI : JOY_ENTER_HI;
        int loThresh = (currentDir == JOY_LEFT)  ? JOY_THRESH_LO : JOY_ENTER_LO;
        if (vrx > hiThresh) return JOY_RIGHT;
        if (vrx < loThresh) return JOY_LEFT;
    } else {
        int loThresh = (currentDir == JOY_UP)   ? JOY_THRESH_LO : JOY_ENTER_LO;
        int hiThresh = (currentDir == JOY_DOWN) ? JOY_THRESH_HI : JOY_ENTER_HI;
        if (vry < loThresh) return JOY_UP;
        if (vry > hiThresh) return JOY_DOWN;
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
    // Jesli strap pin error — ignoruj SW (moze byc zwarty)
    bool reading = _strapPinError ? HIGH : digitalRead(PIN_JOY_SW);

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

    // Blokada osi dopoki joystick nie wroci do centrum (po zmianie ekranu)
    if (_centerRequired) {
        if (dir == JOY_NONE) {
            _centerRequired = false;
        }
        currentDir = JOY_NONE;
        firstEventFired = false;
        return;
    }

    if (dir != currentDir) {
        // Zmiana kierunku
        currentDir = dir;
        dirStartMs = now;
        lastRepeatMs = now;
        firstEventFired = false;

        if (dir != JOY_NONE) {
            if (!dirRepeatable(dir) &&
                dir == lastNonRepeatDir &&
                (now - lastNonRepeatFiredMs < NON_REPEAT_LOCKOUT_MS)) {
                // Lockout: ten sam kierunek jednorazowy zbyt szybko (bounce/szum ADC)
                // Nie generuj zdarzenia
            } else {
                pendingEvent = dirToEvent(dir);
                firstEventFired = true;
                if (!dirRepeatable(dir)) {
                    lastNonRepeatDir = dir;
                    lastNonRepeatFiredMs = now;
                }
            }
        } else {
            // Powrot do centrum — resetuj lockout kierunku
            lastNonRepeatDir = JOY_NONE;
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
        _wasAxisEvent = false;
        return EVT_START_LONG;      // Dlugie SW = START long (np. otwarcie SETUP)
    }
    if (swPendingShort) {
        swPendingShort = false;
        _wasAxisEvent = false;
        return EVT_SELECT_LONG;     // Krotkie SW = wejdz/potwierdz
    }

    if (pendingEvent != EVT_NONE) {
        ButtonEvent evt = pendingEvent;
        pendingEvent = EVT_NONE;
        _wasAxisEvent = true;
        return evt;
    }

    _wasAxisEvent = false;
    return EVT_NONE;
}
