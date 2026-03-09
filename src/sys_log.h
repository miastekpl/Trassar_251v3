#pragma once
// ============================================================
// TrassarV3 - System logowania z poziomami
// Zamiast golego Serial.print — makra z filtracja poziomu
//
// Poziomy: ERROR (0), WARN (1), INFO (2), DEBUG (3)
// Domyslny: LOG_LEVEL_INFO (2) — pokazuje ERROR + WARN + INFO
// Zmiana: #define LOG_LEVEL LOG_LEVEL_DEBUG przed #include
//
// Uzycie:
//   LOG_INFO("MODUL", "Wiadomosc %d", val);
//   LOG_WARN("MODUL", "Ostrzezenie!");
//   LOG_ERROR("MODUL", "Blad krytyczny: %s", err);
// ============================================================

#include <Arduino.h>

// ============ Poziomy logowania ============
#define LOG_LEVEL_NONE    (-1)
#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARN    1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3

// Domyslny poziom (moze byc nadpisany w platformio.ini build_flags)
#ifndef LOG_LEVEL
  #define LOG_LEVEL  LOG_LEVEL_INFO
#endif

// ============ Makra logowania ============
// Kazde makro sprawdza poziom w compile-time — jesli LOG_LEVEL < wymagany,
// kompilator calkowicie eliminuje kod (zero overhead).

#if LOG_LEVEL >= LOG_LEVEL_ERROR
  #define LOG_ERROR(tag, fmt, ...) \
    Serial.printf("[ERROR][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_ERROR(tag, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
  #define LOG_WARN(tag, fmt, ...) \
    Serial.printf("[WARN][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_WARN(tag, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
  #define LOG_INFO(tag, fmt, ...) \
    Serial.printf("[INFO][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_INFO(tag, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
  #define LOG_DEBUG(tag, fmt, ...) \
    Serial.printf("[DBG][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(tag, fmt, ...) ((void)0)
#endif
