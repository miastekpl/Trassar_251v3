# TrassarV3 - Schemat podłączeń

## 1. Tabela podłączeń pinów ESP32-S3 N16R8

### Wyświetlacz ILI9341 2.8" (SPI)

| Pin ILI9341 | Pin ESP32-S3 | GPIO | Opis |
|-------------|-------------|------|------|
| VCC | 3V3 | - | Zasilanie 3.3V |
| GND | GND | - | Masa |
| CS | GPIO 10 | 10 | Chip Select |
| RESET | GPIO 14 | 14 | Reset wyświetlacza |
| DC/RS | GPIO 9 | 9 | Data/Command |
| SDI (MOSI) | GPIO 11 | 11 | SPI Master Out |
| SCK | GPIO 12 | 12 | SPI Clock |
| LED | GPIO 21 | 21 | Podświetlenie (PWM) |
| SDO (MISO) | GPIO 13 | 13 | SPI Master In |
| T_CLK | GPIO 12 | 12 | Touch SPI Clock (wspólny z SCK) |
| T_CS | GPIO 15 | 15 | Touch Chip Select |
| T_DIN | GPIO 11 | 11 | Touch MOSI (wspólny) |
| T_DO | GPIO 13 | 13 | Touch MISO (wspólny) |
| T_IRQ | - | - | Nie podłączony (opcjonalny) |

### Zegar RTC DS1307 (I2C)

| Pin DS1307 | Pin ESP32-S3 | GPIO | Opis |
|------------|-------------|------|------|
| VCC | 5V | - | Zasilanie 5V |
| GND | GND | - | Masa |
| SDA | GPIO 17 | 17 | I2C Data |
| SCL | GPIO 18 | 18 | I2C Clock |

> **Uwaga:** Moduł DS1307 wymaga zasilania 5V. Linie I2C mają wbudowane rezystory pull-up na module.

### Enkoder obrotowy

| Pin enkodera | Pin ESP32-S3 | GPIO | Opis |
|-------------|-------------|------|------|
| GND | GND | - | Masa |
| CLK (A) | GPIO 5 | 5 | Sygnał A (z przerwaniem) |
| DT (B) | GPIO 6 | 6 | Sygnał B |
| SW | GPIO 7 | 7 | Przycisk (opcjonalny) |
| + (VCC) | 3V3 | - | Zasilanie (jeśli wymagane) |

> **Uwaga:** Piny CLK, DT i SW mają włączone wewnętrzne rezystory pull-up ESP32-S3. Jeśli enkoder ma własne rezystory, podłączenie VCC nie jest konieczne.

### Przyciski BS-33B (monostabilne)

| Przycisk | Pin ESP32-S3 | GPIO | Funkcja |
|----------|-------------|------|---------|
| Start/Pauza | GPIO 35 | 35 | Uruchom/Pauzuj malowanie |
| Stop | GPIO 36 | 36 | Zatrzymaj / Wejdź w menu (1s) |
| Selektor | GPIO 37 | 37 | Nawigacja / Wejdź w opcję (1s) |

> **Podłączenie przycisków:** Jeden styk do GPIO, drugi do GND. Wewnętrzne rezystory pull-up są aktywowane programowo.

## 2. Schemat blokowy

```
                          ┌─────────────────────────┐
                          │     ESP32-S3 N16R8       │
                          │                          │
    ┌─────────┐    SPI    │  GPIO 10 ← CS           │
    │ ILI9341 │◄─────────│  GPIO  9 ← DC           │
    │ 2.8"TFT │           │  GPIO 14 ← RST          │
    │ Display  │           │  GPIO 11 ← MOSI         │
    │         │           │  GPIO 13 → MISO          │
    │         │           │  GPIO 12 ← SCK           │
    │         │           │  GPIO 21 ← LED (PWM)     │
    │ Touch   │           │  GPIO 15 ← T_CS          │
    └─────────┘           │                          │
                          │                          │
    ┌─────────┐    I2C    │  GPIO 17 ↔ SDA           │
    │ DS1307  │◄─────────│  GPIO 18 ← SCL           │
    │ RTC     │           │                          │
    └─────────┘           │                          │
                          │                          │
    ┌─────────┐  Digital  │  GPIO  5 → CLK (IRQ)     │
    │ Enkoder │──────────│  GPIO  6 → DT            │
    │ obrotowy│           │  GPIO  7 → SW            │
    └─────────┘           │                          │
                          │                          │
    [START/PAUZA]─── GND ─│─ GPIO 35 (pull-up)       │
    [STOP]──────── GND ─│─ GPIO 36 (pull-up)       │
    [SELEKTOR]──── GND ─│─ GPIO 37 (pull-up)       │
                          │                          │
                          │  WiFi AP: TrassarV3      │
                          │  IP: 192.168.4.1         │
                          └─────────────────────────┘
```

## 3. Schemat podłączenia przycisków BS-33B

```
    ESP32-S3 GPIO 35/36/37
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
    │  CLK ├──── GPIO 5  (pull-up, przerwanie)
    │      │
    │  DT  ├──── GPIO 6  (pull-up)
    │      │
    │  SW  ├──── GPIO 7  (pull-up)
    │      │
    │  GND ├──── GND
    └──────┘
```

## 5. Zasilanie

| Źródło | Napięcie | Odbiorcy |
|--------|----------|----------|
| USB-C ESP32-S3 | 5V | ESP32-S3, DS1307 |
| Regulator ESP32 | 3.3V | ILI9341, Enkoder |

> **Ważne:** Wyświetlacz ILI9341 zasilany jest z pinu 3V3 płytki ESP32-S3. Moduł DS1307 wymaga 5V - podłączyć do pinu 5V (VBUS) płytki.

## 6. Uwagi montażowe

1. Wszystkie połączenia SPI powinny być jak najkrótsze (maks. 15-20cm)
2. Przy dłuższych przewodach enkoder może wymagać kondensatorów filtrujących (100nF) między CLK/DT a GND
3. Moduł DS1307 posiada baterię CR2032 - zapewnia podtrzymanie czasu po odłączeniu zasilania
4. Przyciski BS-33B nie wymagają zewnętrznych rezystorów - wykorzystywane są wewnętrzne pull-up ESP32-S3
