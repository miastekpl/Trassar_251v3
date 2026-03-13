#pragma once
// ============================================================
// TrassarV3 - Czysta logika strzalu pistoletu (bez zaleznosci HW)
// Uzywane przez PaintingEngine (ESP32) i unit testy (native)
// ============================================================

#include <cmath>
#include <cstdint>

#ifndef NUM_GUNS
#define NUM_GUNS 6
#endif

// --- Typy pistoletu (powtorzone z config.h dla native) ---
// Na ESP32: config.h definiuje je wczesniej, wiec #ifndef chroni
#ifndef GUN_LOGIC_TYPES_DEFINED
#define GUN_LOGIC_TYPES_DEFINED

#ifndef UNIT_TEST
// Na ESP32 — typy z config.h, ten header wlaczany po config.h
#else
// Na native (unit testy) — minimalne definicje bez Arduino.h
enum GunMode : uint8_t {
    GUN_OFF = 0,
    GUN_CONTINUOUS,
    GUN_DASHED
};

enum GunID : uint8_t {
    GUN_P1 = 0, GUN_P2, GUN_P3, GUN_P4, GUN_P5, GUN_P6
};

struct GunPatternCfg {
    GunMode mode;
    float lineLen;
    float gapLen;
};
#endif // UNIT_TEST

#endif // GUN_LOGIC_TYPES_DEFINED

// ============================================================
// Czysta funkcja decyzji strzalu — zero zaleznosci sprzetowych
// cfg:                 konfiguracja pistoletu (tryb + linia + przerwa)
// distFromPatternStart: dystans od poczatku wzorca [m]
// return:              true = pistolet powinien strzelac
// ============================================================
inline bool shouldGunFirePure(GunPatternCfg cfg, float distFromPatternStart) {
    switch (cfg.mode) {
        case GUN_OFF:
            return false;
        case GUN_CONTINUOUS:
            return true;
        case GUN_DASHED: {
            float cycle = cfg.lineLen + cfg.gapLen;
            if (cycle <= 0) return false;
            float pos = fmodf(distFromPatternStart, cycle);
            return (pos < cfg.lineLen);
        }
    }
    return false;
}
