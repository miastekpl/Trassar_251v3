# TrassarV3 - Dokumentacja techniczna i schemat podłączeń v2.21.0

## Spis treści

1. [Specyfikacja sprzętowa](#1-specyfikacja-sprzętowa)
2. [Tabela podłączeń pinów ESP32-S3 N16R8](#2-tabela-podłączeń-pinów-esp32-s3-n16r8)
3. [Kompletna mapa GPIO](#3-kompletna-mapa-gpio)
4. [Mapowanie funkcji na piny](#4-mapowanie-funkcji-na-piny)
5. [Schemat blokowy systemu](#5-schemat-blokowy-systemu)
6. [Schematy podłączeń poszczególnych modułów](#6-schematy-podłączeń-poszczególnych-modułów)
7. [Zasilanie](#7-zasilanie)
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

### 1.3 Firmware

| Parametr | Wartość |
|----------|---------|
| Wersja | 2.21.0 |
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
| 12 | Przewody połączeniowe Dupont | ~45 | Żeńsko-żeński i żeńsko-męski |
| 13 | Zasilacz USB-C 5V/2A | 1 | Minimum 1.5A przy pełnym obciążeniu |
| 14 | Koło pomiarowe + uchwyt enkodera | 1 | Obwód dopasowany do kalibracji |
| 15 | Zawory elektromagnetyczne pistoletów | 6 | Podłączenie do wyjść NO przekaźników |

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

### 2.4 Enkoder obrotowy (pomiar dystansu i prędkości)

| Pin enkodera | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|-------------|-------------|------|----------|------|
| GND | GND | — | — | Masa |
| CLK (A) | GPIO 5 | 5 | INPUT_PULLUP | Sygnał A (ISR CHANGE, kwadraturowy x4) |
| DT (B) | GPIO 6 | 6 | INPUT_PULLUP | Sygnał B (ISR CHANGE, kwadraturowy x4) |
| SW | GPIO 7 | 7 | INPUT_PULLUP | Przycisk "Start od przerwy" |
| + (VCC) | 3V3 | — | — | Zasilanie (opcjonalne) |

> **Uwaga:** Piny CLK i DT mają włączone wewnętrzne rezystory pull-up ESP32-S3. Enkoder pracuje w trybie kwadraturowym x4 — ISR CHANGE na obu kanałach A i B z tablicą stanów 4×4 (Gray code). Bezpośredni odczyt rejestru GPIO (~50 ns). Debouncing ISR: 200 μs (ENC_ISR_DEBOUNCE_US). Obliczanie prędkości: co 250 ms z filtrem wykładniczym (alpha = 0.3).

### 2.5 Joystick analogowy KY-023

| Pin KY-023 | Pin ESP32-S3 | GPIO | Kierunek | Opis |
|------------|-------------|------|----------|------|
| GND | GND | — | — | Masa |
| +5V | 3V3 | — | — | Zasilanie 3.3V |
| VRx | GPIO 19 | 19 | ANALOG (ADC2) | Oś pozioma (lewo/prawo) |
| VRy | GPIO 20 | 20 | ANALOG (ADC2) | Oś pionowa (góra/dół) |
| SW | GPIO 46 | 46 | INPUT_PULLUP | Przycisk wciskany (aktywny LOW) |

> **Uwaga:** GPIO 46 jest pinem strapping (ROM boot select). Z wewnętrznym pull-up jest HIGH podczas startu (normalny boot z Flash). **Nie wciskać joysticka podczas włączania urządzenia** — może spowodować wejście w tryb download.

### 2.6 Przyciski sterujące (BS-33B monostabilne)

| Przycisk | Pin ESP32-S3 | GPIO | Kierunek | Funkcja |
|----------|-------------|------|----------|---------|
| START | GPIO 38 | 38 | INPUT_PULLUP | Start / Pauza / Wznów / Wybór trybu (1 s, HOME) |
| STOP | GPIO 39 | 39 | INPUT_PULLUP | Stop / Menu (1 s) / Cofnij |
| SELEKTOR | GPIO 40 | 40 | INPUT_PULLUP | Odwróć P-3a/P-3b (HOME/PAINTING), nawigacja + wejście w opcję (menu serwis.) |
| GAP (od przerwy) | GPIO 7 | 7 | INPUT_PULLUP | Start od przerwy (HOME) |

> **UWAGA:** GPIO 26–37 są zajęte przez Octal PSRAM modułu N16R8! NIE wolno ich używać!
>
> **Parametry przycisków:** Debounce: 50 ms, Długie naciśnięcie: 1000 ms. Podłączenie: jeden styk do GPIO, drugi do GND. Wewnętrzne pull-up aktywowane programowo.

### 2.6 Przekaźniki pistoletów (6 szt.)

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
| **5** | Enkoder CLK (A) | INPUT_PULLUP | ISR CHANGE, kwadraturowy x4, debounce 200 μs |
| **6** | Enkoder DT (B) | INPUT_PULLUP | ISR CHANGE, kwadraturowy x4, debounce 200 μs |
| **7** | Przycisk GAP (SW enkodera) | INPUT_PULLUP | "Start od przerwy" |
| **8** | Buzzer | PWM (LEDC ch1) | Sygnalizacja dźwiękowa (pasywny) |
| **9** | TFT DC | OUTPUT | Data/Command |
| **10** | TFT CS | OUTPUT | Chip Select wyświetlacza |
| **11** | SPI MOSI | OUTPUT | Wspólny TFT + SD |
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

Zmiana wzorca jest możliwa **wyłącznie** przez:
- **Panel WWW** — 16 przycisków wzorców (http://192.168.4.1), w tym WŁASNY
- **API REST** — `POST /api/control` z `action=set_pattern&value=0..15` (15 = WŁASNY)

Na fizycznym panelu sterowania (ekran HOME i PAINTING) przycisk SELEKTOR **nie zmienia** wzorca.

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
    ┌───────────┐  I2C    │  GPIO 17 ↔ SDA                    │
    │  DS1307   │◄────────│  GPIO 18 ← SCL                    │
    │  RTC      │         │                                   │
    │  CR2032   │         │                                   │
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
                          │  WebSocket: ws://192.168.4.1:81   │
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

### 6.7 Podłączenie wyświetlacza i karty SD (wspólna magistrala SPI)


```
    ESP32-S3               Moduł ILI9341 2.8" (TFT + SD + Touch)
    ┌──────────┐           ┌──────────────────────────────────┐
    │          │           │                                  │
    │ GPIO 11  ├───────────┤ MOSI (TFT_SDI / SD_MOSI / T_DIN)│
    │ GPIO 12  ├───────────┤ SCK  (TFT_SCK / SD_SCK / T_CLK) │
    │ GPIO 13  ├───────────┤ MISO (TFT_SDO / SD_MISO / T_DO) │
    │          │           │                                  │
    │ GPIO 10  ├───────────┤ TFT_CS                           │
    │ GPIO  9  ├───────────┤ TFT_DC                           │
    │ GPIO 14  ├───────────┤ TFT_RST                          │
    │ GPIO 21  ├───────────┤ TFT_LED (podświetlenie PWM)      │
    │          │           │                                  │
    │ GPIO 15  ├───────────┤ T_CS (Touch Chip Select)         │
    │          │           │                                  │
    │ GPIO 16  ├───────────┤ SD_CS (SD Card Chip Select)      │
    │          │           │                                  │
    │    3V3   ├───────────┤ VCC                              │
    │    GND   ├───────────┤ GND                              │
    └──────────┘           └──────────────────────────────────┘

    Magistrala SPI współdzielona — przełączanie urządzeń przez CS:
      GPIO 10 = LOW → komunikacja z TFT
      GPIO 16 = LOW → komunikacja z kartą SD
      GPIO 15 = LOW → komunikacja z Touch
```

### 6.8 Podłączenie zegara RTC DS1307

```
    ESP32-S3               Moduł DS1307
    ┌──────────┐           ┌──────────────┐
    │          │           │              │
    │ GPIO 17  ├───────────┤ SDA          │
    │ GPIO 18  ├───────────┤ SCL          │
    │          │           │              │
    │    5V    ├───────────┤ VCC          │
    │    GND   ├───────────┤ GND          │
    └──────────┘           │   [CR2032]   │
                           └──────────────┘
```

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

### 7.3 Uwagi o zasilaniu

- Wyświetlacz ILI9341 zasilany z pinu 3V3 płytki ESP32-S3
- Moduł DS1307 wymaga 5V — podłączyć do pinu 5V (VBUS)
- Moduły przekaźnikowe wymagają 5V — podłączyć do pinu 5V (VBUS)
- **Ważne:** Przy 6 przekaźnikach aktywnych jednocześnie pobór prądu jest znaczny (~6 × 70 mA = ~420 mA dla cewek). Przy większych obciążeniach rozważ zewnętrzne zasilanie 5V dla modułu przekaźnikowego
- Bateria CR2032 w module DS1307 podtrzymuje czas po odłączeniu zasilania głównego

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
| **web_server** | web_server.cpp/h | WiFi AP + HTTP + API REST + WebSocket (port 81) |
| **gps_track** | gps_track.cpp/h | Zapis trasy GPS (GPX + GeoJSON) na SD |
| **event_log** | event_log.cpp/h | Log zdarzeń na kartę SD (dzienne pliki) |
| **nvs_backup** | nvs_backup.cpp/h | Backup/restore NVS na kartę SD (JSON) |

### 8.2 Architektura dual-core (v2.6.0)

```
╔══════════════════════════════════╗  ╔══════════════════════════════════╗
║         CORE 1 (loop)           ║  ║      CORE 0 (FreeRTOS)          ║
║                                  ║  ║                                  ║
║  0. esp_task_wdt_reset()         ║  ║  webTaskFunc() {                 ║
║  1. buttons.update()             ║  ║      vTaskDelay(5s) // WDT init  ║
║  1b. joystick.update()           ║  ║      esp_task_wdt_add(NULL)      ║
║  2. menu.handleEvent()           ║  ║      for(;;) {                   ║
║  3. encoderDist.update()         ║  ║        esp_task_wdt_reset()      ║
║  4. rtcModule.update()           ║  ║        server.handleClient()     ║
║  5. paintEngine.update()         ║  ║        wsServer.loop()           ║
║  5b. checkGunKeepAlive()         ║  ║        broadcast co 500ms (WS)   ║
║  5c. buzzer.update()             ║  ║        vTaskDelay(2ms)           ║
║  5d. gpsTrack.addPoint() (5s)    ║  ║      }                           ║
║  6. display refresh (500ms)      ║  ║  }                               ║
║  7. menu.update() (100ms)        ║  ║                                  ║
║  8. lifetime save (60s)          ║  ║  Stack: 16384 B                  ║
║  9. diagnostyka (30s)            ║  ║  Priorytet: 1                    ║
║  10. anomalia pistoletów (10s)   ║  ║  WDT: 3s (dual watchdog)        ║
║  11. cache raportów SD (15s)     ║  ╚══════════════════════════════════╝
║  12. NVS backup (30min)          ║
║  13. event log (anomalie)        ║
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
| WS broadcast | 500 ms | Push statusu JSON przez WebSocket (port 81) |
| WS fallback polling | 2000 ms | REST polling /api/status gdy WebSocket niedostępny |
| WDT_TIMEOUT_SEC | 3000 ms | Watchdog timer — auto-reset ESP32 |
| GUN_KEEPALIVE_TIMEOUT_MS | 300 ms | Awaryjne wyłączenie pistoletów |
| LIFETIME_SAVE_MS | 60000 ms | Okresowy zapis statystyk do NVS |
| DIAG_PRINT_MS | 30000 ms | Diagnostyka systemowa (Serial) |
| GUN_ANOMALY_CHECK_MS | 10000 ms | Sprawdzanie anomalii pistoletów |
| REPORT_CACHE_MS | 15000 ms | Odświeżanie cache raportów SD |
| GPX_RECORD_INTERVAL_MS | 5000 ms | Zapis punktu GPS do bufora trasy |
| NVS_BACKUP | 1800000 ms | Backup NVS na kartę SD (30 min) |

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
       │    │    (3 tryby pracy:   │    │
       │    │  AUTO/SEMI/MANUAL)   │    │
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
    2. Sprawdź prędkość >= minSpeedKmh (domyślnie 3 km/h, konfigurowalne)
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

    4. Zaktualizuj statystyki (dystans, powierzchnia)
```

### 8.6 API REST

| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/` | GET | Strona HTML panelu sterowania |
| `/api/status` | GET | JSON ze stanem systemu (+ anomalia pistoletów) |
| `/api/stats` | GET | Statystyki lifetime + sesja + per-gun |
| `/api/reports` | GET | Lista plików raportów CSV z karty SD |
| `/api/control` | POST | Sterowanie maszyną (action=start\|pause\|stop\|start_from_gap\|set_pattern\|toggle_reverse\|set_mode\|semi_next_line\|save_custom_pattern\|cal_start\|cal_finish\|set_max_speed\|set_min_speed) |
| `/api/reports/geojson` | GET | Konwersja raportu CSV → GeoJSON Points (chunked) |
| `/api/tracks` | GET | Lista plików tras GPS (GPX/GeoJSON) |
| `/api/tracks/download` | GET | Pobieranie pliku trasy GPS |
| **WebSocket :81** | WS | Push statusu JSON co 500 ms do klientów |

Szczegółowa dokumentacja API → [API_WWW.md](API_WWW.md)

---

## 9. Parametry konfiguracyjne

### 9.1 Parametry zdefiniowane w config.h

| Parametr | Wartość | Opis |
|----------|---------|------|
| FW_VERSION | "2.21.0" | Wersja firmware |
| FW_NAME | "TrassarV3" | Nazwa systemu |
| WIFI_AP_SSID | "TrassarV3" | Nazwa sieci WiFi |
| WIFI_AP_PASS | "12345678" | Hasło WiFi |
| WIFI_AP_CHANNEL | 6 | Kanał WiFi |
| WIFI_AP_MAX_CON | 4 | Max klientów WiFi |
| WEB_SERVER_PORT | 80 | Port serwera HTTP |
| DEFAULT_MIN_PAINT_SPEED_KMH | 3.0 | Domyślny próg minimalnej prędkości [km/h] (konfigurowalne runtime) |
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
| WS_PORT | 81 | Port serwera WebSocket |
| WS_BROADCAST_MS | 500 | Interwał broadcast statusu przez WebSocket [ms] |
| GPX_RECORD_INTERVAL_MS | 5000 | Interwał zapisu punktu trasy GPS [ms] |
| GPX_MAX_POINTS | 4320 | Max punktów GPS w buforze PSRAM (~6h) |
| NVS_BACKUP_INTERVAL_MS | 1800000 | Interwał backupu NVS na SD [ms] (30 min) |
| EVENT_LOG_MAX_SIZE | 65536 | Max rozmiar dziennego pliku logu [B] (64 KB) |

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

*TrassarV3 — Dokumentacja techniczna v2.21.0*
*ESP32-S3 N16R8 | ILI9341 320×240 | GPS NEO-6M | 6 pistoletów | 16 wzorców | 3 tryby pracy | WiFi AP*
