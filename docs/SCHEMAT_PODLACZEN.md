# TrassarV3 - Dokumentacja techniczna i schemat podłączeń v2.52.0

## Spis treści

1. [Specyfikacja sprzętowa](#1-specyfikacja-sprzętowa)
   - 1.5 [Fizyczny pinout ESP32-S3 DevKitC-1](#15-fizyczny-pinout-esp32-s3-n16r8-devkitc-1)
2. [Tabela podłączeń pinów ESP32-S3 N16R8](#2-tabela-podłączeń-pinów-esp32-s3-n16r8)
3. [Kompletna mapa GPIO](#3-kompletna-mapa-gpio)
4. [Mapowanie funkcji na piny](#4-mapowanie-funkcji-na-piny)
5. [Schemat blokowy systemu](#5-schemat-blokowy-systemu)
6. [Schematy podłączeń poszczególnych modułów](#6-schematy-podłączeń-poszczególnych-modułów)
   - 6.7 [MCP23017 — pełny pinout DIP-28 + mapowanie przycisków](#67-podłączenie-ekspandera-mcp23017-15-przycisków-wzorców)
   - 6.8 [ILI9341 — pinout złącza 14-pin + tabela SPI CS](#68-podłączenie-wyświetlacza-ili9341-i-karty-sd-wspólna-magistrala-spi)
   - 6.10 [Diagram magistrali I2C (DS1307 + MCP23017)](#610-diagram-magistrali-i2c-2-urządzenia-na-wspólnej-szynie)
6A. [Kompletna lista połączeń — checklist montażowy (83 przewody)](#6a-kompletna-lista-połączeń--checklist-montażowy)
7. [Zasilanie + bilans energetyczny](#7-zasilanie)
8. [Architektura oprogramowania](#8-architektura-oprogramowania)
9. [Parametry konfiguracyjne](#9-parametry-konfiguracyjne)
10. [Uwagi montażowe](#10-uwagi-montażowe)

---

## 1. Specyfikacja sprzętowa

### 1.1 Mikrokontroler

| Parametr | Wartość |
|----------|---------|
| Moduł | ESP32-S3 N16R8 (DevKitC-1) |
| Procesor | Xtensa LX7 dual-core, do 240 MHz |
| Flash | 16 MB (Quad SPI) |
| PSRAM | 8 MB (Octal SPI) — **GPIO 26–37 zajęte!** |
| WiFi | 802.11 b/g/n, 2.4 GHz |
| Zasilanie | USB-C 5V |
| Framework | Arduino (ESP-IDF) + PlatformIO |

### 1.2 Peryferia — dokładne modele komponentów

| # | Komponent | Model / Part Number | Interfejs | Opis |
|---|-----------|---------------------|-----------|------|
| 1 | Wyświetlacz LCD | **ILI9341 2.8" TFT SPI** (moduł 240×320 z Touch + SD) | SPI (HSPI/SPI3), 27 MHz | Sterownik ILI9341, rozdzielczość 320×240 px, tryb landscape |
| 2 | Karta SD | **MicroSD** (slot zintegrowany w module ILI9341) | SPI (HSPI/SPI3) | FAT32, współdzieli magistralę SPI z TFT |
| 3 | Zegar RTC | **DS1307 AT24C32** (moduł z EEPROM + slot baterii) | I2C (adres 0x68) | Bateria CR2032, dokładność ±2 ppm |
| 4 | Enkoder | **Enkoder obrotowy inkrementalny** (typ KY-040 lub HW-040) | Digital + ISR (CHANGE) | 20 impulsów/obrót, z przyciskiem SW |
| 5 | Przyciski | **BS-33B** monostabilne NO × 3 szt. | Digital (INPUT_PULLUP) | START, STOP, SELEKTOR — montaż panelowy |
| 6 | Moduł przekaźników | **SRD-05VDC-SL-C 6-kanałowy** (moduł z optoisolacją) | Digital (HIGH = ON) | 6× przekaźnik 5V/10A, opto-izolacja, diody flyback |
| 7 | Buzzer | **Buzzer pasywny 5V** (np. TMB12A05 lub odpowiednik) | LEDC PWM (kanał 1) | Pasywny — wymaga sygnału PWM, zakres 100 Hz – 5 kHz |
| 8 | GPS | **GY-NEO6MV2** (chip u-blox NEO-6M + antena ceramiczna) | UART2 (9600 baud) | Antena 25×25 mm, 50 kanałów, NMEA 0183, cold start <35 s |
| 9 | Joystick | **KY-023** (moduł joysticka analogowego 2-osiowego) | ADC2 (GPIO 19/20) + Digital (GPIO 46) | 2 potencjometry 10kΩ + przycisk tact |
| 10 | Bateria RTC | **CR2032** 3V litowa | — | Podtrzymanie zegara DS1307 po odłączeniu zasilania |
| 11 | Ekspander I/O | **MCP23017** (ekspander I2C 16-bit) | I2C (adres 0x20, wspólna magistrala z DS1307) | 15 wejść przyciskowych, wewn. pull-up, DIP-28 |
| 12 | Przyciski wzorców | **Monostabilne NO** × 15 szt. | Digital (via MCP23017 GPA0–GPB6) | Montaż panelowy, po jednym na wzorzec P-1a…P-7d |

### 1.3 Firmware

| Parametr | Wartość |
|----------|---------|
| Wersja | 2.52.0 |
| Platforma | ESP32-S3 (PlatformIO) |
| Biblioteki | TFT_eSPI v2.5.43, ArduinoJson v7.0.4, RTClib v2.1.4, TinyGPSPlus v1.0.3, WebSockets v2.4.1, SD, Wire, WiFi, esp_task_wdt |
| Orientacja ekranu | Landscape (setRotation 1) |
| Anti-flicker | setTextPadding() zamiast clear() na HOME/PAINTING |

### 1.4 Lista materiałów (BOM)

| # | Komponent | Ilość | Uwagi zakupowe |
|---|-----------|-------|----------------|
| 1 | ESP32-S3 N16R8 DevKitC-1 | 1 | Espressif, 16 MB Flash, 8 MB PSRAM, USB-C |
| 2 | Moduł ILI9341 2.8" TFT z SD i Touch | 1 | Moduł 14-pin (SPI), zintegrowany slot MicroSD |
| 3 | Moduł RTC DS1307 AT24C32 | 1 | Z gniazdem na CR2032 |
| 4 | Bateria CR2032 | 1 | Litowa 3V, do modułu DS1307 |
| 5 | Enkoder obrotowy KY-040 | 1 | 5-pin: CLK, DT, SW, VCC, GND |
| 6 | Przycisk BS-33B (NO, monostabilny) | 3 | START, STOP, SELEKTOR |
| 7 | Moduł przekaźników 6-kanałowy 5V | 1 | Z opto-izolacją, wejścia aktywne HIGH |
| 8 | Moduł GPS GY-NEO6MV2 NEO-6M | 1 | Z anteną ceramiczną na kablu |
| 9 | Joystick analogowy KY-023 | 1 | 5-pin: VRx, VRy, SW, +5V, GND |
| 10 | Buzzer pasywny 5V | 1 | 2-pin (+/−), montaż panelowy |
| 11 | Karta MicroSD | 1 | FAT32, min. 1 GB, klasa 4+ |
| 12 | Ekspander MCP23017 DIP-28 | 1 | I2C adres 0x20 (A0=A1=A2=GND), zasilanie 3.3V |
| 13 | Przycisk monostabilny NO (wzorce) | 15 | Montaż panelowy, podłączenie: pin MCP → GND |
| 14 | Przewody połączeniowe Dupont | ~65 | Żeńsko-żeński i żeńsko-męski |
| 15 | Zasilacz USB-C 5V/2A | 1 | Minimum 1.5A przy pełnym obciążeniu |
| 14 | Koło pomiarowe + uchwyt enkodera | 1 | Obwód dopasowany do kalibracji |
| 15 | Zawory elektromagnetyczne pistoletów | 6 | Podłączenie do wyjść NO przekaźników |

---

## 1.5 Fizyczny pinout ESP32-S3 N16R8 DevKitC-1

Poniższy diagram pokazuje fizyczne rozmieszczenie pinów na płytce DevKitC-1 (widok z góry, USB-C na dole). Piny oznaczone **[✓]** są wykorzystywane w projekcie TrassarV3.

```
                        ┌──────────────┐
                        │   ESP32-S3   │
                        │  N16R8       │
                        │  DevKitC-1   │
                        │              │
             Lewy       │  ┌────────┐  │      Prawy
             header     │  │ CHIP   │  │      header
                        │  │ ESP32  │  │
                        │  │  -S3   │  │
                        │  └────────┘  │
                        │              │
   ─────────────────────┤              ├─────────────────────
   Pin# │ Funkcja       │              │ Funkcja       │ Pin#
   ─────┤───────────────┤              ├───────────────┤─────
    1   │ 3V3           │              │ GND           │  1
    2   │ 3V3           │              │ TX (GPIO 43)  │  2
    3   │ RST           │              │ RX (GPIO 44)  │  3
    4   │ GPIO 4  [✓]P6 │              │ GPIO 1  [✓]P3 │  4
    5   │ GPIO 5  [✓]CLK│              │ GPIO 2  [✓]P4 │  5
    6   │ GPIO 6  [✓]DT │              │ GPIO 42 [✓]P2 │  6
    7   │ GPIO 7  [✓]GAP│              │ GPIO 41 [✓]P1 │  7
    8   │ GPIO 15 [✓]T_CS              │ GPIO 40 [✓]SEL│  8
    9   │ GPIO 16 [✓]SDCS              │ GPIO 39 [✓]STP│  9
   10   │ GPIO 17 [✓]SDA│              │ GPIO 38 [✓]STR│ 10
   11   │ GPIO 18 [✓]SCL│              │ GPIO 37 ──PSRAM│ 11
   12   │ GPIO 8  [✓]BUZ│              │ GPIO 36 ──PSRAM│ 12
   13   │ GPIO 3  [✓]P5 │              │ GPIO 35 ──PSRAM│ 13
   14   │ GPIO 46 [✓]JSW│              │ GPIO 0        │ 14
   15   │ GPIO 9  [✓]DC │              │ GPIO 45       │ 15
   16   │ GPIO 10 [✓]CS │              │ GPIO 48 [✓]GTX│ 16
   17   │ GPIO 11 [✓]MOS│              │ GPIO 47 [✓]GRX│ 17
   18   │ GPIO 12 [✓]SCK│              │ GPIO 21 [✓]LED│ 18
   19   │ GPIO 13 [✓]MIS│              │ GPIO 20 [✓]VRy│ 19
   20   │ GPIO 14 [✓]RST│              │ GPIO 19 [✓]VRx│ 20
   21   │ 5V (VBUS)     │              │ GND           │ 21
   22   │ GND           │              │ GND           │ 22
   ─────┤───────────────┤              ├───────────────┤─────
                        │  ┌────────┐  │
                        │  │ USB-C  │  │
                        │  └────────┘  │
                        └──────────────┘

    Legenda pinów [✓] użytych w projekcie:
    ─────────────────────────────────────
    P1–P6   = Przekaźniki pistoletów
    CLK/DT  = Enkoder obrotowy
    GAP     = Przycisk "start od przerwy"
    T_CS    = Touch Chip Select
    SDCS    = SD Card Chip Select
    SDA/SCL = Magistrala I2C (RTC + MCP23017)
    BUZ     = Buzzer pasywny (PWM)
    JSW     = Joystick przycisk (strap!)
    DC/CS   = TFT Data/Command, Chip Select
    MOS/SCK/MIS = SPI MOSI/SCK/MISO
    RST     = TFT Reset
    GTX/GRX = GPS UART2 TX/RX
    LED     = TFT podświetlenie (PWM)
    VRx/VRy = Joystick osie analogowe
    STR/STP/SEL = Przyciski START/STOP/SELECT
    PSRAM   = Zajęte przez Octal PSRAM — NIE UŻYWAĆ!
```

> **WAŻNE:** GPIO 26–37 (w tym 33–37 widoczne na prawym headerze) są zajęte przez Octal PSRAM w wariancie N16R8. Podłączenie czegokolwiek do tych pinów spowoduje crash systemu!

---

## 2. Tabela podłączeń pinów ESP32-S3 N16R8

### 2.1 Wyświetlacz ILI9341 2.8" (SPI — HSPI)

| Pin ILI9341 | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|-------------|-------------|------|----------|------|
| VCC | 3V3 | — | — | Zasilanie 3.3V |
| GND | GND | — | — | Masa |
| CS | GPIO 10 | 10 | OUTPUT | Chip Select wyświetlacza |
| RESET | GPIO 14 | 14 | OUTPUT | Reset wyświetlacza |
| DC/RS | GPIO 9 | 9 | OUTPUT | Data / Command |
| SDI (MOSI) | GPIO 11 | 11 | OUTPUT | SPI Master Out Slave In |
| SCK | GPIO 12 | 12 | OUTPUT | SPI Clock |
| LED | GPIO 21 | 21 | PWM | Podświetlenie (LEDC PWM, kanał 0, 5 kHz) |
| SDO (MISO) | GPIO 13 | 13 | INPUT | SPI Master In Slave Out |
| T_CLK | GPIO 12 | 12 | — | Touch SPI Clock (wspólny z SCK) |
| T_CS | GPIO 15 | 15 | OUTPUT | Touch Chip Select |
| T_DIN | GPIO 11 | 11 | — | Touch MOSI (wspólny z SDI) |
| T_DO | GPIO 13 | 13 | — | Touch MISO (wspólny z SDO) |
| T_IRQ | — | — | — | Nie podłączony (opcjonalny) |

> **Uwaga:** TFT używa portu HSPI (SPI3) — odizolowanego od Octal PSRAM. Częstotliwość SPI: 27 MHz. Podświetlenie sterowane PWM przez LEDC (kanał 0, 5 kHz, rozdzielczość 8 bitów, wartość domyślna 200/255).

### 2.2 Czytnik kart SD (zintegrowany w module wyświetlacza)

| Pin SD | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|--------|-------------|------|----------|------|
| SD_CS | GPIO 16 | 16 | OUTPUT | Chip Select karty SD |
| SD_MOSI | GPIO 11 | 11 | OUTPUT | SPI MOSI (wspólny z TFT) |
| SD_MISO | GPIO 13 | 13 | INPUT | SPI MISO (wspólny z TFT) |
| SD_SCK | GPIO 12 | 12 | OUTPUT | SPI Clock (wspólny z TFT) |

> **Uwaga:** Karta SD współdzieli magistralę SPI (HSPI) z wyświetlaczem. Każde urządzenie ma osobny pin CS — TFT na GPIO 10, SD na GPIO 16. **Przed inicjalizacją TFT** pin SD_CS (GPIO 16) jest ustawiany na HIGH, aby karta SD nie odpowiadała na ruch SPI przeznaczony dla wyświetlacza.

### 2.3 Zegar RTC DS1307 (I2C)

| Pin DS1307 | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|------------|-------------|------|----------|------|
| VCC | 5V | — | — | Zasilanie 5V |
| GND | GND | — | — | Masa |
| SDA | GPIO 17 | 17 | I/O | I2C Data |
| SCL | GPIO 18 | 18 | OUTPUT | I2C Clock |

> **Uwaga:** Moduł DS1307 wymaga zasilania 5V. Linie I2C mają wbudowane rezystory pull-up na module. Bateria CR2032 podtrzymuje czas po odłączeniu zasilania.

### 2.4 Ekspander MCP23017 (I2C — 15 przycisków wzorców)

| Pin MCP23017 | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|-------------|-------------|------|----------|------|
| VDD | 3V3 | — | — | Zasilanie 3.3V |
| VSS | GND | — | — | Masa |
| SDA | GPIO 17 | 17 | I/O | I2C Data (wspólna z DS1307) |
| SCL | GPIO 18 | 18 | OUTPUT | I2C Clock (wspólna z DS1307) |
| A0 | GND | — | — | Bit adresu 0 (LOW → 0x20) |
| A1 | GND | — | — | Bit adresu 1 (LOW → 0x20) |
| A2 | GND | — | — | Bit adresu 2 (LOW → 0x20) |
| RESET | 3V3 | — | — | Reset nieaktywny (HIGH) |
| GPA0–GPA7 | — | — | INPUT (pull-up) | 8 przycisków wzorców (P-1a…P-3a) |
| GPB0–GPB6 | — | — | INPUT (pull-up) | 7 przycisków wzorców (P-3b…P-7d) |
| GPB7 | — | — | — | Nieużywany |

> **Uwaga:** MCP23017 na wspólnej magistrali I2C z DS1307 (SDA=17, SCL=18). Adres I2C: 0x20. Wewnętrzne pull-up aktywowane programowo. Każdy przycisk podłączony: pin MCP → GND. Skanowanie co 20 ms z debounce.

### 2.5 Enkoder obrotowy (pomiar dystansu i prędkości)

| Pin enkodera | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|-------------|-------------|------|----------|------|
| GND | GND | — | — | Masa |
| CLK (A) | GPIO 5 | 5 | INPUT_PULLUP | Sygnał A (przerwanie ISR CHANGE) |
| DT (B) | GPIO 6 | 6 | INPUT_PULLUP | Sygnał B |
| SW | GPIO 7 | 7 | INPUT_PULLUP | Przycisk "Start od przerwy" |
| + (VCC) | 3V3 | — | — | Zasilanie (opcjonalne) |

> **Uwaga:** Piny CLK i DT mają włączone wewnętrzne rezystory pull-up ESP32-S3. Enkoder służy wyłącznie do pomiaru dystansu i prędkości (ISR na CLK/CHANGE). Debouncing ISR: 200 μs (ENC_ISR_DEBOUNCE_US). Obliczanie prędkości: co 250 ms z filtrem wykładniczym (alpha = 0.3).

### 2.6 Joystick analogowy KY-023

| Pin KY-023 | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|------------|-------------|------|----------|------|
| GND | GND | — | — | Masa |
| +5V | 3V3 | — | — | Zasilanie 3.3V |
| VRx | GPIO 19 | 19 | ANALOG (ADC2) | Oś pozioma (lewo/prawo) |
| VRy | GPIO 20 | 20 | ANALOG (ADC2) | Oś pionowa (góra/dół) |
| SW | GPIO 46 | 46 | INPUT_PULLUP | Przycisk wciskany (aktywny LOW) |

> **Uwaga:** GPIO 46 jest pinem strapping (ROM boot select). Z wewnętrznym pull-up jest HIGH podczas startu (normalny boot z Flash). **Nie wciskać joysticka podczas włączania urządzenia** — może spowodować wejście w tryb download.

### 2.7 Przyciski sterujące (BS-33B monostabilne)

| Przycisk | Pin ESP32-S3 | GPIO | Kierunek | Funkcja |
|----------|-------------|------|----------|---------|
| START | GPIO 38 | 38 | INPUT_PULLUP | Start / Pauza / Wznów / Wybór trybu (1 s, HOME) |
| STOP | GPIO 39 | 39 | INPUT_PULLUP | Stop / Menu (1 s) / Cofnij |
| SELEKTOR | GPIO 40 | 40 | INPUT_PULLUP | Odwróć P-3a/P-3b (HOME/PAINTING), nawigacja + wejście w opcję (menu serwis.) |
| GAP (od przerwy) | GPIO 7 | 7 | INPUT_PULLUP | Start od przerwy (HOME) |

> **UWAGA:** GPIO 26–37 są zajęte przez Octal PSRAM modułu N16R8! NIE wolno ich używać!
>
> **Parametry przycisków:** Debounce: 50 ms, Długie naciśnięcie: 1000 ms. Podłączenie: jeden styk do GPIO, drugi do GND. Wewnętrzne pull-up aktywowane programowo.

### 2.8 Przekaźniki pistoletów (6 szt.)

| Pistolet | Pin ESP32-S3 | GPIO | Szerokość | Zastosowanie | Logika |
|----------|-------------|------|-----------|-------------|--------|
| P1 | GPIO 41 | 41 | 12 cm | Oś jezdni — lewy | HIGH = ON |
| P2 | GPIO 42 | 42 | 12 cm | Oś jezdni — środek | HIGH = ON |
| P3 | GPIO 1 | 1 | 12 cm | Oś jezdni — prawy | HIGH = ON |
| P4 | GPIO 2 | 2 | 24 cm | Oś jezdni — szeroki | HIGH = ON |
| P5 | GPIO 3 | 3 | 12 cm | Krawędź — wąska | HIGH = ON |
| P6 | GPIO 4 | 4 | 24 cm | Krawędź — szeroka | HIGH = ON |

> **Podłączenie:** GPIO → IN modułu przekaźnikowego 5V. Wyjścia NO (Normally Open) przekaźników podłączone do zaworów pistoletów natryskowych.

---

## 3. Kompletna mapa GPIO

| GPIO | Funkcja | Typ | Uwagi |
|------|---------|-----|-------|
| **1** | Przekaźnik P3 | OUTPUT | Pistolet oś R, 12 cm |
| **2** | Przekaźnik P4 | OUTPUT | Pistolet oś, 24 cm |
| **3** | Przekaźnik P5 | OUTPUT | Pistolet krawędź, 12 cm |
| **4** | Przekaźnik P6 | OUTPUT | Pistolet krawędź, 24 cm |
| **5** | Enkoder CLK | INPUT_PULLUP | ISR CHANGE, debounce 200 μs |
| **6** | Enkoder DT | INPUT_PULLUP | Sygnał kierunku |
| **7** | Przycisk GAP (SW enkodera) | INPUT_PULLUP | "Start od przerwy" |
| **8** | Buzzer | PWM (LEDC ch1) | Sygnalizacja dźwiękowa (pasywny) |
| **9** | TFT DC | OUTPUT | Data/Command |
| **10** | TFT CS | OUTPUT | Chip Select wyświetlacza |
| **11** | SPI MOSI | OUTPUT | Wspólny TFT + SD |
| **12** | SPI SCK | OUTPUT | Wspólny TFT + SD + Touch |
| **13** | SPI MISO | INPUT | Wspólny TFT + SD + Touch |
| **14** | TFT RST | OUTPUT | Reset wyświetlacza |
| **15** | Touch CS | OUTPUT | Touch Chip Select |
| **16** | SD Card CS | OUTPUT | Chip Select karty SD |
| **17** | RTC SDA | I/O | I2C Data (DS1307) |
| **18** | RTC SCL | OUTPUT | I2C Clock (DS1307) |
| **19** | Joystick VRx | ADC2_CH8 | KY-023 oś pozioma (lewo/prawo) |
| **20** | Joystick VRy | ADC2_CH9 | KY-023 oś pionowa (góra/dół) |
| **21** | TFT LED | PWM | Podświetlenie LEDC kanał 0 |
| **26–37** | **ZAJĘTE** | — | **Flash + Octal PSRAM — nie podłączać!** |
| **38** | Przycisk START | INPUT_PULLUP | Start / Pauza / Wznów |
| **39** | Przycisk STOP | INPUT_PULLUP | Stop / Menu (1 s) |
| **40** | Przycisk SELECT | INPUT_PULLUP | Odwróć (P-3a/P-3b) / Menu nawigacja |
| **41** | Przekaźnik P1 | OUTPUT | Pistolet oś L, 12 cm |
| **42** | Przekaźnik P2 | OUTPUT | Pistolet oś C, 12 cm |
| **46** | Joystick SW | INPUT_PULLUP | KY-023 przycisk (strap pin) |
| **47** | GPS RX (UART2) | INPUT | ESP32 RX ← GPS TX |
| **48** | GPS TX (UART2) | OUTPUT | ESP32 TX → GPS RX |

---

## 4. Mapowanie funkcji na piny

### 4.1 Funkcje przycisków w kontekście ekranów

| Funkcja | Element | GPIO | Ekran | Uwagi |
|---------|---------|------|-------|-------|
| Start malowania | START | 38 | HOME | Krótkie naciśnięcie |
| Ekran przygotowania (SETUP) | START | 38 | HOME | Długie naciśnięcie (1 s) |
| Start od przerwy | GAP | **7** | HOME | Krótkie naciśnięcie |
| Pauza / Wznowienie | START | 38 | PAINTING | Krótkie naciśnięcie (AUTO) |
| Kolejna linia (SEMI) | START | 38 | PAINTING | Krótkie naciśnięcie (SEMI, po kreski) |
| Pistolety ON (RĘCZNY) | START | 38 | PAINTING | Trzymanie (tryb RĘCZNY) |
| Kursor dalej (SETUP) | SELEKTOR | 40 | SETUP | Krótkie naciśnięcie |
| Zmień opcję (SETUP) | SELEKTOR | 40 | SETUP | Długie naciśnięcie (1 s) |
| Maluj z ustawieniami | START | 38 | SETUP | Krótkie naciśnięcie |
| Zatrzymanie | STOP | 39 | PAINTING | Krótkie naciśnięcie |
| Odwrócenie wzorca | SELEKTOR | 40 | HOME / PAINTING | Krótkie naciśnięcie, **tylko P-3a/P-3b** |
| Smart/Instant toggle | SELEKTOR | 40 | SETUP | Zmiana opcji "Przełączanie" |
| Menu serwisowe | STOP | 39 | HOME | Długie naciśnięcie (1 s) |
| Nawigacja → dalej | SELEKTOR | 40 | MENU SERWIS. | Krótkie naciśnięcie |
| Nawigacja → cofnij | STOP | 39 | MENU SERWIS. | Krótkie naciśnięcie |
| Wejście w opcję | SELEKTOR | 40 | MENU SERWIS. | Długie naciśnięcie (1 s) |
| Powrót | STOP | 39 | MENU / EKRANY | Długie naciśnięcie (1 s) |
| Kalibracja start/stop | START | 38 | KALIBRACJA | Krótkie naciśnięcie |
| Pomiar dystansu | START | 38 | POMIAR | Krótkie naciśnięcie |
| Reset pomiaru | STOP | 39 | POMIAR | Krótkie naciśnięcie |
| Czyszczenie dysz | START | 38 | CZYSZCZ. DYSZ | **Trzymaj** — pistolety ON |
| Zmiana wzorca (czyszcz.) | SELEKTOR | 40 | CZYSZCZ. DYSZ | Krótko = dalej, długo = cofnij |
| Pomiar dystansu/prędkości | Enkoder CLK/DT | 5, 6 | Dowolny | ISR, brak funkcji UI |

### 4.2 Zmiana wzorca malowania

Zmiana wzorca jest możliwa przez:
- **15 fizycznych przycisków wzorców** (MCP23017) — dedykowany przycisk per wzorzec predefiniowany (P-1a…P-7d)
- **Panel WWW** — 16 przycisków wzorców (http://192.168.4.1), w tym WŁASNY
- **API REST** — `POST /api/control` z `action=set_pattern&value=0..15` (15 = WŁASNY)

Na fizycznym panelu sterowania (ekran HOME i PAINTING) przycisk SELEKTOR **nie zmienia** wzorca (służy do odwracania P-3a/P-3b). Wzorzec WŁASNY dostępny wyłącznie z panelu WWW/API.

---

## 5. Schemat blokowy systemu

```
                          ┌──────────────────────────────────┐
                          │         ESP32-S3 N16R8            │
                          │     (16MB Flash, 8MB PSRAM)       │
                          │                                   │
    ┌───────────┐  SPI    │  GPIO 10 ← CS  (TFT)             │
    │  ILI9341  │◄────────│  GPIO  9 ← DC                    │
    │  2.8" TFT │ (HSPI)  │  GPIO 14 ← RST                   │
    │  320×240  │         │  GPIO 11 ← MOSI ──────┐          │
    │  landscape│         │  GPIO 13 → MISO ──────┤ Wspólne  │
    │           │         │  GPIO 12 ← SCK  ──────┤ SPI      │
    │           │         │  GPIO 21 ← LED (PWM)  │          │
    │  Touch    │         │  GPIO 15 ← T_CS       │          │
    │           │         │                        │          │
    │  SD Card  │         │  GPIO 16 ← SD_CS ─────┘          │
    └───────────┘         │                                   │
                          │                                   │
    ┌───────────┐  I2C    │  GPIO 17 ↔ SDA ──┐ Wspólna        │
    │  DS1307   │◄────────│  GPIO 18 ← SCL ──┤ magistrala     │
    │  RTC      │         │                   │ I2C            │
    │  CR2032   │         │                   │                │
    └───────────┘         │                   │                │
                          │                   │                │
    ┌───────────┐  I2C    │                   │                │
    │ MCP23017  │◄────────│  SDA ─────────────┘                │
    │ expander  │         │  SCL ─────────────┘                │
    │ (0x20)    │         │                                   │
    │ 15 przycis│         │  GPA0..GPA7 + GPB0..GPB6           │
    │ wzorców   │         │  = 15 przycisków P-1a...P-7d       │
    └───────────┘         │                                   │
                          │                                   │
    ┌───────────┐ Digital │  GPIO  5 → CLK (ISR CHANGE)       │
    │  Enkoder  │─────────│  GPIO  6 → DT                     │
    │  obrotowy │         │  GPIO  7 → SW (przycisk GAP)      │
    │  (koło    │         │                                   │
    │  pomiar.) │         │                                   │
    └───────────┘         │                                   │
                          │                                   │
    ┌───────────┐         │  --- PRZYCISKI (pull-up) ---      │
    │  BS-33B   │         │                                   │
    │ przyciski │         │                                   │
    │           │         │                                   │
    │ [START]───│── GND ──│── GPIO 38                         │
    │ [STOP]────│── GND ──│── GPIO 39                         │
    │ [SELECT]──│── GND ──│── GPIO 40                         │
    └───────────┘         │                                   │
                          │  --- JOYSTICK ---                │
    ┌───────────┐         │                                   │
    │  KY-023   │         │                                   │
    │ joystick  │         │                                   │
    │           │         │                                   │
    │  VRx ─────├─────────│── GPIO 19  (ADC2)                │
    │  VRy ─────├─────────│── GPIO 20  (ADC2)                │
    │  SW  ─────├─────────│── GPIO 46  (INPUT_PULLUP)        │
    └───────────┘         │                                   │
                          │  --- BUZZER ---                   │
    ┌───────────┐         │                                   │
    │  Buzzer   │         │                                   │
    │  pasywny  ├─────────│── GPIO  8  (PWM LEDC ch1)        │
    │           ├── GND ──│── GND                             │
    └───────────┘         │                                   │
                          │  --- PRZEKAŹNIKI ---              │
    ┌───────────┐         │                                   │
    │ Moduł     │         │                                   │
    │ 6-kanał.  │         │                                   │
    │ przekaźn. │         │                                   │
    │           │         │                                   │
    │ P1(12cm)◄─│─────────│── GPIO 41                         │
    │ P2(12cm)◄─│─────────│── GPIO 42                         │
    │ P3(12cm)◄─│─────────│── GPIO  1                         │
    │ P4(24cm)◄─│─────────│── GPIO  2                         │
    │ P5(12cm)◄─│─────────│── GPIO  3                         │
    │ P6(24cm)◄─│─────────│── GPIO  4                         │
    └───────────┘         │                                   │
                          │                                   │
    ┌───────────┐  UART2  │  GPIO 47 → RX (← GPS TX)          │
    │  GPS      │─────────│  GPIO 48 ← TX (→ GPS RX)          │
    │ NEO-6M    │         │  9600 baud                         │
    │ + antena  │         │                                   │
    └───────────┘         │                                   │
                          │  WiFi AP: TrassarV3 (12345678)    │
                          │  HTTP: http://192.168.4.1:80      │
                          │  Max klientów: 4                  │
                          └──────────────────────────────────┘
```

---

## 6. Schematy podłączeń poszczególnych modułów

### 6.1 Podłączenie przycisków BS-33B

```
    3.3V (wewnętrzny pull-up ESP32-S3)
     │
     ├───────── GPIO 38  [START/PAUZA]
     │            │
     │      ┌─────┴─────┐
     │      │  BS-33B   │
     │      │  przycisk │
     │      └─────┬─────┘
     │            │
     └──── GND ───┘

    Identycznie dla:
     GPIO 39  [STOP]
     GPIO 40  [SELEKTOR]
```

### 6.2 Podłączenie enkodera obrotowego

```
           3V3 (opcjonalnie)
            │
    ┌───────┤
    │  VCC  │
    │       │
    │  CLK  ├──── GPIO 5  (INPUT_PULLUP, ISR CHANGE, debounce 200μs)
    │       │
    │  DT   ├──── GPIO 6  (INPUT_PULLUP)
    │       │
    │  SW   ├──── GPIO 7  (INPUT_PULLUP) ← "START OD PRZERWY"
    │       │
    │  GND  ├──── GND
    └───────┘
```

> Pin SW enkodera (GPIO 7) pełni rolę dedykowanego przycisku "Start od przerwy". Obroty enkodera (CLK/DT) służą wyłącznie do pomiaru dystansu i prędkości.

### 6.3 Podłączenie buzzera pasywnego

```
    ESP32-S3               Buzzer pasywny
    ┌──────────┐           ┌───────────┐
    │          │           │           │
    │ GPIO  8  ├───────────┤ +  (sygnał)│
    │          │           │           │
    │    GND   ├───────────┤ -  (masa)  │
    └──────────┘           └───────────┘
```

> **Uwaga:** Buzzer musi być **pasywny** (bez wbudowanego generatora). Sygnał generowany jest przez LEDC PWM (kanał 1, oddzielny od podświetlenia TFT na kanale 0). Częstotliwość tonów: 600 Hz – 3 kHz.

### 6.4 Podłączenie przekaźników

```
    ESP32-S3                       Moduł przekaźnikowy 5V
    ┌──────────┐                   ┌────────────────────────┐
    │          │                   │                        │
    │ GPIO 41  ├───────────────────┤ IN1  [P1 — oś L 12cm] │──── Zawór P1
    │ GPIO 42  ├───────────────────┤ IN2  [P2 — oś C 12cm] │──── Zawór P2
    │ GPIO  1  ├───────────────────┤ IN3  [P3 — oś R 12cm] │──── Zawór P3
    │ GPIO  2  ├───────────────────┤ IN4  [P4 — oś   24cm] │──── Zawór P4
    │ GPIO  3  ├───────────────────┤ IN5  [P5 — kraw  12cm]│──── Zawór P5
    │ GPIO  4  ├───────────────────┤ IN6  [P6 — kraw  24cm]│──── Zawór P6
    │          │                   │                        │
    │    5V    ├───────────────────┤ VCC                    │
    │    GND   ├───────────────────┤ GND                    │
    └──────────┘                   └────────────────────────┘

    Wyjścia NO (Normally Open) → zawory elektromagnetyczne pistoletów
    Logika: HIGH na GPIO = przekaźnik włączony = pistolet maluje
```

### 6.5 Podłączenie joysticka KY-023

```
    ESP32-S3               Joystick KY-023
    ┌──────────┐           ┌───────────────┐
    │          │           │               │
    │ GPIO 19  ├───────────┤ VRx (oś X)    │
    │ (ADC2)   │           │               │
    │          │           │               │
    │ GPIO 20  ├───────────┤ VRy (oś Y)    │
    │ (ADC2)   │           │               │
    │          │           │               │
    │ GPIO 46  ├───────────┤ SW (przycisk)  │
    │ (pullup) │           │               │
    │          │           │               │
    │    3V3   ├───────────┤ +5V           │
    │    GND   ├───────────┤ GND           │
    └──────────┘           └───────────────┘
```

> **Uwaga:** KY-023 zasilany z 3.3V (zakres ADC 0–3.3V). Centrum joysticka = ~1.65V (ADC ~2048). GPIO 46 jest pinem strapping — z pullup HIGH podczas bootu (poprawne). **Nie wciskać SW podczas włączania ESP32!**

### 6.6 Podłączenie modułu GPS GY-NEO6MV2

```
    ESP32-S3               Moduł GPS GY-NEO6MV2
    ┌──────────┐           ┌──────────────────┐
    │          │           │                  │
    │ GPIO 47  ├───────────┤ TX  (dane NMEA)  │
    │ (UART2 RX)           │                  │
    │          │           │                  │
    │ GPIO 48  ├───────────┤ RX               │
    │ (UART2 TX)           │                  │
    │          │           │                  │
    │    3V3   ├───────────┤ VCC              │
    │    GND   ├───────────┤ GND              │
    └──────────┘           │   [Antena GPS]   │
                           └──────────────────┘
```

> **Uwaga:** Moduł NEO-6M komunikuje się na 9600 baud (domyślnie). Antena ceramiczna musi mieć widoczność nieba. Pin TX modułu GPS podłączamy do GPIO 47 (UART2 RX), pin RX do GPIO 48 (UART2 TX).

### 6.7 Podłączenie ekspandera MCP23017 (15 przycisków wzorców)

#### 6.7.1 Pinout MCP23017 DIP-28 (widok z góry)

```
                       MCP23017 DIP-28
                      ┌────────U────────┐
  [P-3b] ── GND ── 1 │ GPB0       GPA7 │ 28 ── [P-3a] ── GND
  [P-4 ] ── GND ── 2 │ GPB1       GPA6 │ 27 ── [P-2b] ── GND
  [P-6 ] ── GND ── 3 │ GPB2       GPA5 │ 26 ── [P-2a] ── GND
  [P-7a] ── GND ── 4 │ GPB3       GPA4 │ 25 ── [P-1e] ── GND
  [P-7b] ── GND ── 5 │ GPB4       GPA3 │ 24 ── [P-1d] ── GND
  [P-7c] ── GND ── 6 │ GPB5       GPA2 │ 23 ── [P-1c] ── GND
  [P-7d] ── GND ── 7 │ GPB6       GPA1 │ 22 ── [P-1b] ── GND
     (n/c)          8 │ GPB7       GPA0 │ 21 ── [P-1a] ── GND
        3V3  ────── 9 │ VDD        INTA │ 20 ── (n/c)
        GND  ─────10 │ VSS        INTB │ 19 ── (n/c)
     (n/c)         11 │ NC       RESETN │ 18 ── 3V3 (pull HIGH)
  GPIO 18 (SCL) ──12 │ SCL          A2 │ 17 ── GND (adres=0)
  GPIO 17 (SDA) ──13 │ SDA          A1 │ 16 ── GND (adres=0)
     (n/c)         14 │ NC           A0 │ 15 ── GND (adres=0)
                      └─────────────────┘

    n/c = nie podłączony
    U   = wcięcie (notch) na obudowie DIP — orientacja układu
```

#### 6.7.2 Schemat połączeń ESP32-S3 ↔ MCP23017

```
    ESP32-S3                        MCP23017 (DIP-28)
    ┌──────────┐                    ┌─────────────────────────────────────┐
    │          │                    │                                     │
    │ GPIO 17  ├────────────────────┤ SDA (pin 13)                        │
    │ (I2C SDA)│   ┌──(wspólna      │                                     │
    │          │   │  magistrala    │                                     │
    │ GPIO 18  ├───┘────────────────┤ SCL (pin 12)                        │
    │ (I2C SCL)│     z DS1307)      │                                     │
    │          │                    │ A0 (pin 15) ── GND ┐                │
    │          │                    │ A1 (pin 16) ── GND ├ adres = 0x20   │
    │          │                    │ A2 (pin 17) ── GND ┘                │
    │          │                    │ RESETN (pin 18) ── 3V3 (pull HIGH)  │
    │          │                    │                                     │
    │    3V3   ├────────────────────┤ VDD (pin 9)  = zasilanie 3.3V       │
    │    GND   ├────────────────────┤ VSS (pin 10) = masa                 │
    │          │                    │                                     │
    └──────────┘                    │ INTA (pin 20) ── (nie podłączony)   │
                                    │ INTB (pin 19) ── (nie podłączony)   │
                                    │                                     │
                                    │  --- PORT A (8 przycisków) ---       │
                                    │ GPA0 (pin 21) ── [P-1a] ── GND      │
                                    │ GPA1 (pin 22) ── [P-1b] ── GND      │
                                    │ GPA2 (pin 23) ── [P-1c] ── GND      │
                                    │ GPA3 (pin 24) ── [P-1d] ── GND      │
                                    │ GPA4 (pin 25) ── [P-1e] ── GND      │
                                    │ GPA5 (pin 26) ── [P-2a] ── GND      │
                                    │ GPA6 (pin 27) ── [P-2b] ── GND      │
                                    │ GPA7 (pin 28) ── [P-3a] ── GND      │
                                    │                                     │
                                    │  --- PORT B (7 przycisków) ---       │
                                    │ GPB0 (pin 1)  ── [P-3b] ── GND      │
                                    │ GPB1 (pin 2)  ── [P-4 ] ── GND      │
                                    │ GPB2 (pin 3)  ── [P-6 ] ── GND      │
                                    │ GPB3 (pin 4)  ── [P-7a] ── GND      │
                                    │ GPB4 (pin 5)  ── [P-7b] ── GND      │
                                    │ GPB5 (pin 6)  ── [P-7c] ── GND      │
                                    │ GPB6 (pin 7)  ── [P-7d] ── GND      │
                                    │ GPB7 (pin 8)  ── (nieużywany)        │
                                    └─────────────────────────────────────┘
```

#### 6.7.3 Mapowanie przycisk → pin MCP → bit → wzorzec

| # | Przycisk | Port MCP | Pin DIP | Bit w rejestrze | Maska | PatternID |
|---|----------|----------|---------|-----------------|-------|-----------|
| 0 | P-1a | GPA0 | 21 | bit 0 | 0x0001 | PAT_P1A |
| 1 | P-1b | GPA1 | 22 | bit 1 | 0x0002 | PAT_P1B |
| 2 | P-1c | GPA2 | 23 | bit 2 | 0x0004 | PAT_P1C |
| 3 | P-1d | GPA3 | 24 | bit 3 | 0x0008 | PAT_P1D |
| 4 | P-1e | GPA4 | 25 | bit 4 | 0x0010 | PAT_P1E |
| 5 | P-2a | GPA5 | 26 | bit 5 | 0x0020 | PAT_P2A |
| 6 | P-2b | GPA6 | 27 | bit 6 | 0x0040 | PAT_P2B |
| 7 | P-3a | GPA7 | 28 | bit 7 | 0x0080 | PAT_P3A |
| 8 | P-3b | GPB0 | 1 | bit 8 | 0x0100 | PAT_P3B |
| 9 | P-4 | GPB1 | 2 | bit 9 | 0x0200 | PAT_P4 |
| 10 | P-6 | GPB2 | 3 | bit 10 | 0x0400 | PAT_P6 |
| 11 | P-7a | GPB3 | 4 | bit 11 | 0x0800 | PAT_P7A |
| 12 | P-7b | GPB4 | 5 | bit 12 | 0x1000 | PAT_P7B |
| 13 | P-7c | GPB5 | 6 | bit 13 | 0x2000 | PAT_P7C |
| 14 | P-7d | GPB6 | 7 | bit 14 | 0x4000 | PAT_P7D |

#### 6.7.4 Schemat podłączenia pojedynczego przycisku wzorca

```
    MCP23017                      Przycisk monostabilny NO
    ┌─────────┐                   ┌─────────────┐
    │         │                   │    ┌───┐    │
    │  GPAx/  ├───────────────────┤────┤   ├────┤──── GND
    │  GPBx   │                   │    └───┘    │
    │ (pullup)│                   │  normalnie  │
    └─────────┘                   │  otwarty    │
                                  └─────────────┘

    Stan spoczynkowy: GPAx/GPBx = HIGH (pull-up wewnętrzny MCP)
    Stan wciśnięty:   GPAx/GPBx = LOW  (zwarcie do GND)
    Debounce: programowy, skanowanie co 20 ms
```

> **Uwaga:** MCP23017 dzieli magistralę I2C z DS1307 RTC (SDA=GPIO 17, SCL=GPIO 18). Adres I2C: **0x20** (A0=A1=A2 podłączone do GND). Zasilanie z **3.3V**. Pin RESET podłączony do VCC (brak aktywnego resetu). Wewnętrzne pull-up aktywowane programowo (rejestr GPPU) — **nie potrzeba zewnętrznych rezystorów**. Przerwania (INTA/INTB) nie są używane — skanowanie polling co 20 ms. Maska aktywnych bitów: `0x7FFF` (bity 0–14, GPB7 nieużywany).

### 6.8 Podłączenie wyświetlacza ILI9341 i karty SD (wspólna magistrala SPI)

#### 6.8.1 Fizyczny pinout modułu ILI9341 2.8" (złącze 14-pin)

```
    Moduł ILI9341 2.8" TFT (widok z przodu, złącze na dole)
    ┌─────────────────────────────────────────┐
    │                                         │
    │              ╔═══════════╗               │
    │              ║  Ekran    ║               │
    │              ║  TFT LCD  ║               │
    │              ║  240×320  ║               │
    │              ║  (ILI9341)║               │
    │              ╚═══════════╝               │
    │                                         │
    │   ┌─────────────────┐  Slot MicroSD     │
    │   │ ▓▓▓▓▓▓▓▓▓▓▓▓▓  │  (z tyłu)        │
    │   └─────────────────┘                   │
    │                                         │
    └──┤1 ┤2 ┤3 ┤4 ┤5 ┤6 ┤7 ┤8 ┤9 ┤10┤11┤12┤13┤14┤
       │  │  │  │  │  │  │  │  │  │  │  │  │  │
       VCC GND CS RST DC MOSI SCK LED MISO T_CLK T_CS T_DIN T_DO T_IRQ
```

#### 6.8.2 Tabela połączeń pin-po-pinie

| # złącza | Oznaczenie modułu | → GPIO ESP32 | Kolor sugerowany | Opis / Funkcja |
|----------|-------------------|--------------|------------------|----------------|
| 1 | VCC | 3V3 | **czerwony** | Zasilanie 3.3V |
| 2 | GND | GND | **czarny** | Masa |
| 3 | CS | GPIO 10 | żółty | TFT Chip Select (aktywny LOW) |
| 4 | RST | GPIO 14 | biały | Reset wyświetlacza (aktywny LOW) |
| 5 | DC/RS | GPIO 9 | zielony | Data (HIGH) / Command (LOW) |
| 6 | SDI (MOSI) | GPIO 11 | niebieski | SPI dane — **współdzielony z SD i Touch** |
| 7 | SCK | GPIO 12 | fioletowy | SPI zegar — **współdzielony z SD i Touch** |
| 8 | LED | GPIO 21 | pomarańczowy | Podświetlenie (PWM LEDC ch0, 5 kHz) |
| 9 | SDO (MISO) | GPIO 13 | szary | SPI odczyt — **współdzielony z SD i Touch** |
| 10 | T_CLK | GPIO 12 | — | Touch Clock = wspólny z SCK (pin 7) |
| 11 | T_CS | GPIO 15 | brązowy | Touch Chip Select (nieaktywnie używany) |
| 12 | T_DIN | GPIO 11 | — | Touch MOSI = wspólny z SDI (pin 6) |
| 13 | T_DO | GPIO 13 | — | Touch MISO = wspólny z SDO (pin 9) |
| 14 | T_IRQ | — | — | Touch IRQ — **niepodłączony** |

> **Piny 10, 12, 13** nie wymagają osobnych przewodów — łączą się wewnętrznie z pinami 7, 6, 9 na PCB modułu.

#### 6.8.3 Dodatkowy pin karty SD

| Funkcja | → GPIO ESP32 | Opis |
|---------|--------------|------|
| SD_CS | GPIO 16 | Chip Select karty SD (osobny pin na PCB, z tyłu modułu) |

SD_MOSI, SD_MISO, SD_SCK — współdzielone z TFT (GPIO 11, 13, 12).

#### 6.8.4 Schemat połączeń z ESP32-S3

```
    ESP32-S3                    Moduł ILI9341 2.8" (14-pin)
    ┌──────────┐                ┌──────────────────────────────────┐
    │          │    HSPI/SPI3   │                                  │
    │ GPIO 11  ├────────────────┤ pin 6: SDI/MOSI ─┐              │
    │ GPIO 12  ├────────────────┤ pin 7: SCK  ─────┼── Magistrala │
    │ GPIO 13  ├────────────────┤ pin 9: SDO/MISO ─┘  SPI 27 MHz │
    │          │                │                                  │
    │ GPIO 10  ├────── CS ──────┤ pin 3: CS  (TFT select)         │
    │ GPIO  9  ├────── DC ──────┤ pin 5: DC  (Data/Command)       │
    │ GPIO 14  ├────── RST ─────┤ pin 4: RST (Reset)              │
    │ GPIO 21  ├────── PWM ─────┤ pin 8: LED (podświetlenie)      │
    │          │                │                                  │
    │ GPIO 15  ├────── CS ──────┤ pin 11: T_CS (Touch - nieaktywny)│
    │          │                │                                  │
    │ GPIO 16  ├────── CS ──────┤ SD_CS (z tyłu modułu)           │
    │          │                │                                  │
    │    3V3   ├────────────────┤ pin 1: VCC                      │
    │    GND   ├────────────────┤ pin 2: GND                      │
    └──────────┘                └──────────────────────────────────┘

    Przełączanie urządzeń SPI przez Chip Select:
    ┌──────────────┬──────────┬──────────┬──────────┐
    │ Urządzenie   │ TFT (CS) │ SD (CS)  │ Touch(CS)│
    │              │ GPIO 10  │ GPIO 16  │ GPIO 15  │
    ├──────────────┼──────────┼──────────┼──────────┤
    │ TFT aktywny  │   LOW    │   HIGH   │   HIGH   │
    │ SD aktywna   │   HIGH   │   LOW    │   HIGH   │
    │ Touch aktywny│   HIGH   │   HIGH   │   LOW    │
    │ Wszystko OFF │   HIGH   │   HIGH   │   HIGH   │
    └──────────────┴──────────┴──────────┴──────────┘
```

### 6.9 Podłączenie zegara RTC DS1307

```
    ESP32-S3               Moduł DS1307 AT24C32
    ┌──────────┐           ┌──────────────────────┐
    │          │           │                      │
    │ GPIO 17  ├───────────┤ SDA  ──┐ Wspólna     │
    │ (I2C SDA)│           │        │ magistrala  │
    │ GPIO 18  ├───────────┤ SCL  ──┘ I2C         │
    │ (I2C SCL)│           │   (z MCP23017)       │
    │          │           │                      │
    │    5V    ├───────────┤ VCC  (wymaga 5V!)    │
    │   (VBUS) │           │                      │
    │    GND   ├───────────┤ GND                  │
    │          │           │                      │
    └──────────┘           │  ┌──────┐            │
                           │  │CR2032│ 3V backup  │
                           │  └──────┘            │
                           │  Pull-up 4.7kΩ na    │
                           │  SDA i SCL (wbudowane)│
                           │  Adres I2C: 0x68     │
                           └──────────────────────┘
```

> **WAŻNE:** DS1307 wymaga zasilania **5V** (pin VBUS na ESP32-S3). Moduł AT24C32 ma wbudowane rezystory pull-up 4.7kΩ na liniach SDA/SCL, które wystarczają dla obu urządzeń na magistrali I2C.

### 6.10 Diagram magistrali I2C (2 urządzenia na wspólnej szynie)

```
    ESP32-S3 GPIO 17 (SDA) ───────┬──────────────────┬──── 3.3V
                                   │                  │    (pull-up
                                   │                  │     4.7kΩ
                                   │                  │     na module
    ┌──────────────┐               │                  │     DS1307)
    │  DS1307 RTC  │               │                  │
    │  adres: 0x68 ├── SDA ────────┤                  │
    │  zasilanie 5V├── SCL ────────┤──────────┐       │
    └──────────────┘               │          │       │
                                   │          │       │
    ┌──────────────┐               │          │       │
    │  MCP23017    │               │          │       │
    │  adres: 0x20 ├── SDA ────────┘          │       │
    │ zasilanie 3.3V── SCL ──────────────────┘       │
    └──────────────┘                                  │
                                                      │
    ESP32-S3 GPIO 18 (SCL) ──────────────────────────┘

    Prędkość I2C: domyślna 100 kHz (Wire.begin)
    Adresy na magistrali:
      0x20 = MCP23017 (ekspander I/O, 15 przycisków)
      0x68 = DS1307 (zegar RTC)
    Pull-up: wbudowane 4.7kΩ na module DS1307 (wystarczające)
```

---

## 6A. Kompletna lista połączeń — checklist montażowy

Poniżej lista **wszystkich 44 przewodów** do podłączenia, pogrupowana modułami. Kolumna "Kolor" to sugerowany schemat kolorów przewodów Dupont.

### Magistrala zasilania (wspólna)

| # | Z (ESP32-S3) | Do (moduł) | Kolor | Uwagi |
|---|-------------|-----------|-------|-------|
| 1 | **3V3** (lewy pin 1) | ILI9341 pin 1 (VCC) | czerwony | 3.3V |
| 2 | **3V3** (lewy pin 1) | GPS VCC | czerwony | 3.3V |
| 3 | **3V3** (lewy pin 1) | MCP23017 pin 9 (VDD) | czerwony | 3.3V |
| 4 | **3V3** (lewy pin 1) | MCP23017 pin 18 (RESETN) | czerwony | Pull-HIGH |
| 5 | **3V3** (lewy pin 1) | Joystick KY-023 (+5V) | czerwony | Zasilanie 3.3V (nie 5V!) |
| 6 | **3V3** (lewy pin 1) | Enkoder VCC (opcjonalnie) | czerwony | Opcjonalny |
| 7 | **5V** (lewy pin 21) | DS1307 VCC | czerwony+biały | **5V!** |
| 8 | **5V** (lewy pin 21) | Moduł przekaźnikowy VCC | czerwony+biały | **5V!** |
| 9 | **GND** (prawy pin 1) | ILI9341 pin 2 (GND) | czarny | Masa |
| 10 | **GND** | GPS GND | czarny | Masa |
| 11 | **GND** | DS1307 GND | czarny | Masa |
| 12 | **GND** | MCP23017 pin 10 (VSS) | czarny | Masa |
| 13 | **GND** | MCP23017 pin 15 (A0) | czarny | Adres=0 |
| 14 | **GND** | MCP23017 pin 16 (A1) | czarny | Adres=0 |
| 15 | **GND** | MCP23017 pin 17 (A2) | czarny | Adres=0 |
| 16 | **GND** | Moduł przekaźnikowy GND | czarny | Masa |
| 17 | **GND** | Joystick KY-023 GND | czarny | Masa |
| 18 | **GND** | Enkoder GND | czarny | Masa |
| 19 | **GND** | Buzzer (−) | czarny | Masa |

### Magistrala SPI (wyświetlacz + SD)

| # | GPIO ESP32 | Do (ILI9341) | Kolor | Uwagi |
|---|-----------|-------------|-------|-------|
| 20 | GPIO 11 | pin 6: SDI/MOSI | niebieski | Współdzielony TFT+SD+Touch |
| 21 | GPIO 12 | pin 7: SCK | fioletowy | Współdzielony TFT+SD+Touch |
| 22 | GPIO 13 | pin 9: SDO/MISO | szary | Współdzielony TFT+SD+Touch |
| 23 | GPIO 10 | pin 3: CS | żółty | TFT Chip Select |
| 24 | GPIO 9 | pin 5: DC/RS | zielony | Data/Command |
| 25 | GPIO 14 | pin 4: RST | biały | TFT Reset |
| 26 | GPIO 21 | pin 8: LED | pomarańczowy | Podświetlenie PWM |
| 27 | GPIO 15 | pin 11: T_CS | brązowy | Touch CS (nieużywany aktywnie) |
| 28 | GPIO 16 | SD_CS (z tyłu) | żółty+czarny | SD Chip Select |

### Magistrala I2C (RTC + MCP23017)

| # | GPIO ESP32 | Do (moduł) | Kolor | Uwagi |
|---|-----------|-----------|-------|-------|
| 29 | GPIO 17 | DS1307 SDA | zielony+biały | I2C Data |
| 30 | GPIO 18 | DS1307 SCL | niebieski+biały | I2C Clock |
| 31 | GPIO 17 | MCP23017 pin 13 (SDA) | zielony+biały | Równolegle z DS1307! |
| 32 | GPIO 18 | MCP23017 pin 12 (SCL) | niebieski+biały | Równolegle z DS1307! |

> Piny 29+31 i 30+32 to ta sama linia — użyj splitterów Y lub lutuj na jednym przewodzie.

### Przekaźniki pistoletów (6 przewodów sygnałowych)

| # | GPIO ESP32 | Do (moduł przekaźn.) | Pistolet | Kolor | Szerokość |
|---|-----------|---------------------|----------|-------|-----------|
| 33 | GPIO 41 | IN1 | P1 — oś L | pomarańczowy | 12 cm |
| 34 | GPIO 42 | IN2 | P2 — oś C | pomarańczowy | 12 cm |
| 35 | GPIO 1 | IN3 | P3 — oś R | pomarańczowy | 12 cm |
| 36 | GPIO 2 | IN4 | P4 — oś W | pomarańczowy | 24 cm |
| 37 | GPIO 3 | IN5 | P5 — kraw. | pomarańczowy | 12 cm |
| 38 | GPIO 4 | IN6 | P6 — kraw. | pomarańczowy | 24 cm |

### Pozostałe moduły

| # | GPIO ESP32 | Do (moduł) | Kolor | Uwagi |
|---|-----------|-----------|-------|-------|
| 39 | GPIO 5 | Enkoder CLK | niebieski | ISR CHANGE |
| 40 | GPIO 6 | Enkoder DT | zielony | Kierunek |
| 41 | GPIO 7 | Enkoder SW | żółty | Przycisk GAP |
| 42 | GPIO 8 | Buzzer (+) | pomarańczowy | PWM LEDC ch1 |
| 43 | GPIO 47 | GPS TX→ESP RX | zielony | UART2 RX |
| 44 | GPIO 48 | GPS RX←ESP TX | żółty | UART2 TX |
| 45 | GPIO 19 | Joystick VRx | niebieski | ADC2 oś X |
| 46 | GPIO 20 | Joystick VRy | zielony | ADC2 oś Y |
| 47 | GPIO 46 | Joystick SW | żółty | Strap pin! |

### Przyciski sterujące (3 przewody sygnałowe + 3× GND)

| # | GPIO ESP32 | Do | Kolor | Uwagi |
|---|-----------|---|-------|-------|
| 48 | GPIO 38 | START — styk 1 | czerwony | Pull-up wewnętrzny |
| 49 | GND | START — styk 2 | czarny | Masa |
| 50 | GPIO 39 | STOP — styk 1 | żółty | Pull-up wewnętrzny |
| 51 | GND | STOP — styk 2 | czarny | Masa |
| 52 | GPIO 40 | SELECT — styk 1 | zielony | Pull-up wewnętrzny |
| 53 | GND | SELECT — styk 2 | czarny | Masa |

### Przyciski wzorców MCP23017 (15 przycisków × 2 przewody)

| # | Pin MCP23017 | Przycisk → GND | Wzorzec |
|---|-------------|----------------|---------|
| 54–55 | GPA0 (pin 21) | [P-1a] → GND | Przerywana długa |
| 56–57 | GPA1 (pin 22) | [P-1b] → GND | Przerywana krótka |
| 58–59 | GPA2 (pin 23) | [P-1c] → GND | Wydzielająca |
| 60–61 | GPA3 (pin 24) | [P-1d] → GND | Prowadząca wąska |
| 62–63 | GPA4 (pin 25) | [P-1e] → GND | Prowadząca szeroka |
| 64–65 | GPA5 (pin 26) | [P-2a] → GND | Ciągła wąska |
| 66–67 | GPA6 (pin 27) | [P-2b] → GND | Ciągła szeroka |
| 68–69 | GPA7 (pin 28) | [P-3a] → GND | Przekraczalna długa |
| 70–71 | GPB0 (pin 1) | [P-3b] → GND | Przekraczalna krótka |
| 72–73 | GPB1 (pin 2) | [P-4] → GND | Podwójna ciągła |
| 74–75 | GPB2 (pin 3) | [P-6] → GND | Ostrzegawcza |
| 76–77 | GPB3 (pin 4) | [P-7a] → GND | Krawędziowa przeryw. szer. |
| 78–79 | GPB4 (pin 5) | [P-7b] → GND | Krawędziowa ciągła szer. |
| 80–81 | GPB5 (pin 6) | [P-7c] → GND | Krawędziowa przeryw. wąska |
| 82–83 | GPB6 (pin 7) | [P-7d] → GND | Krawędziowa ciągła wąska |

> Każdy przycisk wymaga 2 przewodów: pin MCP → styk 1 przycisku, GND → styk 2 przycisku. Masę (GND) przycisków wzorców można łączyć łańcuchowo z jednego źródła GND na MCP23017 (pin 10/VSS) lub z ESP32.

**Łączna liczba przewodów: ~83** (w tym ~30 przewodów GND, które można łączyć łańcuchowo)

---

## 7. Zasilanie

### 7.1 Źródła zasilania

| Źródło | Napięcie | Odbiorcy |
|--------|----------|----------|
| USB-C ESP32-S3 | 5V (VBUS) | ESP32-S3, DS1307, moduł przekaźnikowy |
| Regulator ESP32-S3 | 3.3V | ILI9341, enkoder, karta SD, GPS GY-NEO6MV2 |

### 7.2 Schemat zasilania

```
    USB-C 5V
      │
      ├──── ESP32-S3 (5V VBUS)
      │       │
      │       └──── Regulator 3.3V
      │               │
      │               ├──── ILI9341 TFT (VCC)
      │               ├──── Karta SD (VCC)
      │               ├──── Enkoder (VCC, opcjonalnie)
      │               └──── GPS GY-NEO6MV2 (VCC 3.3V)
      │
      ├──── DS1307 RTC (VCC = 5V)
      │
      └──── Moduł przekaźnikowy (VCC = 5V)
```

### 7.3 Bilans energetyczny — szczegółowy pobór prądu

#### Linia 3.3V (z regulatora ESP32-S3, max ~500 mA)

| Moduł | Pobór typowy | Pobór max | Uwagi |
|-------|-------------|----------|-------|
| ESP32-S3 (CPU + WiFi AP) | ~120 mA | ~240 mA | WiFi TX: do 240 mA (peaki) |
| ILI9341 TFT (z podświetleniem) | ~40 mA | ~80 mA | LED PWM 200/255 ≈ 40 mA |
| Karta MicroSD (zapis) | ~30 mA | ~100 mA | Peaki przy zapisie CSV |
| GPS GY-NEO6MV2 | ~35 mA | ~50 mA | Tracking mode ~35 mA |
| MCP23017 | ~1 mA | ~1 mA | Statyczny, skan I2C |
| Enkoder (VCC opcjonalny) | ~5 mA | ~5 mA | Dioda LED enkodera |
| Joystick KY-023 | ~1 mA | ~1 mA | 2× potencjometr 10kΩ |
| **RAZEM linia 3.3V** | **~232 mA** | **~477 mA** | Blisko limitu! |

#### Linia 5V (VBUS USB-C)

| Moduł | Pobór typowy | Pobór max | Uwagi |
|-------|-------------|----------|-------|
| Moduł przekaźnikowy (cewki) | ~70 mA/kanał | ~420 mA | 6 kanałów × 70 mA |
| Moduł przekaźnikowy (optocoupler) | ~10 mA/kanał | ~60 mA | 6 kanałów × 10 mA |
| DS1307 RTC | ~1.5 mA | ~3 mA | Znikomy pobór |
| Regulator 3.3V (obciążenie jw.) | ~232 mA | ~477 mA | Przeliczone z linii 3.3V |
| **RAZEM linia 5V** | **~384 mA** | **~960 mA** | |

#### Podsumowanie zasilania

```
    Scenariusz                                    Pobór 5V (VBUS)
    ─────────────────────────────────────────────────────────────
    Spoczynek (IDLE, WiFi, TFT, GPS)              ~250 mA
    Malowanie 1 pistolet (AUTO, 1 relay)          ~350 mA
    Malowanie 3 pistolety (typowe P-4)            ~500 mA
    Malowanie 6 pistoletów + zapis SD + WiFi      ~960 mA ← MAX
    ─────────────────────────────────────────────────────────────
    Wymagany zasilacz USB-C:  MINIMUM 5V / 1.5A
    Zalecany zasilacz USB-C:  5V / 2A – 3A
```

> **OSTRZEŻENIE:** Standardowy port USB 2.0 dostarcza max 500 mA — **niewystarczające** przy 3+ aktywnych przekaźnikach! Wymagany jest zasilacz USB-C lub USB 3.0 z wyższym limitem prądowym.

### 7.4 Uwagi o zasilaniu

- Wyświetlacz ILI9341 zasilany z pinu 3V3 płytki ESP32-S3
- Moduł DS1307 wymaga 5V — podłączyć do pinu 5V (VBUS)
- Moduły przekaźnikowe wymagają 5V — podłączyć do pinu 5V (VBUS)
- **Ważne:** Przy 6 przekaźnikach aktywnych jednocześnie pobór prądu jest znaczny (~420 mA + ~60 mA opto = ~480 mA). Przy większych obciążeniach rozważ zewnętrzne zasilanie 5V dla modułu przekaźnikowego
- Bateria CR2032 w module DS1307 podtrzymuje czas po odłączeniu zasilania głównego
- **Rozwiązanie dla dużego poboru:** Oddzielny zasilacz 5V dla modułu przekaźnikowego (osobne GND+VCC), masa wspólna z ESP32

---

## 8. Architektura oprogramowania

### 8.1 Moduły systemu

| Moduł | Plik | Opis |
|-------|------|------|
| **main** | main.cpp | Pętla główna, inicjalizacja, timery |
| **config** | config.h | Konfiguracja pinów, stałe, struktury danych |
| **display_manager** | display_manager.cpp/h | Sterowanie wyświetlaczem ILI9341 |
| **menu** | menu.cpp/h | System menu, obsługa zdarzeń przycisków |
| **patterns** | patterns.cpp/h | Definicje 16 wzorców malowania (15 + własny) |
| **painting_engine** | painting_engine.cpp/h | Silnik malowania — 3 tryby sterowania pistoletami |
| **button_handler** | button_handler.cpp/h | Obsługa przycisków z debounce i long-press |
| **encoder_distance** | encoder_distance.cpp/h | Pomiar dystansu i prędkości z enkodera |
| **guns** | guns.cpp/h | Sterowanie 6 przekaźnikami pistoletów |
| **statistics** | statistics.cpp/h | Statystyki sesji (dystans, powierzchnia, czas) |
| **storage** | storage.cpp/h | Pamięć NVS (kalibracja, wzorzec, tryb, wzorzec własny) |
| **rtc_handler** | rtc_handler.cpp/h | Obsługa zegara RTC DS1307 |
| **report_logger** | report_logger.cpp/h | Zapis raportów CSV na kartę SD |
| **buzzer** | buzzer.cpp/h | Sygnalizacja dźwiękowa (LEDC PWM, non-blocking) |
| **gps_handler** | gps_handler.cpp/h | Obsługa GPS NEO-6M (UART2, TinyGPS++) |
| **joystick** | joystick.cpp/h | Joystick analogowy KY-023 (ADC + przycisk, nawigacja menu) |
| **pattern_buttons** | pattern_buttons.cpp/h | 15 przycisków wzorców via MCP23017 I2C (skan, debounce) |
| **web_server** | web_server.cpp/h | WiFi AP + serwer HTTP + API REST + WebSocket |
| **gps_track** | gps_track.cpp/h | Zapis trasy GPS (GPX + GeoJSON, bufor PSRAM) |
| **event_log** | event_log.cpp/h | Log zdarzeń systemowych na kartę SD |
| **nvs_backup** | nvs_backup.cpp/h | Backup/restore NVS na kartę SD (JSON) |
| **paint_consumption** | paint_consumption.cpp/h | Predykcja zużycia farby |
| **session_report** | session_report.cpp/h | Generowanie raportów HTML sesji |
| **temp_sensor** | temp_sensor.cpp/h | Czujnik temperatury DS18B20 (opcjonalny) |
| **hal** | hal.h | Warstwa abstrakcji sprzętowej (testy native) |

### 8.2 Architektura dual-core (v2.6.0)

```
╔══════════════════════════════════╗  ╔══════════════════════════════╗
║         CORE 1 (loop)           ║  ║      CORE 0 (FreeRTOS)      ║
║                                  ║  ║                              ║
║  0. esp_task_wdt_reset()         ║  ║  webTaskFunc() {             ║
║  1. buttons.update()             ║  ║      for(;;) {               ║
║  1b. joystick.update()           ║  ║          server.handleClient()║
║  2. menu.handleEvent()           ║  ║
║  3. encoderDist.update()         ║  ║          vTaskDelay(2ms)     ║
║  4. rtcModule.update()           ║  ║      }                       ║
║  5. paintEngine.update()         ║  ║  }                           ║
║  5b. checkGunKeepAlive()         ║  ║                              ║
║  5c. buzzer.update()             ║  ║  Stack: 12288 B              ║
║  6. display refresh (500ms)      ║  ║  Priorytet: 1                ║
║  7. menu.update() (100ms)        ║  ╚══════════════════════════════╝
║  8. lifetime save (60s)          ║
║  9. diagnostyka (30s)            ║
║  10. anomalia pistoletów (10s)   ║
║  11. cache raportów SD (15s)     ║
║  delay(1)                        ║
╚══════════════════════════════════╝
```

### 8.3 Timery i interwały

| Timer | Interwał | Funkcja |
|-------|----------|---------|
| DISPLAY_REFRESH_MS | 100 ms | Minimalna częstotliwość renderowania ekranu |
| DYNAMIC_UPDATE_MS | 500 ms | Dynamiczne odświeżanie ekranów (HOME, PAINTING, itp.) |
| SPEED_CALC_INTERVAL_MS | 250 ms | Przeliczanie prędkości z impulsów enkodera |
| ENC_ISR_DEBOUNCE_US | 200 μs | Debouncing przerwania enkodera |
| BTN_DEBOUNCE_MS | 50 ms | Debouncing przycisków |
| BTN_LONG_PRESS_MS | 1000 ms | Próg długiego naciśnięcia |
| Auto-refresh WWW | 1000 ms | Odpytywanie /api/status przez JavaScript |
| WDT_TIMEOUT_SEC | 3000 ms | Watchdog timer — auto-reset ESP32 |
| GUN_KEEPALIVE_TIMEOUT_MS | 300 ms | Awaryjne wyłączenie pistoletów |
| LIFETIME_SAVE_MS | 60000 ms | Okresowy zapis statystyk do NVS |
| DIAG_PRINT_MS | 30000 ms | Diagnostyka systemowa (Serial) |
| GUN_ANOMALY_CHECK_MS | 10000 ms | Sprawdzanie anomalii pistoletów |
| REPORT_CACHE_MS | 15000 ms | Odświeżanie cache raportów SD |
| WS_BROADCAST_MS | 500 ms | Broadcast WebSocket do klientów |
| GPX_RECORD_INTERVAL_MS | 5000 ms | Interwał zapisu punktu GPS na trasie |
| MTH_SAVE_INTERVAL_MS | 300000 ms | Zapis motogodzin do NVS |
| NVS_BACKUP_INTERVAL_MS | 1800000 ms | Backup NVS na kartę SD (30 min) |
| AUTO_PAUSE_DELAY_MS | 1500 ms | Opóźnienie auto-pauzy |

### 8.4 Maszyna stanów

```
                  ┌─────────┐
       ┌──────────│  IDLE   │◄──────────┐
       │          └────┬────┘           │
       │               │                │
       │    START      │  GAP           │  STOP
       │               ▼                │
       │          ┌─────────┐           │
       │    ┌─────│PAINTING │──────┐    │
       │    │     └─────────┘      │    │
       │    │    (4 tryby pracy:   │    │
       │    │ AUTO/SEMI/MANUAL/DEMO)│   │
       │    │                      │    │
       │    │ START (pauza/AUTO)   │ STOP
       │    ▼                      │    │
       │ ┌──────┐                  │    ▼
       │ │PAUSED│──────────────┐ ┌──────────┐
       │ └──┬───┘              │ │ SUMMARY  │
       │    │                  │ │ dystans,  │
       │    │ START (wznów)    │ │ czas, GPS │
       │    └──► PAINTING ─────┘ └───┬──┬───┘
       │                  START=kontynuuj│ │
       │                  (→PAINTING)    │ │
       │        STOP=nowy etap (reset)───┘ │
       │        STOP(1s)=HOME──────────────┘
       │
       │    STOP (1s na HOME)       START (1s na HOME)
       ▼                              ▼
  ┌──────────┐                ┌──────────────┐
  │ SERVICE  │                │    SETUP     │
  │  MENU    │                │ Tryb/Smart/  │
  └─────┬────┘                │ Start        │
   → Kalibracja/Pomiar/       └──────┬───────┘
     Raporty/Czyszczenie/       START = maluj
     Reset etapu                 → PAINTING
        │
        ▼
  ┌──────────────┐
  │SESSION RESET │  START=TAK → zeruj liczniki → HOME
  │ (potwierdź)  │  STOP=NIE → powrót do MENU
  └──────────────┘
```

### 8.5 Logika sterowania pistoletami (3 tryby)

```
paintEngine.update():
    1. Oblicz dystans od startu wzorca
    2. Sprawdź prędkość >= 3 km/h (MIN_PAINT_SPEED_KMH)
    3. Zależnie od trybu (g_state.machineMode):

       TRYB AUTO:
         Dla każdego pistoletu (P1–P6):
           a. Pobierz konfigurację z wzorca (z uwzgl. odwrócenia)
           b. GUN_OFF → wyłączony
           c. GUN_CONTINUOUS → włączony (jeśli speedOK)
           d. GUN_DASHED → fmod(dist, kreska+przerwa) < kreska ? ON : OFF
         Inteligentne przełączanie wzorców (Smart Switch)

       TRYB SEMI:
         semiLineDist += deltaDist
         GUN_CONTINUOUS → włączony (jeśli speedOK)
         GUN_DASHED → ON jeśli semiLineDist < lineLen && speedOK
                       Po osiągnięciu lineLen → semiLineComplete=true, buzzer
                       Operator naciska START → semiNextLine() → reset

       TRYB RĘCZNY:
         fire = speedOK && buttons.isStartHeld() && (mode != GUN_OFF)
         Pistolety ON tylko gdy operator trzyma przycisk START

       TRYB DEMO (v2.52.0):
         Logika identyczna jak AUTO, ale:
         guns.setGun(i, false)    // Fizycznie zawsze OFF
         gunStates[i] = wouldFire // Wizualizacja na ekranie i WWW

    4. Zaktualizuj statystyki (dystans, powierzchnia)
    5. Auto-pauza: jeśli prędkość < 0.5 km/h przez 1.5s → automatyczna pauza
    6. Auto-wznowienie: jeśli prędkość >= min → automatyczne wznowienie
```

### 8.6 API REST

| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/` | GET | Strona HTML panelu sterowania |
| `/api/status` | GET | JSON ze stanem systemu (+ anomalia pistoletów) |
| `/api/stats` | GET | Statystyki lifetime + sesja + per-gun |
| `/api/reports` | GET | Lista plików raportów CSV z karty SD |
| `/api/control` | POST | Sterowanie maszyną (action=start\|pause\|stop\|start_from_gap\|set_pattern\|toggle_reverse\|set_mode\|semi_next_line\|save_custom_pattern\|cal_start\|cal_finish\|set_max_speed\|set_min_speed\|set_tank_capacity\|set_paint_rate\|set_auto_resume) |
| `/api/reports/download` | GET | Pobierz plik raportu CSV z karty SD |
| `/api/tracks` | GET | Lista plików tras GPS (GPX/GeoJSON) |
| `/api/tracks/download` | GET | Pobierz plik trasy GPS |
| `/api/html_reports` | GET | Lista raportów HTML sesji |
| `/api/html_reports/download` | GET | Pobierz raport HTML sesji |
| `/api/reports/geojson` | GET | Eksport GeoJSON |

Szczegółowa dokumentacja API → [API_WWW.md](API_WWW.md)

---

## 9. Parametry konfiguracyjne

### 9.1 Parametry zdefiniowane w config.h

| Parametr | Wartość | Opis |
|----------|---------|------|
| FW_VERSION | "2.51.0" | Wersja firmware |
| FW_NAME | "TrassarV3" | Nazwa systemu |
| WIFI_AP_SSID | "TrassarV3" | Nazwa sieci WiFi |
| WIFI_AP_PASS | "12345678" | Hasło WiFi |
| WIFI_AP_CHANNEL | 6 | Kanał WiFi |
| WIFI_AP_MAX_CON | 4 | Max klientów WiFi |
| WEB_SERVER_PORT | 80 | Port serwera HTTP |
| MIN_PAINT_SPEED_KMH | 3.0 | Minimalna prędkość malowania [km/h] |
| DEFAULT_PULSES_PER_METER | 100.0 | Domyślna wartość kalibracji |
| CALIBRATION_DISTANCE_M | 10.0 | Dystans kalibracji [m] |
| TFT_SCREEN_W | 320 | Szerokość ekranu [px] (landscape) |
| TFT_SCREEN_H | 240 | Wysokość ekranu [px] (landscape) |
| TFT_BACKLIGHT_PWM | 200 | Jasność podświetlenia (0–255) |
| TFT_BL_LEDC_FREQ | 5000 | Częstotliwość PWM podświetlenia [Hz] |
| BTN_DEBOUNCE_MS | 50 | Czas debounce przycisków [ms] |
| BTN_LONG_PRESS_MS | 1000 | Próg długiego naciśnięcia [ms] |
| ENC_ISR_DEBOUNCE_US | 200 | Debounce ISR enkodera [μs] |
| SPEED_CALC_INTERVAL_MS | 250 | Interwał obliczania prędkości [ms] |
| SPEED_FILTER_ALPHA | 0.3 | Współczynnik filtra wykładniczego prędkości |
| PIN_BUZZER | 8 | GPIO pinu buzzera pasywnego |
| BUZZER_LEDC_CH | 1 | Kanał LEDC dla buzzera (0 = podświetlenie TFT) |
| DEFAULT_MAX_PAINT_SPEED_KMH | 15.0 | Domyślny próg alarmu prędkości [km/h] |
| WDT_TIMEOUT_SEC | 3 | Timeout watchdoga [s] z auto-resetem |
| GUN_KEEPALIVE_TIMEOUT_MS | 300 | Timeout keepalive pistoletów [ms] |
| GUN_ANOMALY_DISTANCE_M | 50.0 | Min dystans sesji do detekcji anomalii [m] |
| GUN_ANOMALY_CHECK_MS | 10000 | Interwał sprawdzania anomalii [ms] |
| PIN_GPS_RX | 47 | ESP32 RX ← GPS TX (UART2) |
| PIN_GPS_TX | 48 | ESP32 TX → GPS RX (UART2) |
| GPS_BAUD | 9600 | Domyślny baudrate NEO-6M |
| PIN_JOY_VRX | 19 | Joystick oś X (ADC2_CH8) |
| PIN_JOY_VRY | 20 | Joystick oś Y (ADC2_CH9) |
| PIN_JOY_SW | 46 | Joystick przycisk (strap pin) |
| JOY_DEAD_ZONE | 500 | Strefa martwa ±500 z centrum 2048 |
| JOY_INITIAL_DELAY_MS | 400 | Opóźnienie przed auto-repeat [ms] |
| JOY_REPEAT_MS | 200 | Interwał auto-repeat [ms] |
| MCP23017_I2C_ADDR | 0x20 | Adres I2C ekspandera MCP23017 |
| MCP23017_NUM_BUTTONS | 15 | Liczba przycisków wzorców |
| MCP23017_SCAN_MS | 20 | Interwał skanowania przycisków [ms] |
| MCP23017_BUTTON_MASK | 0x7FFF | Maska bitowa aktywnych przycisków (bity 0–14) |

### 9.2 Kolory UI (format RGB565)

| Stała | Wartość | Kolor | Zastosowanie |
|-------|---------|-------|-------------|
| COLOR_BG | 0x0000 | Czarny | Tło ekranu |
| COLOR_TEXT | 0xFFFF | Biały | Tekst główny |
| COLOR_ACCENT | 0x07E0 | Zielony | Status OK, wzorzec, kalibracja |
| COLOR_WARNING | 0xFBE0 | Żółty | Pauza, odwrócenie, pistolety we wzorcu |
| COLOR_ERROR | 0xF800 | Czerwony | Błędy, zatrzymany |
| COLOR_GUN_ON | 0x07E0 | Zielony | Pistolet aktywnie maluje |
| COLOR_GUN_OFF | 0x4208 | Szary | Pistolet nieużywany |
| COLOR_DIVIDER | 0x4208 | Szary | Separatory |
| COLOR_HEADER_BG | 0x1A3C | Ciemnoniebieski | Tło nagłówka |
| COLOR_MENU_SEL | 0x2A7D | Ciemnozielony | Zaznaczenie w menu |
| COLOR_MENU_TXT | 0xC618 | Jasnoszary | Tekst menu/etykiety |

---

## 10. Uwagi montażowe

### 10.1 Okablowanie

1. **SPI (TFT + SD):** Połączenia jak najkrótsze (max 15–20 cm). Dłuższe przewody mogą powodować błędy komunikacji przy 27 MHz
2. **I2C (RTC):** Do 50 cm z wbudowanymi pull-up na module DS1307
3. **Enkoder:** Przy dłuższych przewodach (>30 cm) dodaj kondensatory filtrujące 100 nF między CLK/DT a GND
4. **Przekaźniki:** Przewody do 50 cm — sygnał cyfrowy 3.3V jest odporny na zakłócenia
5. **Przyciski:** Bez ograniczeń długości dla BS-33B (sygnał cyfrowy z pull-up)
6. **GPS (UART2):** Przewody TX/RX do 1 m — sygnał cyfrowy 3.3V. Antena GPS na zewnątrz kabiny z widocznością nieba

### 10.2 Montaż enkodera

- Enkoder powinien być zamontowany na kole pomiarowym z dobrym stykiem z podłożem
- Koło pomiarowe musi obracać się swobodnie bez poślizgu
- Po montażu wykonać kalibrację na odcinku 10 m

### 10.3 Karta SD

- Format: FAT32
- Moduł SD współdzieli magistralę SPI z wyświetlaczem — nie wymaga dodatkowego okablowania poza jednym przewodem CS (GPIO 16)
- **Ważne:** Przed inicjalizacją TFT pin SD_CS (GPIO 16) jest ustawiany na HIGH. Jeśli SD_CS jest floating (LOW), karta SD może odpowiadać na ruch SPI i zakłócać obraz TFT

### 10.4 Zasilanie przekaźników

- 6 przekaźników jednocześnie: ~420 mA (cewki) + ~20 mA (optocouplers)
- Przy zasilaniu z USB-C (500 mA): może nie wystarczyć przy wszystkich przekaźnikach + TFT + WiFi
- Zalecane: zewnętrzny zasilacz 5V/2A dla modułu przekaźnikowego lub zasilacz USB-C wspierający 1.5A+

### 10.5 Bezpieczeństwo GPIO

- **GPIO 26–37:** ZAJĘTE przez Flash + Octal PSRAM na module N16R8. Podłączenie czegokolwiek spowoduje niestabilność lub crash
- **GPIO 0:** Używany przez bootloader — nie podłączać
- Moduły przekaźnikowe powinny mieć diody zabezpieczające (flyback) — wbudowane w większości modułów

### 10.6 Montaż anteny GPS

- Antena ceramiczna modułu GY-NEO6MV2 **musi mieć widoczność nieba** — zamontuj na zewnątrz kabiny
- Antena na kablu — można poprowadzić przewód do dachu maszyny
- Cold start (pierwsze uruchomienie): do 35 sekund na złapanie fix
- Warm start (kolejne): 1–5 sekund
- Moduł GPS zasilany z 3.3V — podłączyć do pinu 3V3 ESP32-S3

### 10.7 Aktualizacja firmware

- Podłącz ESP32-S3 przez USB-C
- W PlatformIO: `pio run --target upload`
- Alternatywnie: OTA przez WiFi (wymaga dodatkowej implementacji)

---

---

## 11. Nowe moduły w v2.52.0

### 11.1 Zapis trasy GPS (GPX + GeoJSON)

Podczas malowania system buforuje punkty GPS w PSRAM (max 4320 punktów = ~6h). Po STOP eksportuje trasę na kartę SD w dwóch formatach:
- `/tracks/trasa_RRRRMMDD_HHMMSS.gpx` — Google Earth, QGIS, Strava
- `/tracks/trasa_RRRRMMDD_HHMMSS.geojson` — narzędzia GIS, mapy webowe

Punkt GPS zawiera: lat, lng, altitude, speed, timestamp (32 bajty/punkt).

### 11.2 Backup NVS na SD (JSON)

Wszystkie ustawienia NVS (kalibracja, statystyki, wzorce, progi) automatycznie backupowane co 30 min do `/backup/nvs_backup.json`. Automatyczne przywracanie przy pustym NVS (nowe ESP32).

### 11.3 Predykcja zużycia farby

Oblicza zużycie farby na podstawie namalowanej powierzchni i współczynnika (domyślnie 0.6 l/m²). Parametry konfigurowalne z panelu WWW, zapisywane w NVS.

### 11.4 Raporty HTML sesji

Po każdym STOP generowany jest stylizowany raport HTML ze szczegółami sesji, rozbiciem na wzorce, zużyciem farby i koordynatami GPS. Pliki: `/html_reports/raport_RRRRMMDD_HHMMSS.html`.

### 11.5 Czujnik temperatury (opcjonalny)

DS18B20 na GPIO 15 (OneWire). Odczyt co 5 s. Progi: < 5°C (za zimno na farbę), > 35°C (przegrzanie). Wynik na ekranie POST.

### 11.6 Motogodziny (MTH)

Rejestracja czasu pracy silnika malowania. Zapis co 5 min do NVS. Niezależne od czasu sesji.

### 11.7 Auto-pauza / auto-wznowienie

Automatyczna pauza gdy prędkość < 0.5 km/h przez 1.5 s. Automatyczne wznowienie po przekroczeniu progu minimalnej prędkości. Konfigurowalne z WWW.

### 11.8 WebSocket (port 81)

Broadcast statusu JSON co 500 ms do wszystkich podłączonych klientów. Niższe opóźnienie niż HTTP polling.

### 11.9 Tryb nocny

Alternatywna paleta kolorów (amber/dark) dla pracy nocnej. Zmniejsza oślepienie operatora.

### 11.10 Tryb DEMO

Czwarty tryb pracy — nauka operatora. Logika identyczna jak AUTO, ale przekaźniki zawsze OFF. Wizualizacja na ekranie pokazuje co by strzelało.

### 11.11 Ekran POST (Power-On Self-Test)

Diagnostyka startowa: SD, RTC, GPS, MCP23017, enkoder, czujnik temp. Wynik wyświetlany na TFT z 5 s timeoutem.

---

---

## 11. Nowe moduły w v2.52.0 — szczegóły podłączeń

### 11.1 Czujnik temperatury DS18B20 (opcjonalny)

```
    ESP32-S3               DS18B20 (TO-92)
    ┌──────────┐           ┌──────────────┐
    │          │           │              │
    │ GPIO 15  ├───────────┤ DATA (pin 2) │
    │          │    4.7kΩ  │              │
    │    3V3   ├───┤├──────┤ VDD  (pin 3) │
    │          │           │              │
    │    GND   ├───────────┤ GND  (pin 1) │
    └──────────┘           └──────────────┘

    DS18B20 TO-92 (widok od przodu, nóżki w dół):
    ┌─────────┐
    │  DS18B20│
    │    ___  │
    │   /   \ │
    │  │     ││
    │   \___/ │
    └─┤─┤─┤──┘
      1  2  3
     GND DQ VDD

    Rezystor pull-up 4.7kΩ WYMAGANY między DQ a VDD!
    Pin: GPIO 15 (współdzielony z Touch CS — jeśli Touch nie jest aktywny)
```

> **UWAGA:** Czujnik DS18B20 jest opcjonalny. System działa poprawnie bez niego — na ekranie POST wyświetli "Temp: BRAK". Jeśli Touch wyświetlacza jest aktywny, GPIO 15 nie może być użyty do czujnika.

---

## 12. Specyfikacja przewodów i złączy

### 12.1 Zalecane przekroje i typy przewodów

| Magistrala | Typ przewodu | Długość max | Uwagi |
|------------|-------------|-------------|-------|
| **SPI (TFT+SD)** | AWG 24-26, ekranowany | 15–20 cm | Wyżej 27 MHz — wrażliwe na zakłócenia |
| **I2C (RTC+MCP)** | AWG 24-28 | 50 cm | Pull-up 4.7kΩ na module DS1307 |
| **UART (GPS)** | AWG 24-28 | 100 cm | 3.3V, odporny na zakłócenia |
| **Przekaźniki** | AWG 22-24 | 50 cm | Sygnał 3.3V, niskostratne |
| **Przyciski** | AWG 22-28 | Bez limitu | Sygnał cyfrowy z pull-up |
| **Enkoder** | AWG 24, **skrętka** | 200 cm | Dodaj 100 nF przy >30 cm |
| **Zasilanie 5V** | AWG 20-22, **gruby** | 30 cm | Prąd do 1A przy 6 przekaźnikach |
| **Zasilanie 3.3V** | AWG 22-24 | 30 cm | Z regulatora ESP32-S3 |
| **Buzzer** | AWG 24-28 | 50 cm | Sygnał PWM |
| **Joystick** | AWG 24-28, **ekranowany** | 50 cm | ADC wrażliwy na szum |
| **Antena GPS** | Koaksjalny (w zestawie) | Wg producenta | Nie skracać! |

### 12.2 Zalecane typy złączy

| Złącze | Zastosowanie | Typ | Uwagi |
|--------|-------------|-----|-------|
| **Dupont 2.54mm** | Podłączenia do ESP32 i modułów | żeńskie/męskie | Standardowe dla prototypów |
| **JST-XH 2.54mm** | Trwałe podłączenia panelowe | Zatrzaskowe | Lepsze niż Dupont w terenie |
| **Molex KK 2.54mm** | Przekaźniki, zasilanie | Złącze z zabezpieczeniem | Odporne na wibracje |
| **Goldpin 2.54mm** | Na PCB modułów | Lutowane | Nie na przewodach! |
| **Śrubowe (screw terminal)** | Przekaźniki → zawory | AWG 14-22 | Dla przewodów zasilania zaworów |

### 12.3 Schemat kolorów przewodów (zalecany)

```
    ┌──────────────────────────────────────────────────────────┐
    │              STANDARD KOLORÓW PRZEWODÓW                  │
    ├──────────────────────────────────────────────────────────┤
    │                                                          │
    │  🔴 CZERWONY     = Zasilanie 3.3V                       │
    │  🔴+⬜ CZERW.+BIAŁY = Zasilanie 5V (UWAGA!)             │
    │  ⬛ CZARNY       = Masa (GND)                           │
    │                                                          │
    │  🔵 NIEBIESKI    = SPI MOSI / I2C SDA / Enkoder CLK    │
    │  🟣 FIOLETOWY    = SPI SCK                              │
    │  ⬜ SZARY         = SPI MISO                             │
    │  🟡 ŻÓŁTY        = Chip Select (CS) / Przycisk START    │
    │  🟢 ZIELONY      = DC / Enkoder DT / I2C SCL           │
    │  ⬜ BIAŁY         = TFT Reset                            │
    │  🟠 POMARAŃCZOWY = Przekaźniki / Buzzer / LED           │
    │  🟤 BRĄZOWY      = Touch CS                              │
    │                                                          │
    │  Magistrala I2C: zielony+biały (SDA), niebieski+biały (SCL) │
    │  Magistrala UART GPS: zielony (RX), żółty (TX)          │
    └──────────────────────────────────────────────────────────┘
```

---

## 13. Zabezpieczenia elektryczne

### 13.1 Schemat zabezpieczeń

```
    USB-C 5V
      │
      ├──── [Bezpiecznik polimerowy PTC 1.5A] ──── VCC_5V
      │                                              │
      │     ┌────────────────────────────────────────┤
      │     │                                        │
      │  [TVS dioda 5.5V]                    ESP32-S3 DevKit
      │     │                               (regulator 3.3V wbudowany)
      │     GND                                      │
      │                                        VCC_3V3
      │                                              │
      │     ┌────────────────────────────────────────┤
      │     │         │         │         │         │
      │  ILI9341    GPS      MCP23017  Enkoder   Joystick
      │  (3.3V)   (3.3V)    (3.3V)   (3.3V)    (3.3V)
      │
      ├──── Moduł DS1307 (5V)
      │
      └──── Moduł przekaźnikowy 6ch (5V)
             │
             └──── [Diody flyback wbudowane]
                   [Opto-izolacja wbudowana]
```

### 13.2 Zabezpieczenie magistrali SPI

```
    Przełączanie urządzeń SPI — logika Chip Select:

    ESP32-S3          TFT CS      SD CS       Touch CS
    GPIO 10 ─────────[LOW]────── [HIGH]────── [HIGH]     ← TFT aktywny
    GPIO 16 ─────────[HIGH]───── [LOW]─────── [HIGH]     ← SD aktywna
    GPIO 15 ─────────[HIGH]───── [HIGH]────── [LOW]      ← Touch aktywny

    WAŻNE: Przed inicjalizacją TFT system ustawia GPIO 16 (SD_CS) = HIGH
    aby karta SD nie odpowiadała na ruch SPI przeznaczony dla wyświetlacza.

    Kolejność inicjalizacji SPI w setup():
    1. pinMode(PIN_SD_CS, OUTPUT); digitalWrite(PIN_SD_CS, HIGH);
    2. tft.init();          // TFT CS obsługiwany przez bibliotekę
    3. SD.begin(PIN_SD_CS); // SD CS obsługiwany przez bibliotekę SD
```

### 13.3 Zabezpieczenie enkodera przed zakłóceniami

```
    Dla przewodów enkodera > 30 cm:

    GPIO 5 (CLK) ──┬──── Enkoder CLK
                    │
                  [100nF]  ← Kondensator filtrujący
                    │
                   GND

    GPIO 6 (DT) ───┬──── Enkoder DT
                    │
                  [100nF]  ← Kondensator filtrujący
                    │
                   GND

    Dodatkowo: użyj skrętki (twisted pair) dla CLK+DT
    z oddzielnym GND jako trzecim przewodem.
```

### 13.4 Zabezpieczenie przekaźników — diody flyback

```
    Moduł przekaźnikowy (wbudowane zabezpieczenia):

    GPIO ──► [Optocoupler] ──► [Tranzystor] ──► [Cewka przekaźnika]
                                                      │     │
                                                   [Dioda flyback]
                                                      │     │
                                                     VCC   GND

    Wyjście NO (Normally Open) ──► Zawór elektromagnetyczny pistoletu
    Wyjście COM ──────────────────► Zasilanie zaworu (zewnętrzne)

    UWAGA: Zawory pistoletów mają WŁASNE zasilanie (12V/24V DC),
    niezależne od zasilania ESP32. Przekaźnik działa jako przełącznik.
```

---

## 14. Layout PCB — zalecenia dla płytki pośredniczącej

### 14.1 Sugerowany rozkład komponentów

```
    ┌───────────────────────────────────────────────────────────┐
    │                    PŁYTA GŁÓWNA TRASSARV3                  │
    │                                                           │
    │  ┌─────────────┐     ┌──────────────┐    ┌────────────┐  │
    │  │  ESP32-S3   │     │  ILI9341     │    │  Moduł     │  │
    │  │  DevKitC-1  │     │  2.8" TFT    │    │  6-ch      │  │
    │  │  (centralny)│     │  (front      │    │  przekaźn. │  │
    │  │             │     │   panel)     │    │            │  │
    │  └──────┬──────┘     └──────┬───────┘    └─────┬──────┘  │
    │         │                   │                   │         │
    │    ┌────┴────┐         ┌────┴────┐         ┌───┴───┐     │
    │    │  I2C    │         │  SPI    │         │  GPIO │     │
    │    │ Bus     │         │ Bus     │         │ Bus   │     │
    │    └────┬────┘         └────┬────┘         └───┬───┘     │
    │         │                   │                   │         │
    │  ┌──────┴──────┐     ┌─────┴─────┐     ┌──────┴──────┐  │
    │  │ DS1307 RTC  │     │  SD Card  │     │ 6× Zawory   │  │
    │  │ MCP23017    │     │  (w ILI9341│     │ pistoletów  │  │
    │  └─────────────┘     └───────────┘     └─────────────┘  │
    │                                                           │
    │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
    │  │ Enkoder  │  │ 3×przycisk│  │ Joystick │  │  GPS     │ │
    │  │ (koło)   │  │ BS-33B    │  │ KY-023   │  │ NEO-6M   │ │
    │  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │
    │                                                           │
    │  ┌──────────────────────────────────────┐                │
    │  │  MCP23017 + 15 przycisków wzorców    │                │
    │  │  (panel boczny)                      │                │
    │  └──────────────────────────────────────┘                │
    │                                                           │
    │  [Buzzer]    [USB-C zasilanie]    [Zasilacz 5V/2A]       │
    └───────────────────────────────────────────────────────────┘
```

### 14.2 Zasady trasowania

| Reguła | Opis |
|--------|------|
| Separacja SPI | Przewody MOSI/MISO/SCK prowadzić razem, daleko od enkodera i przekaźników |
| Separacja I2C | SDA/SCL prowadzić jako parę, daleko od linii zasilania 5V |
| Masa gwiaździsta | Wszystkie GND zbiegają się w jednym punkcie (przy ESP32) |
| Kondensatory blokujące | 100 nF ceramiczny przy VCC każdego modułu (blisko pinu VCC) |
| Prowadzenie zasilania | Oddzielny przewód 5V do modułu przekaźnikowego (duży prąd) |
| Antena GPS | Kabel antenowy daleko od przewodów SPI i zasilania |

---

## 15. Diagnostyka połączeń elektrycznych

### 15.1 Procedura weryfikacji (checklist montażowy)

**Faza 1 — Zasilanie (BEZ podłączania modułów)**

| # | Test | Narzędzie | Wynik OK |
|---|------|-----------|----------|
| 1 | Napięcie na 3V3 | Multimetr | 3.20–3.40 V |
| 2 | Napięcie na 5V (VBUS) | Multimetr | 4.75–5.25 V |
| 3 | Napięcie GND–3V3 | Multimetr | 3.20–3.40 V |
| 4 | Napięcie GND–5V | Multimetr | 4.75–5.25 V |

**Faza 2 — I2C (podłącz DS1307 i MCP23017)**

| # | Test | Narzędzie | Wynik OK |
|---|------|-----------|----------|
| 5 | I2C scan (Wire.begin + scan) | Monitor szeregowy | 0x20, 0x68 |
| 6 | Odczyt czasu RTC | Monitor szeregowy | Data i godzina poprawna |
| 7 | Odczyt rejestrów MCP23017 | Monitor szeregowy | 0xFF (pull-up, brak wciśnięć) |

**Faza 3 — SPI (podłącz ILI9341)**

| # | Test | Narzędzie | Wynik OK |
|---|------|-----------|----------|
| 8 | Inicjalizacja TFT | Wzrok | Ekran powitalny TrassarV3 |
| 9 | Inicjalizacja SD | Monitor szeregowy | "SD OK" |
| 10 | Podświetlenie | Wzrok | Jasny ekran (PWM 200/255) |

**Faza 4 — Przekaźniki (podłącz moduł)**

| # | Test | Narzędzie | Wynik OK |
|---|------|-----------|----------|
| 11 | Czyszczenie dysz → P1 | Słuch (kliknięcie) | Przekaźnik klika |
| 12 | Czyszczenie dysz → P2 | Słuch | Przekaźnik klika |
| 13 | Czyszczenie dysz → P3–P6 | Słuch | Wszystkie klikają |

**Faza 5 — Enkoder, przyciski, joystick**

| # | Test | Narzędzie | Wynik OK |
|---|------|-----------|----------|
| 14 | Obrót enkodera | Ekran → prędkość | Prędkość > 0 |
| 15 | START | Ekran | Zmiana stanu |
| 16 | STOP | Ekran | Zmiana stanu |
| 17 | SELEKTOR | Ekran | Nawigacja w menu |
| 18 | GAP | Ekran | "Start od przerwy" |
| 19 | Joystick góra/dół | Menu | Nawigacja |
| 20 | Joystick lewo/prawo | Menu | Wejście/cofnij |

**Faza 6 — GPS i WiFi**

| # | Test | Narzędzie | Wynik OK |
|---|------|-----------|----------|
| 21 | GPS fix | Panel WWW → GPS | Fix: TAK po 1–2 min |
| 22 | WiFi AP | Telefon → WiFi | Widoczna sieć TrassarV3 |
| 23 | Panel WWW | Przeglądarka | http://192.168.4.1 ładuje się |

### 15.2 Typowe błędy montażowe i ich objawy

| Objaw | Prawdopodobna przyczyna | Test |
|-------|------------------------|------|
| Biały ekran TFT | Zamienione MOSI/MISO lub brak CS | Sprawdź GPIO 11↔MOSI, 13↔MISO, 10↔CS |
| Migający ekran TFT | SD_CS floating LOW | Sprawdź GPIO 16 → HIGH przed tft.init() |
| I2C scan: 0 urządzeń | Zamienione SDA/SCL | Sprawdź GPIO 17↔SDA, 18↔SCL |
| I2C scan: tylko 0x68 | MCP23017 brak zasilania lub adres | Sprawdź VDD=3.3V, A0=A1=A2=GND |
| Przekaźnik nie klika | Brak 5V na module | Sprawdź VCC przekaźnika → 5V (VBUS) |
| Enkoder liczy do tyłu | Zamienione CLK/DT | Zamień GPIO 5↔6 |
| GPS brak danych | Zamienione TX/RX | Zamień GPIO 47↔48 |
| Joystick driftuje | Szum ADC | Zwiększ JOY_DEAD_ZONE, dodaj kondensator |
| Boot loop | GPIO 46 zwarty do GND | Nie wciskaj joysticka przy starcie |
| Boot loop | GPIO 26–37 podłączone | Odłącz — zajęte przez PSRAM! |

### 15.3 Pomiar prądów — weryfikacja zasilania

```
    Test poboru prądu — podłącz multimetr szeregowo w linię USB-C:

    Zasilacz USB-C ──[A]── ESP32-S3
                     │
                   Multimetr
                   (zakres 2A DC)

    Oczekiwane odczyty:
    ┌─────────────────────────────────┬────────────┐
    │ Stan                            │ Prąd [mA]  │
    ├─────────────────────────────────┼────────────┤
    │ Boot (POST)                     │ 200–300    │
    │ IDLE (HOME, WiFi, TFT, GPS)    │ 250–350    │
    │ Malowanie 1 pistolet           │ 350–450    │
    │ Malowanie 2 pistolety          │ 420–520    │
    │ Malowanie 3 pistolety          │ 490–600    │
    │ Malowanie 6 pistoletów + SD    │ 700–960    │
    │ WebSocket + 4 klienty          │ +30–50     │
    └─────────────────────────────────┴────────────┘

    Jeśli IDLE > 500 mA → zwarcie lub uszkodzony moduł
    Jeśli 6 pistoletów > 1200 mA → użyj zewnętrznego zasilacza
    dla modułu przekaźnikowego
```

---

## 16. Kompletny diagram okablowania — widok z lotu ptaka

```
                                 ANTENA GPS
                                 (na zewnątrz)
                                    │
                                    │ kabel
                            ┌───────┴───────┐
                            │  GPS NEO-6M   │
                            │  GY-NEO6MV2   │
                            │  TX→GPIO47    │
                            │  RX←GPIO48    │
                            │  VCC←3V3      │
                            │  GND←GND      │
                            └───────────────┘
                                    │
    ┌───────────────────────────────┼───────────────────────────────┐
    │                               │                               │
    │              ┌────────────────┴────────────────┐              │
    │              │                                  │              │
    │              │         ESP32-S3 N16R8           │              │
    │              │         DevKitC-1                │              │
    │              │                                  │              │
    │  ┌───────────┤  3V3  5V  GND                   ├──────────┐  │
    │  │           │                                  │          │  │
    │  │  ┌────────┤  GPIO 5,6,7 (Enkoder)           │          │  │
    │  │  │        │  GPIO 38,39,40 (Przyciski)      │          │  │
    │  │  │  ┌─────┤  GPIO 19,20,46 (Joystick)      │          │  │
    │  │  │  │     │  GPIO 8 (Buzzer)                │          │  │
    │  │  │  │     │  GPIO 41,42,1,2,3,4 (Przek.)   ├──┐       │  │
    │  │  │  │     │  GPIO 10,9,14,21 (TFT ctrl)    │  │       │  │
    │  │  │  │     │  GPIO 11,12,13 (SPI bus)        │  │       │  │
    │  │  │  │     │  GPIO 15,16 (Touch CS, SD CS)   │  │       │  │
    │  │  │  │     │  GPIO 17,18 (I2C SDA/SCL)       │  │       │  │
    │  │  │  │     │                                  │  │       │  │
    │  │  │  │     └────────────────┬─────────────────┘  │       │  │
    │  │  │  │                      │                    │       │  │
    │  │  │  │                      │ USB-C              │       │  │
    │  │  │  │                ┌─────┴─────┐              │       │  │
    │  │  │  │                │ Zasilacz  │              │       │  │
    │  │  │  │                │ 5V/2A     │              │       │  │
    │  │  │  │                └───────────┘              │       │  │
    │  │  │  │                                           │       │  │
    │  │  │  │                                           │       │  │
    │  │  │  │                                           │       │  │
┌───┴──┴──┴──┴───┐  ┌──────────────┐  ┌─────────────┐  │  ┌────┴────────┐
│  PANEL         │  │  WYŚWIETLACZ │  │  DS1307 RTC │  │  │  MODUŁ      │
│  STEROWANIA    │  │  ILI9341     │  │  + CR2032   │  │  │  PRZEKAŹN.  │
│                │  │  2.8" TFT    │  │  I2C: 0x68  │  │  │  6-kanałowy │
│  [START]       │  │  + SD card   │  └──────┬──────┘  │  │             │
│  [STOP]        │  │  SPI 27MHz   │         │ I2C     │  │  IN1→P1     │
│  [SELECT]      │  └──────┬───────┘         │         │  │  IN2→P2     │
│  [GAP]         │         │ SPI             │         │  │  IN3→P3     │
│                │         │                 │         │  │  IN4→P4     │
│  Enkoder       │  ┌──────┴───────┐  ┌──────┴──────┐ │  │  IN5→P5     │
│  CLK/DT/SW     │  │  MicroSD     │  │  MCP23017   │ │  │  IN6→P6     │
│                │  │  FAT32       │  │  I2C: 0x20  │ │  │             │
│  Joystick      │  │  CS=GPIO 16  │  │  15 przycisk│ │  │  VCC←5V     │
│  KY-023        │  └──────────────┘  └──────┬──────┘ │  │  GND←GND    │
│                │                           │         │  │             │
│  Buzzer        │                    ┌──────┴──────┐  │  │  NO→Zawory  │
│  GPIO 8        │                    │ 15 PRZYCISK.│  │  └─────────────┘
└────────────────┘                    │ WZORCÓW     │  │
                                      │ P-1a...P-7d│  │
                                      └─────────────┘  │
                                                       │
                                              ┌────────┴────────┐
                                              │  6× ZAWORY      │
                                              │  PISTOLETÓW      │
                                              │  NATRYSKOWYCH    │
                                              │  (12V/24V DC)    │
                                              │  zewn. zasilanie │
                                              └─────────────────┘
```

---

## 17. Zabezpieczenia sprzętowo-programowe (v2.52.0 SAFETY PATCH)

### 17.1 Wielowarstwowa ochrona pistoletów

System TrassarV3 v2.52.0 implementuje **5 warstw ochrony** przed niekontrolowanym działaniem pistoletów natryskowych:

```
Warstwa 1: Sprzętowy STOP awaryjny (ISR na GPIO 39)
   │        Bezpośredni zapis do rejestrów GPIO — <1 µs, niezależny od oprogramowania
   │
Warstwa 2: Gun keepalive (300 ms timeout)
   │        Core 1 + Core 0 monitorują niezależnie — brak update() → allOff()
   │
Warstwa 3: Overspeed gun disable (v2.52.0)
   │        Przekroczenie maxSpeedKmh → natychmiastowe wyłączenie pistoletów + alarm
   │
Warstwa 4: Shutdown handler (v2.52.0)
   │        esp_register_shutdown_handler() → guns OFF PRZED resetem WDT/panic
   │        Bezpośredni GPIO register write — działa nawet w kontekście panic
   │
Warstwa 5: Watchdog timer (3s)
           TWDT per-task — ostatnia linia obrony, reset całego ESP
```

### 17.2 Izolacja awarii Core 0 (serwer WWW)

```
    Core 1 (loop)                        Core 0 (web task)
    ┌─────────────────┐                  ┌──────────────────┐
    │  Enkoder        │                  │  HTTP server     │
    │  Przyciski      │   monitoruje     │  WebSocket       │
    │  Pistolety      │◄────────────────►│  REST API        │
    │  Wyświetlacz    │  core0AliveMs    │                  │
    │  GPS/RTC        │                  │  WDT per-task    │
    │                 │                  │                  │
    │ Soft watchdog   │  ┌────────────┐  │ Aktualizuje      │
    │ sprawdza co 5s  │──│ Restart    │  │ core0AliveMs     │
    │ isCore0Alive()  │  │ web task   │  │ co 2 ms          │
    │                 │  │ (nie ESP!) │  │                  │
    └─────────────────┘  └────────────┘  └──────────────────┘

    Awaria Core 0:
    1. core0AliveMs przestaje się aktualizować
    2. Core 1 wykrywa po 10s (isCore0Alive timeout)
    3. restartWebTask() — usuwa stary task, tworzy nowy
    4. Malowanie NIE jest przerywane, pistolety NIE są wyłączane
    5. Event log: "Core 0 web task nie odpowiada — restart tasku"
```

### 17.3 Detekcja zablokowanego przekaźnika

```
    ┌─── Normalny cykl (wzorzec DASHED) ───┐
    │                                        │
    │  ON ████████████      ON ████████████  │
    │  OFF            ██████              ██ │
    │     ← lineLen → ← gapLen →            │
    │                                        │
    └────────────────────────────────────────┘

    ┌─── Podejrzenie zablokowanego przekaźnika ───┐
    │                                              │
    │  ON ██████████████████████████████████████████│  >60s ciągły ON
    │                                              │  bez cyklowania!
    │  → Alarm BUZ_ERROR                           │
    │  → Event log: "Podejrzenie zablokowanego     │
    │    przekaznika: P3 (ON > 60s)"               │
    │                                              │
    └──────────────────────────────────────────────┘
```

**Parametry:** Sprawdzanie co 5s (`GUN_RELAY_STUCK_CHECK_MS`), próg 60s ciągłego ON (`GUN_RELAY_MAX_CONT_ON_MS`). Dotyczy wyłącznie pistoletów w trybie `GUN_DASHED` — pistolet `GUN_CONTINUOUS` nie jest monitorowany (ciągły ON jest prawidłowy).

### 17.4 Automatyczne działanie przy niskim heapie

```
    Heap wolny                    Działanie
    ────────────────────────────────────────────────
    > 64 KB                       Normalny tryb pracy
    ────────────────────────────────────────────────
    < 64 KB (WARNING)             Ostrzeżenie w logu
    ────────────────────────────────────────────────
    < 32 KB (CRITICAL)            • Wyłączenie broadcastu WebSocket
                                  • Zatrzymanie malowania (stop)
                                  • Alarm BUZ_ERROR
                                  • Event log: "KRYTYCZNY heap"
    ────────────────────────────────────────────────
```

### 17.5 Ochrona SPI (TFT vs karta SD)

```
    Przed KAŻDĄ operacją renderowania TFT:

    digitalWrite(PIN_SD_CS, HIGH)  ← gwarantuje że karta SD
         │                           nie odpowiada na ruch SPI
         ▼
    menu.update()                  ← bezpieczne renderowanie TFT
         │
         ▼
    SD_LOCK() / SD_UNLOCK()        ← mutex chroni każdy dostęp do SD
                                     (timeout 2s < WDT 3s)
```

### 17.6 Overspeed — wyłączenie pistoletów

```
    Prędkość          Działanie pistoletów     Alarm
    ──────────────────────────────────────────────────
    < minSpeed         OFF (za wolno)           BUZ_LOW_SPEED co 3s
    ──────────────────────────────────────────────────
    minSpeed...maxSpeed ON (normalny tryb)      Brak
    ──────────────────────────────────────────────────
    > maxSpeed          OFF (za szybko!)        BUZ_OVERSPEED co 2s
                        + natychmiastowe        + log zdarzenia
                        guns.allOff()
    ──────────────────────────────────────────────────
```

### 17.7 Auto-resume z cooldown

```
    ┌──── Cykl auto-pauza / auto-resume ────┐
    │                                         │
    │  Malowanie → prędkość < 0.5 km/h       │
    │     │                                   │
    │     ▼  (1.5s delay)                     │
    │  AUTO-PAUZA → pistolety OFF             │
    │     │                                   │
    │     ▼  prędkość >= minSpeed (0.5s debounce) │
    │  AUTO-RESUME → pistolety ON             │
    │     │                                   │
    │     ▼  COOLDOWN 2s                      │
    │  Auto-pauza ZABLOKOWANA na 2s           │
    │  (zapobiega oscylacji pauza↔resume)     │
    │     │                                   │
    │     ▼  Po 2s — normalna detekcja        │
    └─────────────────────────────────────────┘
```

---

## 18. FAQ — Najczęściej zadawane pytania o podłączenia

### Q: Czy mogę użyć innych pinów GPIO?

**A:** Tak, ale wymagana jest zmiana w pliku `config.h` i ponowna kompilacja firmware. Pamiętaj:
- GPIO 26–37: **ZAKAZANE** (PSRAM)
- GPIO 0: Zarezerwowany (bootloader)
- GPIO 43, 44: UART0 (monitor szeregowy) — nie używać
- GPIO 45: Strap pin — unikać
- GPIO 46: Strap pin (joystick SW) — ostrożność przy starcie

### Q: Czy mogę zasilić ESP32-S3 z baterii?

**A:** Tak, ale:
- Bateria LiPo 3.7V + przetwornica boost do 5V (minimum 1.5A)
- Lub power bank USB-C z output 5V/2A
- Monitoruj napięcie — przy <4.5V system może zachowywać się niestabilnie

### Q: Ile MCP23017 mogę podłączyć?

**A:** Do 8 sztuk na jednej magistrali I2C (adresy 0x20–0x27 przez A0/A1/A2). Kod obsługuje aktualnie 1 sztukę (0x20). Rozszerzenie wymaga modyfikacji `pattern_buttons.cpp`.

### Q: Czy mogę użyć wyświetlacza innego niż ILI9341?

**A:** Biblioteka TFT_eSPI obsługuje wiele sterowników (ST7735, ST7789, ILI9488, HX8357 itp.), ale:
- Zmiana sterownika wymaga modyfikacji flag kompilacji w `platformio.ini`
- Layout UI jest zaprojektowany dla rozdzielczości 320×240
- Inna rozdzielczość wymaga modyfikacji stałych w `display_internal.h`

### Q: Dlaczego DS1307 wymaga 5V a MCP23017 3.3V?

**A:** DS1307 jest zaprojektowany dla 5V (Vcc min. 4.5V wg datasheet). MCP23017 akceptuje 1.8–5.5V, ale przy 3.3V zapewnia kompatybilność poziomów logicznych z ESP32-S3 (3.3V). Obie układy współdzielą magistralę I2C — moduł DS1307 ma wbudowane pull-upy zasilane z jego VCC (5V), ale piny SDA/SCL ESP32-S3 tolerują 5V (są 5V-tolerant na większości GPIO).

### Q: Czy mogę wydłużyć kabel enkodera do 5 m?

**A:** Przy 5 m zalecamy:
1. Użyj skrętki (twisted pair) CAT5/CAT6
2. Dodaj kondensatory 100 nF na obu końcach (CLK i DT)
3. Rozważ driver linii RS-485 dla bardzo długich kabli
4. Zwiększ `ENC_ISR_DEBOUNCE_US` do 500–1000 µs
5. Przetestuj dokładność kalibracji po montażu

---

*TrassarV3 — Dokumentacja techniczna v2.52.0 (SAFETY PATCH)*
*ESP32-S3 N16R8 | ILI9341 320×240 | GPS NEO-6M + GPX/GeoJSON | MCP23017 | 6 pistoletów | 16 wzorców | 15 przycisków | 4 tryby pracy | WiFi AP + WebSocket | backup NVS | motogodziny | predykcja farby | raporty HTML*
*v2.52.0: shutdown handler, overspeed gun disable, relay stuck detection, Core 0 isolation, low heap protection, SPI contention fix, API validation, auto-resume cooldown*
*Dokumentacja aktualizowana: marzec 2026*
