# TrassarV3 - Schemat podłączeń v2.0.0

## 1. Tabela podłączeń pinów ESP32-S3 N16R8

### Wyświetlacz ILI9341 2.8" (SPI - HSPI)

| Pin ILI9341 | Pin ESP32-S3 | GPIO | Opis |
|-------------|-------------|------|------|
| VCC | 3V3 | - | Zasilanie 3.3V |
| GND | GND | - | Masa |
| CS | GPIO 10 | 10 | Chip Select |
| RESET | GPIO 14 | 14 | Reset wyświetlacza |
| DC/RS | GPIO 9 | 9 | Data/Command |
| SDI (MOSI) | GPIO 11 | 11 | SPI Master Out |
| SCK | GPIO 12 | 12 | SPI Clock |
| LED | GPIO 21 | 21 | Podświetlenie (PWM LEDC) |
| SDO (MISO) | GPIO 13 | 13 | SPI Master In |
| T_CLK | GPIO 12 | 12 | Touch SPI Clock (wspólny z SCK) |
| T_CS | GPIO 15 | 15 | Touch Chip Select |
| T_DIN | GPIO 11 | 11 | Touch MOSI (wspólny) |
| T_DO | GPIO 13 | 13 | Touch MISO (wspólny) |
| T_IRQ | - | - | Nie podłączony (opcjonalny) |

> **Uwaga:** TFT używa portu HSPI (SPI3) odizolowanego od PSRAM. Częstotliwość SPI: 27 MHz.

### Zegar RTC DS1307 (I2C)

| Pin DS1307 | Pin ESP32-S3 | GPIO | Opis |
|------------|-------------|------|------|
| VCC | 5V | - | Zasilanie 5V |
| GND | GND | - | Masa |
| SDA | GPIO 17 | 17 | I2C Data |
| SCL | GPIO 18 | 18 | I2C Clock |

> **Uwaga:** Moduł DS1307 wymaga zasilania 5V. Linie I2C mają wbudowane rezystory pull-up na module.

### Enkoder obrotowy (dystans + nawigacja)

| Pin enkodera | Pin ESP32-S3 | GPIO | Opis |
|-------------|-------------|------|------|
| GND | GND | - | Masa |
| CLK (A) | GPIO 5 | 5 | Sygnał A (z przerwaniem ISR) |
| DT (B) | GPIO 6 | 6 | Sygnał B |
| SW | GPIO 7 | 7 | Przycisk |
| + (VCC) | 3V3 | - | Zasilanie (jeśli wymagane) |

> **Uwaga:** Piny CLK, DT i SW mają włączone wewnętrzne rezystory pull-up ESP32-S3. Enkoder mierzy dystans (ISR na CLK/CHANGE) i jednocześnie służy do nawigacji menu.

### Przyciski BS-33B (monostabilne)

| Przycisk | Pin ESP32-S3 | GPIO | Funkcja |
|----------|-------------|------|---------|
| Start/Pauza | GPIO 38 | 38 | Uruchom / Pauzuj / Wznów malowanie |
| Stop | GPIO 39 | 39 | Zatrzymaj / Wejdź w menu (1s) |
| Selektor | GPIO 40 | 40 | Nawigacja / Wejdź w opcję (1s) / Odwróć wzorzec (1s) |

> **UWAGA:** GPIO 33-37 są zajęte przez Octal PSRAM modułu N16R8! Nie wolno ich używać!
>
> **Podłączenie przycisków:** Jeden styk do GPIO, drugi do GND. Wewnętrzne rezystory pull-up są aktywowane programowo.

### Przekaźniki pistoletów (6 szt.)

| Pistolet | Pin ESP32-S3 | GPIO | Szerokość | Zastosowanie |
|----------|-------------|------|-----------|-------------|
| P1 | GPIO 41 | 41 | 12 cm | Oś jezdni - lewy |
| P2 | GPIO 42 | 42 | 12 cm | Oś jezdni - środek |
| P3 | GPIO 1 | 1 | 12 cm | Oś jezdni - prawy |
| P4 | GPIO 2 | 2 | 24 cm | Oś jezdni - szeroki |
| P5 | GPIO 3 | 3 | 12 cm | Krawędź - wąska |
| P6 | GPIO 4 | 4 | 24 cm | Krawędź - szeroka |

> **Podłączenie przekaźników:** GPIO → IN modułu przekaźnikowego. Logika: HIGH = włączony, LOW = wyłączony. Moduły przekaźnikowe zasilane z 5V. Każdy pistolet sterowany osobnym przekaźnikiem.

## 2. Schemat blokowy

```
                          ┌──────────────────────────────┐
                          │       ESP32-S3 N16R8          │
                          │                               │
    ┌─────────┐    SPI    │  GPIO 10 ← CS                │
    │ ILI9341 │◄─────────│  GPIO  9 ← DC                │
    │ 2.8"TFT │  (HSPI)  │  GPIO 14 ← RST               │
    │ Display  │           │  GPIO 11 ← MOSI              │
    │         │           │  GPIO 13 → MISO               │
    │         │           │  GPIO 12 ← SCK                │
    │         │           │  GPIO 21 ← LED (PWM)          │
    │ Touch   │           │  GPIO 15 ← T_CS               │
    └─────────┘           │                               │
                          │                               │
    ┌─────────┐    I2C    │  GPIO 17 ↔ SDA                │
    │ DS1307  │◄─────────│  GPIO 18 ← SCL                │
    │ RTC     │           │                               │
    └─────────┘           │                               │
                          │                               │
    ┌─────────┐  Digital  │  GPIO  5 → CLK (ISR CHANGE)   │
    │ Enkoder │──────────│  GPIO  6 → DT                 │
    │ obrotowy│           │  GPIO  7 → SW                 │
    └─────────┘           │                               │
                          │                               │
    [START/PAUZA]─── GND ─│─ GPIO 38 (pull-up)            │
    [STOP]──────── GND ─│─ GPIO 39 (pull-up)            │
    [SELEKTOR]──── GND ─│─ GPIO 40 (pull-up)            │
                          │                               │
                          │  --- PRZEKAŹNIKI ---           │
    ┌──────────┐          │                               │
    │ P1 (12cm)│◄─────────│─ GPIO 41                      │
    │ P2 (12cm)│◄─────────│─ GPIO 42                      │
    │ P3 (12cm)│◄─────────│─ GPIO  1                      │
    │ P4 (24cm)│◄─────────│─ GPIO  2                      │
    │ P5 (12cm)│◄─────────│─ GPIO  3                      │
    │ P6 (24cm)│◄─────────│─ GPIO  4                      │
    └──────────┘          │                               │
                          │  WiFi AP: TrassarV3            │
                          │  IP: 192.168.4.1               │
                          └──────────────────────────────┘
```

## 3. Schemat podłączenia przycisków BS-33B

```
    ESP32-S3 GPIO 38/39/40
         │
         │  (wewnętrzny pull-up do 3.3V)
         │
         ├──── Styk 1 przycisku BS-33B
         │
         │     Styk 2 przycisku BS-33B
         │         │
        GND ──────┘
```

Przycisk łączy GPIO do GND. W stanie spoczynku pin jest w stanie HIGH (pull-up).
Naciśnięcie = stan LOW.

## 4. Schemat podłączenia enkodera

```
          3V3 (opcjonalne)
           │
    ┌──────┤
    │  VCC │
    │      │
    │  CLK ├──── GPIO 5  (pull-up, przerwanie ISR CHANGE)
    │      │
    │  DT  ├──── GPIO 6  (pull-up)
    │      │
    │  SW  ├──── GPIO 7  (pull-up)
    │      │
    │  GND ├──── GND
    └──────┘
```

## 5. Schemat podłączenia przekaźników

```
    ESP32-S3                    Moduł przekaźnikowy 5V
    ┌─────────┐                 ┌─────────────────────┐
    │         │                 │                     │
    │ GPIO 41 ├─────────────────┤ IN1  [P1 - oś L]   │
    │ GPIO 42 ├─────────────────┤ IN2  [P2 - oś C]   │
    │ GPIO  1 ├─────────────────┤ IN3  [P3 - oś R]   │
    │ GPIO  2 ├─────────────────┤ IN4  [P4 - oś 24]  │
    │ GPIO  3 ├─────────────────┤ IN5  [P5 - kraw 12]│
    │ GPIO  4 ├─────────────────┤ IN6  [P6 - kraw 24]│
    │         │                 │                     │
    │    5V   ├─────────────────┤ VCC                 │
    │    GND  ├─────────────────┤ GND                 │
    └─────────┘                 └─────────────────────┘

    Wyjścia NO (Normally Open) przekaźników
    podłączone do zaworów pistoletów natryskowych.
```

> **Logika:** HIGH na GPIO = przekaźnik włączony = pistolet maluje.

## 6. Zasilanie

| Źródło | Napięcie | Odbiorcy |
|--------|----------|----------|
| USB-C ESP32-S3 | 5V | ESP32-S3, DS1307, Moduły przekaźnikowe |
| Regulator ESP32 | 3.3V | ILI9341, Enkoder |

> **Ważne:** Wyświetlacz ILI9341 zasilany jest z pinu 3V3 płytki ESP32-S3. Moduł DS1307 i moduły przekaźnikowe wymagają 5V - podłączyć do pinu 5V (VBUS) płytki.
>
> **Uwaga o prądzie:** Przy 6 przekaźnikach aktywnych jednocześnie pobór prądu może być znaczny. Przy większych obciążeniach rozważ zewnętrzne zasilanie 5V dla modułów przekaźnikowych.

## 7. Mapa GPIO ESP32-S3 N16R8

| GPIO | Funkcja | Uwagi |
|------|---------|-------|
| 1 | Przekaźnik P3 | OUTPUT |
| 2 | Przekaźnik P4 | OUTPUT |
| 3 | Przekaźnik P5 | OUTPUT |
| 4 | Przekaźnik P6 | OUTPUT |
| 5 | Enkoder CLK | INPUT_PULLUP, ISR |
| 6 | Enkoder DT | INPUT_PULLUP |
| 7 | Enkoder SW | INPUT_PULLUP |
| 9 | TFT DC | OUTPUT |
| 10 | TFT CS | OUTPUT |
| 11 | TFT MOSI | SPI |
| 12 | TFT SCK | SPI |
| 13 | TFT MISO | SPI |
| 14 | TFT RST | OUTPUT |
| 15 | Touch CS | OUTPUT |
| 17 | RTC SDA | I2C |
| 18 | RTC SCL | I2C |
| 21 | TFT LED | PWM LEDC |
| 26-37 | **ZAJĘTE** | Flash + Octal PSRAM |
| 38 | Przycisk START | INPUT_PULLUP |
| 39 | Przycisk STOP | INPUT_PULLUP |
| 40 | Przycisk SELECT | INPUT_PULLUP |
| 41 | Przekaźnik P1 | OUTPUT |
| 42 | Przekaźnik P2 | OUTPUT |

## 8. Uwagi montażowe

1. Wszystkie połączenia SPI powinny być jak najkrótsze (maks. 15-20cm)
2. Przy dłuższych przewodach enkoder może wymagać kondensatorów filtrujących (100nF) między CLK/DT a GND
3. Moduł DS1307 posiada baterię CR2032 - zapewnia podtrzymanie czasu po odłączeniu zasilania
4. Przyciski BS-33B nie wymagają zewnętrznych rezystorów - wykorzystywane są wewnętrzne pull-up ESP32-S3
5. **WAŻNE:** Na module ESP32-S3 N16R8 piny GPIO 26-37 są zajęte przez Flash i Octal PSRAM - NIE podłączać do nich niczego!
6. Przewody do przekaźników mogą być dłuższe (do 50cm) - sygnał cyfrowy 3.3V jest odporny na zakłócenia
7. Moduły przekaźnikowe powinny mieć diody zabezpieczające (wbudowane w większości modułów)
8. Enkoder powinien być zamontowany na kole pomiarowym z dobrym stykiem z podłożem
