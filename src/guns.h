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
    bool getState(int index) const { return gunStates[index]; }

private:
    bool gunStates[NUM_GUNS] = {};
};

extern GunController guns;
