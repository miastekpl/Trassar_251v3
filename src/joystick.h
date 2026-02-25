#pragma once
// ============================================================
// TrassarV3 - Joystick analogowy KY-023
// Nawigacja menu: gora/dol/lewo/prawo + przycisk
// ============================================================

#include "config.h"
#include "button_handler.h"

class JoystickHandler {
public:
    void begin();
    void update();
    ButtonEvent getEvent();
    bool isEnabled() const { return axesEnabled; }

private:
    enum JoyDir : uint8_t {
        JOY_NONE = 0,
        JOY_UP,
        JOY_DOWN,
        JOY_LEFT,
        JOY_RIGHT
    };

    // Auto-detekcja i kalibracja
    bool axesEnabled = false;   // Osie analogowe wlaczone (joystick wykryty)
    int centerX = 2048;         // Skalibrowane centrum X
    int centerY = 2048;         // Skalibrowane centrum Y

    // Stan osi analogowych
    JoyDir currentDir = JOY_NONE;
    unsigned long dirStartMs = 0;
    unsigned long lastRepeatMs = 0;
    bool firstEventFired = false;
    ButtonEvent pendingEvent = EVT_NONE;

    // Stan przycisku SW
    bool swPressed = false;
    bool swLastReading = true;
    unsigned long swPressStart = 0;
    bool swLongFired = false;
    bool swPendingShort = false;
    bool swPendingLong = false;

    JoyDir readDirection();
    ButtonEvent dirToEvent(JoyDir dir);
    bool dirRepeatable(JoyDir dir);
};

extern JoystickHandler joystick;
