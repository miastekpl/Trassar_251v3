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
//
// Auto-detekcja: przy starcie sprawdza stabilnosc ADC.
// Jesli piny floating (duza wariancja) — osie wylaczone.
// Jesli stabilne — kalibruje centrum do odczytanej pozycji.
// ============================================================

#include "joystick.h"

JoystickHandler joystick;

void JoystickHandler::begin() {
    pinMode(PIN_JOY_SW, INPUT_PULLUP);
    analogReadResolution(12);  // 12-bit (0-4095)

    // --- Auto-detekcja i kalibracja joysticka ---
    // Zbierz 16 probek z opoznieniem, sprawdz stabilnosc
    const int N = 16;
    int samples_x[N], samples_y[N];

    for (int i = 0; i < N; i++) {
        samples_x[i] = analogRead(PIN_JOY_VRX);
        samples_y[i] = analogRead(PIN_JOY_VRY);
        delay(5);
    }

    // Oblicz min/max/srednia
    int sumX = 0, sumY = 0;
    int minX = 4095, maxX = 0, minY = 4095, maxY = 0;
    for (int i = 0; i < N; i++) {
        sumX += samples_x[i];
        sumY += samples_y[i];
        if (samples_x[i] < minX) minX = samples_x[i];
        if (samples_x[i] > maxX) maxX = samples_x[i];
        if (samples_y[i] < minY) minY = samples_y[i];
        if (samples_y[i] > maxY) maxY = samples_y[i];
    }

    int rangeX = maxX - minX;
    int rangeY = maxY - minY;
    int avgX = sumX / N;
    int avgY = sumY / N;

    // Jesli wariancja jest duza (>500) — piny floating, brak joysticka
    if (rangeX > 500 || rangeY > 500) {
        axesEnabled = false;
        Serial.printf("[JOY] Joystick nie wykryty (floating ADC: rangeX=%d rangeY=%d) — osie wylaczone\n",
                      rangeX, rangeY);
    }
    // Jesli odczyt stabilny ale daleko od 0-4095 (sensowny zakres) — joystick OK
    else if (avgX > 100 && avgX < 3995 && avgY > 100 && avgY < 3995) {
        axesEnabled = true;
        centerX = avgX;
        centerY = avgY;
        Serial.printf("[JOY] Joystick OK, centrum: X=%d Y=%d (range: X=%d Y=%d)\n",
                      centerX, centerY, rangeX, rangeY);
    }
    // Odczyt stabilny ale przy skrajnej wartosci (0 lub 4095) — prawdopodobnie pull-down/pull-up bez joysticka
    else {
        axesEnabled = false;
        Serial.printf("[JOY] Joystick nie wykryty (skrajne ADC: X=%d Y=%d) — osie wylaczone\n",
                      avgX, avgY);
    }
}

JoystickHandler::JoyDir JoystickHandler::readDirection() {
    // Srednia z 4 probek — redukcja szumu ADC
    int vrx = 0, vry = 0;
    for (int i = 0; i < 4; i++) {
        vrx += analogRead(PIN_JOY_VRX);
        vry += analogRead(PIN_JOY_VRY);
    }
    vrx /= 4;
    vry /= 4;

    // Uzyj skalibrowanego centrum zamiast stalej 2048
    int dx = abs(vrx - centerX);
    int dy = abs(vry - centerY);

    // Priorytet dla osi z wieksza wychyleniem
    if (dx > dy) {
        if (vrx > centerX + JOY_DEAD_ZONE) return JOY_RIGHT;
        if (vrx < centerX - JOY_DEAD_ZONE) return JOY_LEFT;
    } else {
        if (vry < centerY - JOY_DEAD_ZONE) return JOY_UP;
        if (vry > centerY + JOY_DEAD_ZONE) return JOY_DOWN;
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

    // --- Przycisk SW (cyfrowy, aktywny LOW) --- zawsze aktywny
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

    // --- Osie analogowe --- tylko jesli joystick wykryty
    if (!axesEnabled) return;

    JoyDir dir = readDirection();

    if (dir != currentDir) {
        // Anti-bounce: ignoruj zmiany kierunku szybsze niz 200ms
        if (now - dirStartMs < 200) return;

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
