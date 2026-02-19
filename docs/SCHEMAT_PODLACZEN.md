# TrassarV3 - Dokumentacja techniczna i schemat podłączeń v2.11.0

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

### 1.2 Peryferia

| Komponent | Model | Interfejs | Opis |
|-----------|-------|-----------|------|
| Wyświetlacz | ILI9341 2.8" TFT | SPI (HSPI/SPI3) | 320×240 px, landscape, 27 MHz |
| Karta SD | Zintegrowana w module TFT | SPI (HSPI/SPI3) | FAT32, współdzieli magistralę z TFT |
| Zegar RTC | DS1307 | I2C | Bateria CR2032, podtrzymanie czasu |
| Enkoder | Obrotowy (koło pomiarowe) | Digital + ISR | CLK/DT + przycisk SW |
| Przyciski | BS-33B monostabilne × 3 + 1 | Digital (pull-up) | START, STOP, SELEKTOR, GAP |
| Przekaźniki | Moduł 6-kanałowy 5V | Digital | Sterowanie pistoletami P1–P6 |
| Buzzer | Pasywny | LEDC PWM (kanał 1) | Sygnalizacja dźwiękowa, GPIO 8 |

### 1.3 Firmware

| Parametr | Wartość |
|----------|---------|
| Wersja | 2.9.0 |
| Platforma | ESP32-S3 (PlatformIO) |
| Biblioteki | TFT_eSPI, ArduinoJson v7, SD, Wire, WiFi, esp_task_wdt |
| Orientacja ekranu | Landscape (setRotation 1) |
| Anti-flicker | setTextPadding() zamiast clear() na HOME/PAINTING |

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
| CLK (A) | GPIO 5 | 5 | INPUT_PULLUP | Sygnał A (przerwanie ISR CHANGE) |
| DT (B) | GPIO 6 | 6 | INPUT_PULLUP | Sygnał B |
| SW | GPIO 7 | 7 | INPUT_PULLUP | Przycisk "Start od przerwy" |
| + (VCC) | 3V3 | — | — | Zasilanie (opcjonalne) |

> **Uwaga:** Piny CLK i DT mają włączone wewnętrzne rezystory pull-up ESP32-S3. Enkoder służy wyłącznie do pomiaru dystansu i prędkości (ISR na CLK/CHANGE). Debouncing ISR: 200 μs (ENC_ISR_DEBOUNCE_US). Obliczanie prędkości: co 250 ms z filtrem wykładniczym (alpha = 0.3).

### 2.5 Przyciski sterujące (BS-33B monostabilne)

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
| **21** | TFT LED | PWM | Podświetlenie LEDC kanał 0 |
| **26–37** | **ZAJĘTE** | — | **Flash + Octal PSRAM — nie podłączać!** |
| **38** | Przycisk START | INPUT_PULLUP | Start / Pauza / Wznów |
| **39** | Przycisk STOP | INPUT_PULLUP | Stop / Menu (1 s) |
| **40** | Przycisk SELECT | INPUT_PULLUP | Odwróć (P-3a/P-3b) / Menu nawigacja |
| **41** | Przekaźnik P1 | OUTPUT | Pistolet oś L, 12 cm |
| **42** | Przekaźnik P2 | OUTPUT | Pistolet oś C, 12 cm |

---

## 4. Mapowanie funkcji na piny

### 4.1 Funkcje przycisków w kontekście ekranów

| Funkcja | Element | GPIO | Ekran | Uwagi |
|---------|---------|------|-------|-------|
| Start malowania | START | 38 | HOME | Krótkie naciśnięcie |
| Wybór trybu pracy | START | 38 | HOME | Długie naciśnięcie (1 s) |
| Start od przerwy | GAP | **7** | HOME | Krótkie naciśnięcie |
| Pauza / Wznowienie | START | 38 | PAINTING | Krótkie naciśnięcie (AUTO) |
| Kolejna linia (SEMI) | START | 38 | PAINTING | Krótkie naciśnięcie (SEMI, po kreski) |
| Pistolety ON (RĘCZNY) | START | 38 | PAINTING | Trzymanie (tryb RĘCZNY) |
| Przełącz tryb | START | 38 | MODE SELECT | Krótkie naciśnięcie |
| Zatwierdź tryb | START | 38 | MODE SELECT | Długie naciśnięcie (1 s) |
| Zatrzymanie | STOP | 39 | PAINTING | Krótkie naciśnięcie |
| Odwrócenie wzorca | SELEKTOR | 40 | HOME / PAINTING | Krótkie naciśnięcie, **tylko P-3a/P-3b** |
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

### 6.5 Podłączenie wyświetlacza i karty SD (wspólna magistrala SPI)

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

### 6.6 Podłączenie zegara RTC DS1307

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
| Regulator ESP32-S3 | 3.3V | ILI9341, enkoder, karta SD |

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
      │               └──── Enkoder (VCC, opcjonalnie)
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
| **web_server** | web_server.cpp/h | WiFi AP + serwer HTTP + API REST |

### 8.2 Architektura dual-core (v2.6.0)

```
╔══════════════════════════════════╗  ╔══════════════════════════════╗
║         CORE 1 (loop)           ║  ║      CORE 0 (FreeRTOS)      ║
║                                  ║  ║                              ║
║  0. esp_task_wdt_reset()         ║  ║  webTaskFunc() {             ║
║  1. buttons.update()             ║  ║      for(;;) {               ║
║  2. menu.handleEvent()           ║  ║          server.handleClient()║
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
       │ ┌──────┐                  │    │
       │ │PAUSED│──────────────────┘    │
       │ └──┬───┘                       │
       │    │                           │
       │    │ START (wznów)             │
       │    └──────► PAINTING ──────────┘
       │
       │    STOP (1s na HOME)       START (1s na HOME)
       ▼                              ▼
  ┌──────────┐                ┌──────────────┐
  │ SERVICE  │                │ MODE SELECT  │
  │  MENU    │                │ AUTO/SEMI/   │
  └──────────┘                │ RĘCZNY       │
   → Kalibracja/Pomiar/       └──────────────┘
     Raporty/Czyszczenie        → Zapisuje do NVS
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

    4. Zaktualizuj statystyki (dystans, powierzchnia)
```

### 8.6 API REST

| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/` | GET | Strona HTML panelu sterowania |
| `/api/status` | GET | JSON ze stanem systemu (+ anomalia pistoletów) |
| `/api/stats` | GET | Statystyki lifetime + sesja + per-gun |
| `/api/reports` | GET | Lista plików raportów CSV z karty SD |
| `/api/control` | POST | Sterowanie maszyną (action=start\|pause\|stop\|start_from_gap\|set_pattern\|toggle_reverse\|set_mode\|semi_next_line\|save_custom_pattern\|cal_start\|cal_finish\|set_max_speed) |

Szczegółowa dokumentacja API → [API_WWW.md](API_WWW.md)

---

## 9. Parametry konfiguracyjne

### 9.1 Parametry zdefiniowane w config.h

| Parametr | Wartość | Opis |
|----------|---------|------|
| FW_VERSION | "2.9.0" | Wersja firmware |
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

### 10.6 Aktualizacja firmware

- Podłącz ESP32-S3 przez USB-C
- W PlatformIO: `pio run --target upload`
- Alternatywnie: OTA przez WiFi (wymaga dodatkowej implementacji)

---

*TrassarV3 — Dokumentacja techniczna v2.11.0*
*ESP32-S3 N16R8 | ILI9341 320×240 | 6 pistoletów | 16 wzorców | 3 tryby pracy | WiFi AP*
