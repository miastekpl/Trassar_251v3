// ============================================================
// TrassarV3 - Unit testy logiki wzorcow i shouldGunFirePure()
// Uruchamiane na PC: pio test -e native
// Import logiki z src/gun_logic.h (bez kopii kodu)
// ============================================================

#include <unity.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>

// Import czystej logiki strzalu — IMPORT zamiast kopii!
#include "gun_logic.h"

// ============ Testy ============

void test_gun_off_never_fires() {
    GunPatternCfg cfg = {GUN_OFF, 0, 0};
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 100.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, -5.0f));
}

void test_gun_continuous_always_fires() {
    GunPatternCfg cfg = {GUN_CONTINUOUS, 0, 0};
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 50.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 999.9f));
}

void test_gun_dashed_line_phase() {
    // P-1a: linia=4m, przerwa=8m, cykl=12m
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // W fazie linii (0..4m)
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 1.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 3.9f));
}

void test_gun_dashed_gap_phase() {
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // W fazie przerwy (4..12m)
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 4.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 6.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 11.9f));
}

void test_gun_dashed_cycle_wrap() {
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // Drugi cykl (12m = nowy poczatek linii)
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 12.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 13.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 15.9f));

    // Druga przerwa
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 16.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 20.0f));
}

void test_gun_dashed_zero_cycle() {
    // Patologiczny przypadek: oba = 0
    GunPatternCfg cfg = {GUN_DASHED, 0.0f, 0.0f};
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 5.0f));
}

void test_gun_dashed_p1c_pattern() {
    // P-1c Wydzielajaca: 2m linia, 2m przerwa
    GunPatternCfg cfg = {GUN_DASHED, 2.0f, 2.0f};

    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 1.5f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 2.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 3.5f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 4.0f));  // nowy cykl
}

void test_gun_dashed_p1d_pattern() {
    // P-1d Prowadzaca: 1m linia, 1m przerwa
    GunPatternCfg cfg = {GUN_DASHED, 1.0f, 1.0f};

    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.5f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 1.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 1.5f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 2.0f));
}

void test_gun_dashed_large_distance() {
    // Test przy duzym dystansie (150km = 150000m)
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};
    float dist = 150000.0f;  // 150 km

    // pos = fmod(150000, 12) = 0 -> linia
    float pos = fmodf(dist, 12.0f);
    TEST_ASSERT_TRUE(pos < 4.0f);
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, dist));
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
    TEST_ASSERT_TRUE(shouldGunFirePure(guns[GUN_P1], 5.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(guns[GUN_P2], 5.0f));
    TEST_ASSERT_FALSE(shouldGunFirePure(guns[GUN_P3], 5.0f));  // 5m w cyklu 6m = przerwa (4+1 > 4)
    TEST_ASSERT_TRUE(shouldGunFirePure(guns[GUN_P3], 0.0f));   // poczatek = linia
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

    TEST_ASSERT_TRUE(shouldGunFirePure(reversedP1, 0.0f));  // P1 teraz przerywany, poczatek=linia
    TEST_ASSERT_TRUE(shouldGunFirePure(reversedP3, 5.0f));  // P3 teraz ciagly
}

// ============ Testy graniczne (edge cases) ============

void test_gun_dashed_exact_boundary() {
    // Test na dokladnej granicy linia/przerwa
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};

    // Dokladnie na granicy lineLen -> przerwa (pos >= lineLen)
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 4.0f));

    // Epsilon przed granica -> jeszcze linia
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 3.999f));
}

void test_gun_dashed_negative_distance() {
    // Ujemny dystans (edge case - nie powinien wystapic, ale bezpieczenstwo)
    GunPatternCfg cfg = {GUN_DASHED, 4.0f, 8.0f};
    // fmodf z ujemna wartoscia: zachowanie zdefiniowane w C
    bool result = shouldGunFirePure(cfg, -1.0f);
    // Nie sprawdzamy konkretnej wartosci, tylko ze nie crashuje
    (void)result;
    TEST_PASS();
}

void test_gun_dashed_very_small_cycle() {
    // Bardzo krotki cykl (np. 0.1m linia + 0.1m przerwa)
    GunPatternCfg cfg = {GUN_DASHED, 0.1f, 0.1f};

    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.0f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.05f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 0.1f));
    TEST_ASSERT_FALSE(shouldGunFirePure(cfg, 0.15f));
    TEST_ASSERT_TRUE(shouldGunFirePure(cfg, 0.2f));
}

void test_all_gun_modes_coverage() {
    // Kazdy tryb dla kazdego pistoletu
    for (int g = 0; g < NUM_GUNS; g++) {
        GunPatternCfg off = {GUN_OFF, 0, 0};
        GunPatternCfg cont = {GUN_CONTINUOUS, 0, 0};
        GunPatternCfg dash = {GUN_DASHED, 2.0f, 3.0f};

        TEST_ASSERT_FALSE(shouldGunFirePure(off, (float)g));
        TEST_ASSERT_TRUE(shouldGunFirePure(cont, (float)g));
        // Dashed at 0 = linia
        TEST_ASSERT_TRUE(shouldGunFirePure(dash, 0.0f));
    }
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
    RUN_TEST(test_gun_dashed_exact_boundary);
    RUN_TEST(test_gun_dashed_negative_distance);
    RUN_TEST(test_gun_dashed_very_small_cycle);
    RUN_TEST(test_all_gun_modes_coverage);

    return UNITY_END();
}
