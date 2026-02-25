#pragma once
// ============================================================
// TrassarV3 - Sterowanie przekaźnikami pistoletów
// Thread-safe: portMUX spinlock chroni gunStates[]
// ============================================================

#include "config.h"
#include <soc/gpio_struct.h>

class GunController {
public:
    void begin();
    void setGun(GunID gun, bool on);
    void allOff();
    void allOn();
    bool isOn(GunID gun) const;
    bool getState(int index) const;

    // Awaryjne wylaczenie — bezposredni zapis do rejestrow GPIO
    // Bezpieczne z ISR i panic handlera (bez mutexa, bez digitalRead)
    static void IRAM_ATTR forceAllOffISR();

private:
    bool gunStates[NUM_GUNS] = {};
    mutable portMUX_TYPE gunMux = portMUX_INITIALIZER_UNLOCKED;
};

extern GunController guns;
