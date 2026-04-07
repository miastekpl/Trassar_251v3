#pragma once
// ============================================================
// TrassarV3 - Wspoldzielone stale layoutu i makra czcionek
// Uzywane przez display_manager.cpp, display_screens_main.cpp,
// display_screens_service.cpp
// ============================================================

#include "display_manager.h"
#include "patterns.h"

// Skroty do czcionek GFX (includowane automatycznie przez TFT_eSPI z LOAD_GFXFF=1)
#define FSB24 &FreeSansBold24pt7b
#define FSB18 &FreeSansBold18pt7b
#define FSB12 &FreeSansBold12pt7b
#define FSB9  &FreeSansBold9pt7b
#define FS9   &FreeSans9pt7b
#define FM9   &FreeMono9pt7b

// ============ Stale layoutu wyswietlacza ============

// Ogolne marginesy
#define MARGIN_X            8       // Margines boczny lewy/prawy [px]
#define HINT_X              6       // X paska podpowiedzi

// Naglowek
#define HDR_H               30      // Wysokosc naglowka [px]
#define HDR_TEXT_CY         15      // Y srodka tekstu naglowka

// Layout 3-kolumnowy (HOME / PAINTING) — padding tekstu
#define COL_L_PAD           92      // setTextPadding lewej kolumny
#define COL_R_PAD           100     // setTextPadding prawej kolumny

// Wizualizacja wzorca (srodkowa kolumna)
#define VIZ_X               100     // X poczatek obszaru wizualizacji
#define VIZ_Y               2       // Y poczatek
#define VIZ_W               120     // Szerokosc
#define VIZ_H               146     // Wysokosc (zmniejszona o 20px na licznik dystansu wzorca)

// Pozycje Y elementow — lewa kolumna
#define ROW_PAT_Y           2       // Kod wzorca (FSB24)
#define ROW_NAME_Y          38      // Nazwa wzorca linia 1
#define ROW_NAME2_Y         52      // Nazwa wzorca linia 2 (dwuslowna)
#define ROW_FLAG_Y          66      // Flaga [ODW] / [GAP]
#define ROW_STATUS_Y        82      // Status "Gotowy" / "Malowanie"
#define ROW_MODE_Y          100     // Tryb pracy na ekranie HOME
#define ROW_TIME_Y          104     // Czas sesji (PAINTING)
#define ROW_DIST_Y          122     // Dystans sesji (PAINTING)
#define ROW_MODE2_Y         140     // Tryb pracy na ekranie PAINTING

// Pozycje Y elementow — prawa kolumna
#define ROW_SPEED_Y         2       // Predkosc (FSB24)
#define ROW_UNIT_Y          38      // Etykieta "km/h"
#define ROW_AREA_Y          56      // Powierzchnia (FSB12)
#define ROW_PAINT_Y         80      // Poziom farby (FS9)

// Dystans wzorca (nad pistoletami, zielony FSB24)
#define ROW_PAT_DIST_Y      155     // Y licznika dystansu biezacego wzorca

// Dol ekranu
#define GUN_RECTS_Y         (TFT_SCREEN_H - 49)    // Y prostokatow pistoletow
#define HINT_Y              (TFT_SCREEN_H - 22)     // Y paska podpowiedzi

// Prostokaty pistoletow
#define GUN_H               33      // Wysokosc prostokata
#define GUN_GAP             2       // Odstep miedzy prostokatami

// Wizualizacja wzorca — parametry kolumn
#define VIZ_COL_GAP         16      // Odstep miedzy kolumnami
#define VIZ_LABEL_H         18      // Wysokosc etykiety nad kolumna
#define VIZ_COL_WIDE        36      // Szerokosc kolumny szerokich pistoletow (P4, P6)
#define VIZ_COL_NARROW      20      // Szerokosc kolumny waskich pistoletow

// Menu serwisowe
#define SMENU_ITEM_H        30      // Wysokosc pozycji menu [px]
#define SMENU_START_Y       34      // Y pierwszej pozycji
#define SMENU_INDENT        24      // X wciecie tekstu
#define SMENU_MARKER_X      8       // X wskaznika ">"
#define SMENU_COUNT         11      // Liczba pozycji menu (SERVICE_MENU_ITEMS)
#define SMENU_VISIBLE       6       // Max pozycji widocznych na ekranie

// Miganie elementow UI
#define BLINK_PERIOD_MS         300     // Okres migania overspeed [ms]
#define BLINK_SLOW_MS           500     // Okres wolnego migania (pauza) [ms]

// Ekran splasha
#define SPLASH_TITLE_OFS    (-40)   // Offset Y tytulu od srodka ekranu
#define SPLASH_SUB_OFS      10      // Offset Y podtytulu
#define SPLASH_VER_OFS      40      // Offset Y wersji
#define SPLASH_INIT_OFS     70      // Offset Y "Inicjalizacja..."
