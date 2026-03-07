// ============================================================
// TrassarV3 - Unit testy logiki wzorcow i shouldGunFire()
// Uruchamiane na PC: pio test -e native
// ============================================================

#include <unity.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>

// ============ Minimalne definicje typow (bez Arduino.h) ============

#define NUM_GUNS 6

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

// ============ Czysta funkcja shouldGunFire (kopia logiki) ============
// Identyczna z PaintingEngine::shouldGunFire() ale standalone

static bool shouldGunFire(GunPatternCfg cfg, float distFromPatternStart) {
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

// ============ Testy ============

void test_gun_off_never_fires() {
    GunPatternCfg cfg = {GUN_OFF, 0, 0};
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 0.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 100.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, -5.0f));
}

void test_gun_continuous_always_fires() {
    GunPatternCfg cfg = {GUN_CONTINUOUS, 0, 0};
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 50.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 999.9f));
}

void test_gun_dashed_line_phase() {
    // P-1a: linia=4m, przerwa=8m, cykl=12m
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // W fazie linii (0..4m)
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 1.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 3.9f));
}

void test_gun_dashed_gap_phase() {
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // W fazie przerwy (4..12m)
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 4.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 6.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 11.9f));
}

void test_gun_dashed_cycle_wrap() {
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // Drugi cykl (12m = nowy poczatek linii)
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 12.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 13.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 15.9f));

    // Druga przerwa
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 16.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 20.0f));
}

void test_gun_dashed_zero_cycle() {
    // Patologiczny przypadek: oba = 0
    GunPatternCfg cfg = {GUN_DASHED, 0.0f, 0.0f};
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 0.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 5.0f));
}

void test_gun_dashed_p1c_pattern() {
    // P-1c Wydzielajaca: 2m linia, 2m przerwa
    GunPatternCfg cfg = {GUN_DASHED, 2.0f, 2.0f};

    TEST_ASSERT_TRUE(shouldGunFire(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 1.5f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 2.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 3.5f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 4.0f));  // nowy cykl
}

void test_gun_dashed_p1d_pattern() {
    // P-1d Prowadzaca: 1m linia, 1m przerwa
    GunPatternCfg cfg = {GUN_DASHED, 1.0f, 1.0f};

    TEST_ASSERT_TRUE(shouldGunFire(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 0.5f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 1.0f));
    TEST_ASSERT_FALSE(shouldGunFire(cfg, 1.5f));
    TEST_ASSERT_TRUE(shouldGunFire(cfg, 2.0f));
}

void test_gun_dashed_large_distance() {
    // Test przy duzym dystansie (150km = 150000m)
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};
    float dist = 150000.0f;  // 150 km

    // pos = fmod(150000, 12) = 0 -> linia
    float pos = fmodf(dist, 12.0f);
    TEST_ASSERT_TRUE(pos < 4.0f);
    TEST_ASSERT_TRUE(shouldGunFire(cfg, dist));
}

// ============ Testy wzorcow P-3 (odwracalne) ============

void test_p3a_pattern_normal() {
    // P-3a: P1=ciagla, P3=przerywana(4,2)
    GunPatternCfg guns[NUM_GUNS] = {
        {GUN_CONTINUOUS, 0, 0},  // P1
        {GUN_OFF, 0, 0},         // P2
        {GUN_DASHED, 4.0f, 2.0f},// P3
        {GUN_OFF, 0, 0},         // P4
        {GUN_OFF, 0, 0},         // P5
        {GUN_OFF, 0, 0}          // P6
    };

    // Normalny: P1 ciagly, P3 przerywany
    TEST_ASSERT_TRUE(shouldGunFire(guns[GUN_P1], 5.0f));
    TEST_ASSERT_FALSE(shouldGunFire(guns[GUN_P2], 5.0f));
    TEST_ASSERT_FALSE(shouldGunFire(guns[GUN_P3], 5.0f));  // 5m w cyklu 6m = przerwa (4+1 > 4)
    TEST_ASSERT_TRUE(shouldGunFire(guns[GUN_P3], 0.0f));   // poczatek = linia
}

void test_p3a_pattern_reversed() {
    // Odwrocony P-3a: swap P1<->P3
    GunPatternCfg guns[NUM_GUNS] = {
        {GUN_CONTINUOUS, 0, 0},   // P1 (bedzie P3 po swapie)
        {GUN_OFF, 0, 0},
        {GUN_DASHED, 4.0f, 2.0f},// P3 (bedzie P1 po swapie)
        {GUN_OFF, 0, 0},
        {GUN_OFF, 0, 0},
        {GUN_OFF, 0, 0}
    };

    // Po odwroceniu: P1 dostaje config P3, P3 dostaje config P1
    GunPatternCfg reversedP1 = guns[GUN_P3];  // P1 <- config P3
    GunPatternCfg reversedP3 = guns[GUN_P1];  // P3 <- config P1

    TEST_ASSERT_TRUE(shouldGunFire(reversedP1, 0.0f));  // P1 teraz przerywany, poczatek=linia
    TEST_ASSERT_TRUE(shouldGunFire(reversedP3, 5.0f));  // P3 teraz ciagly
}

// ============ Main ============

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_gun_off_never_fires);
    RUN_TEST(test_gun_continuous_always_fires);
    RUN_TEST(test_gun_dashed_line_phase);
    RUN_TEST(test_gun_dashed_gap_phase);
    RUN_TEST(test_gun_dashed_cycle_wrap);
    RUN_TEST(test_gun_dashed_zero_cycle);
    RUN_TEST(test_gun_dashed_p1c_pattern);
    RUN_TEST(test_gun_dashed_p1d_pattern);
    RUN_TEST(test_gun_dashed_large_distance);
    RUN_TEST(test_p3a_pattern_normal);
    RUN_TEST(test_p3a_pattern_reversed);

    return UNITY_END();
}
