#pragma once
// ============================================================
// TrassarV3 - System logowania z poziomami + master switch
//
// Master switch: DEBUG_LOG (definiowany w platformio.ini)
//   -DDEBUG_LOG  → Serial aktywny (domyslnie w dev)
//   brak flagi   → caly Serial wylaczony (produkcja, zero overhead)
//
// Poziomy: ERROR (0), WARN (1), INFO (2), DEBUG (3)
// Domyslny: LOG_LEVEL_INFO (2) — pokazuje ERROR + WARN + INFO
// Zmiana: -DLOG_LEVEL=LOG_LEVEL_DEBUG w build_flags
//
// Uzycie:
//   DBG_PRINTLN("[INIT] Start...");
//   DBG_PRINTF("[INIT] Wartosc: %d\n", val);
//   LOG_INFO("MODUL", "Wiadomosc %d", val);
//   LOG_WARN("MODUL", "Ostrzezenie!");
//   LOG_ERROR("MODUL", "Blad krytyczny: %s", err);
// ============================================================

#include <Arduino.h>

// ============ Master switch — caly Serial ON/OFF ============
#ifdef DEBUG_LOG
  #define DBG_BEGIN(baud)     Serial.begin(baud)
  #define DBG_PRINT(...)      Serial.print(__VA_ARGS__)
  #define DBG_PRINTLN(...)    Serial.println(__VA_ARGS__)
  #define DBG_PRINTF(...)     Serial.printf(__VA_ARGS__)
#else
  #define DBG_BEGIN(baud)     ((void)0)
  #define DBG_PRINT(...)      ((void)0)
  #define DBG_PRINTLN(...)    ((void)0)
  #define DBG_PRINTF(...)     ((void)0)
#endif

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
// Aktywne tylko gdy DEBUG_LOG jest zdefiniowany ORAZ LOG_LEVEL >= wymagany.
// Kompilator calkowicie eliminuje kod gdy warunek nie jest spelniony.

#if defined(DEBUG_LOG) && LOG_LEVEL >= LOG_LEVEL_ERROR
  #define LOG_ERROR(tag, fmt, ...) \
    Serial.printf("[ERROR][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_ERROR(tag, fmt, ...) ((void)0)
#endif

#if defined(DEBUG_LOG) && LOG_LEVEL >= LOG_LEVEL_WARN
  #define LOG_WARN(tag, fmt, ...) \
    Serial.printf("[WARN][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_WARN(tag, fmt, ...) ((void)0)
#endif

#if defined(DEBUG_LOG) && LOG_LEVEL >= LOG_LEVEL_INFO
  #define LOG_INFO(tag, fmt, ...) \
    Serial.printf("[INFO][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_INFO(tag, fmt, ...) ((void)0)
#endif

#if defined(DEBUG_LOG) && LOG_LEVEL >= LOG_LEVEL_DEBUG
  #define LOG_DEBUG(tag, fmt, ...) \
    Serial.printf("[DBG][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(tag, fmt, ...) ((void)0)
#endif
