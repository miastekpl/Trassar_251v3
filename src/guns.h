#pragma once
// ============================================================
// TrassarV3 - Sterowanie przekaźnikami pistoletów
// ============================================================

#include "config.h"

class GunController {
public:
    void begin();
    void setGun(GunID gun, bool on);
    void allOff();
    void allOn();
    bool isOn(GunID gun) const;
    bool getState(int index) const;

    // Spinlock chroniący gunStates[] (Core 0 czyta, Core 1 pisze)
    mutable portMUX_TYPE gunMux = portMUX_INITIALIZER_UNLOCKED;

private:
    bool gunStates[NUM_GUNS] = {};
};

extern GunController guns;
