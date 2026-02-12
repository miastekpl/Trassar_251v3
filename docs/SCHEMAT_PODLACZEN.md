# TrassarV3 - Schemat podłączeń v2.2.0

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

### Czytnik kart SD (zintegrowany w module wyświetlacza)

| Pin SD | Pin ESP32-S3 | GPIO | Opis |
|--------|-------------|------|------|
| SD_CS | GPIO 16 | 16 | Chip Select karty SD |
| SD_MOSI | GPIO 11 | 11 | SPI MOSI (wspólny z TFT) |
| SD_MISO | GPIO 13 | 13 | SPI MISO (wspólny z TFT) |
| SD_SCK | GPIO 12 | 12 | SPI Clock (wspólny z TFT) |

> **Uwaga:** Karta SD współdzieli magistralę SPI (HSPI) z wyświetlaczem. Każde urządzenie ma osobny pin CS - TFT na GPIO 10, SD na GPIO 16. Biblioteka SD automatycznie przełącza CS. Karta SD służy do zapisu raportów malowania w formacie CSV.

### Zegar RTC DS1307 (I2C)

| Pin DS1307 | Pin ESP32-S3 | GPIO | Opis |
|------------|-------------|------|------|
| VCC | 5V | - | Zasilanie 5V |
| GND | GND | - | Masa |
| SDA | GPIO 17 | 17 | I2C Data |
| SCL | GPIO 18 | 18 | I2C Clock |

> **Uwaga:** Moduł DS1307 wymaga zasilania 5V. Linie I2C mają wbudowane rezystory pull-up na module.

### Enkoder obrotowy (wyłącznie pomiar dystansu/prędkości)

| Pin enkodera | Pin ESP32-S3 | GPIO | Opis |
|-------------|-------------|------|------|
| GND | GND | - | Masa |
| CLK (A) | GPIO 5 | 5 | Sygnał A (z przerwaniem ISR) |
| DT (B) | GPIO 6 | 6 | Sygnał B |
| + (VCC) | 3V3 | - | Zasilanie (jeśli wymagane) |

> **Uwaga:** Piny CLK i DT mają włączone wewnętrzne rezystory pull-up ESP32-S3. Enkoder służy **wyłącznie** do pomiaru dystansu i prędkości (ISR na CLK/CHANGE). **Nie jest używany do nawigacji menu.**

### Przycisk "Start od przerwy" (dedykowany)

| Pin | Pin ESP32-S3 | GPIO | Opis |
|-----|-------------|------|------|
| Styk 1 | GPIO 7 | **7** | **Przycisk "Start od przerwy"** |
| Styk 2 | GND | - | Masa |

> **Przycisk "Start od przerwy"** na GPIO 7 jest dedykowanym, osobnym przyciskiem fizycznym. Można użyć wbudowanego przycisku enkodera (SW) lub zamontować oddzielny przycisk monostabilny. Podłączenie identyczne jak BS-33B: jeden styk do GPIO 7, drugi do GND, wewnętrzny pull-up aktywny.

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

## 2. Mapowanie funkcji na piny

| Funkcja | Przycisk / Element | GPIO | Uwagi |
|---------|-------------------|------|-------|
| **Start malowania** | START (BS-33B) | 38 | Krótkie naciśnięcie na ekranie HOME |
| **Start od przerwy** | **Dedykowany przycisk** | **7** | **Krótkie naciśnięcie na ekranie HOME** |
| **Start od przerwy (WWW)** | Panel WWW | - | Przycisk "START OD PRZERWY" w przeglądarce |
| **Pauza / Wznowienie** | START (BS-33B) | 38 | Krótkie naciśnięcie podczas malowania |
| **Zatrzymanie** | STOP (BS-33B) | 39 | Krótkie naciśnięcie podczas malowania |
| **Menu serwisowe** | STOP (BS-33B) | 39 | Długie naciśnięcie (1s) na HOME |
| **Zmiana wzorca** | SELEKTOR (BS-33B) | 40 | Krótkie naciśnięcie na HOME lub malowania |
| **Odwrócenie wzorca** | SELEKTOR (BS-33B) | 40 | Długie naciśnięcie (1s) |
| **Nawigacja menu dalej** | SELEKTOR (BS-33B) | 40 | Krótkie naciśnięcie w menu |
| **Nawigacja menu cofnij** | STOP (BS-33B) | 39 | Krótkie naciśnięcie w menu |
| **Wejście w opcję menu** | SELEKTOR (BS-33B) | 40 | Długie naciśnięcie (1s) w menu |
| **Czyszczenie dysz** | START (BS-33B) | 38 | Trzymaj w trybie czyszczenia dysz |
| **Pomiar dystansu** | Enkoder CLK/DT | 5, 6 | Tylko pomiar (ISR), brak funkcji UI |

## 3. Schemat blokowy

> 4 przyciski sterowania: START (38), STOP (39), SELEKTOR (40), GAP/PRZERWA (7)

```
                          ┌──────────────────────────────┐
                          │       ESP32-S3 N16R8          │
                          │                               │
    ┌─────────┐    SPI    │  GPIO 10 ← CS  (TFT)         │
    │ ILI9341 │◄─────────│  GPIO  9 ← DC                │
    │ 2.8"TFT │  (HSPI)  │  GPIO 14 ← RST               │
    │ Display  │           │  GPIO 11 ← MOSI              │
    │         │           │  GPIO 13 → MISO               │
    │         │           │  GPIO 12 ← SCK                │
    │         │           │  GPIO 21 ← LED (PWM)          │
    │ Touch   │           │  GPIO 15 ← T_CS               │
    │         │           │                               │
    │ SD Card │           │  GPIO 16 ← SD_CS              │
    └─────────┘           │                               │
                          │                               │
    ┌─────────┐    I2C    │  GPIO 17 ↔ SDA                │
    │ DS1307  │◄─────────│  GPIO 18 ← SCL                │
    │ RTC     │           │                               │
    └─────────┘           │                               │
                          │                               │
    ┌─────────┐  Digital  │  GPIO  5 → CLK (ISR CHANGE)   │
    │ Enkoder │──────────│  GPIO  6 → DT                 │
    │ obrotowy│           │                               │
    └─────────┘           │                               │
                          │                               │
    [START/PAUZA]─── GND ─│─ GPIO 38 (pull-up)            │
    [STOP]──────── GND ─│─ GPIO 39 (pull-up)            │
    [SELEKTOR]──── GND ─│─ GPIO 40 (pull-up)            │
    [OD PRZERWY]── GND ─│─ GPIO  7 (pull-up)            │
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

## 4. Schemat podłączenia przycisków

```
    Przyciski BS-33B:
    ESP32-S3 GPIO 38/39/40
         │
         │  (wewnętrzny pull-up do 3.3V)
         │
         ├──── Styk 1 przycisku BS-33B
         │
         │     Styk 2 przycisku BS-33B
         │         │
        GND ──────┘

    Przycisk "Start od przerwy":
    ESP32-S3 GPIO 7
         │
         │  (wewnętrzny pull-up do 3.3V)
         │
         ├──── Styk 1 (wbudowany SW enkodera lub osobny przycisk)
         │
         │     Styk 2
         │         │
        GND ──────┘
```

Wszystkie przyciski łączą GPIO do GND. W stanie spoczynku pin jest w stanie HIGH (pull-up).
Naciśnięcie = stan LOW.

## 5. Schemat podłączenia enkodera (tylko pomiar dystansu)

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
    │  SW  ├──── GPIO 7  ← przycisk "START OD PRZERWY"
    │      │
    │  GND ├──── GND
    └──────┘
```

> Pin SW enkodera (GPIO 7) jest wykorzystywany jako **dedykowany przycisk "Start od przerwy"**. Obroty enkodera (CLK/DT) służą wyłącznie do pomiaru dystansu i prędkości. Nawigacja po menu odbywa się przez 3 przyciski BS-33B.

## 6. Schemat podłączenia przekaźników

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

## 7. Schemat podłączenia karty SD

```
    Moduł ILI9341 2.8" (wbudowany slot SD)
    ┌──────────────────────┐
    │                      │
    │  SD_CS  ─────────────├──── GPIO 16 (CS karty SD)
    │  SD_MOSI ────────────├──── GPIO 11 (wspólny z TFT MOSI)
    │  SD_MISO ────────────├──── GPIO 13 (wspólny z TFT MISO)
    │  SD_SCK  ────────────├──── GPIO 12 (wspólny z TFT SCK)
    │                      │
    └──────────────────────┘

    Magistrala SPI współdzielona z wyświetlaczem TFT.
    Przełączanie urządzeń przez osobne linie CS:
      - GPIO 10 = TFT CS
      - GPIO 16 = SD CS
```

> **Format danych:** Raporty zapisywane w `/reports/RRRRMMDD.csv` (np. `/reports/20250115.csv`).
> Każdy wiersz: `data,godzina,wzorzec,dystans_m,powierzchnia_m2`.

## 8. Zasilanie

| Źródło | Napięcie | Odbiorcy |
|--------|----------|----------|
| USB-C ESP32-S3 | 5V | ESP32-S3, DS1307, Moduły przekaźnikowe |
| Regulator ESP32 | 3.3V | ILI9341, Enkoder, Karta SD |

> **Ważne:** Wyświetlacz ILI9341 zasilany jest z pinu 3V3 płytki ESP32-S3. Moduł DS1307 i moduły przekaźnikowe wymagają 5V - podłączyć do pinu 5V (VBUS) płytki.
>
> **Uwaga o prądzie:** Przy 6 przekaźnikach aktywnych jednocześnie pobór prądu może być znaczny. Przy większych obciążeniach rozważ zewnętrzne zasilanie 5V dla modułów przekaźnikowych.

## 9. Mapa GPIO ESP32-S3 N16R8

| GPIO | Funkcja | Uwagi |
|------|---------|-------|
| 1 | Przekaźnik P3 | OUTPUT |
| 2 | Przekaźnik P4 | OUTPUT |
| 3 | Przekaźnik P5 | OUTPUT |
| 4 | Przekaźnik P6 | OUTPUT |
| 5 | Enkoder CLK | INPUT_PULLUP, ISR |
| 6 | Enkoder DT | INPUT_PULLUP |
| **7** | **Przycisk "Start od przerwy"** | **INPUT_PULLUP** |
| 9 | TFT DC | OUTPUT |
| 10 | TFT CS | OUTPUT |
| 11 | TFT MOSI / SD MOSI | SPI (HSPI) |
| 12 | TFT SCK / SD SCK | SPI (HSPI) |
| 13 | TFT MISO / SD MISO | SPI (HSPI) |
| 14 | TFT RST | OUTPUT |
| 15 | Touch CS | OUTPUT |
| 16 | **SD Card CS** | **OUTPUT** |
| 17 | RTC SDA | I2C |
| 18 | RTC SCL | I2C |
| 21 | TFT LED | PWM LEDC |
| 26-37 | **ZAJĘTE** | Flash + Octal PSRAM |
| 38 | Przycisk START | INPUT_PULLUP |
| 39 | Przycisk STOP | INPUT_PULLUP |
| 40 | Przycisk SELECT | INPUT_PULLUP |
| 41 | Przekaźnik P1 | OUTPUT |
| 42 | Przekaźnik P2 | OUTPUT |

## 10. Uwagi montażowe

1. Wszystkie połączenia SPI powinny być jak najkrótsze (maks. 15-20cm)
2. Przy dłuższych przewodach enkoder może wymagać kondensatorów filtrujących (100nF) między CLK/DT a GND
3. Moduł DS1307 posiada baterię CR2032 - zapewnia podtrzymanie czasu po odłączeniu zasilania
4. Przyciski BS-33B nie wymagają zewnętrznych rezystorów - wykorzystywane są wewnętrzne pull-up ESP32-S3
5. **WAŻNE:** Na module ESP32-S3 N16R8 piny GPIO 26-37 są zajęte przez Flash i Octal PSRAM - NIE podłączać do nich niczego!
6. Przewody do przekaźników mogą być dłuższe (do 50cm) - sygnał cyfrowy 3.3V jest odporny na zakłócenia
7. Moduły przekaźnikowe powinny mieć diody zabezpieczające (wbudowane w większości modułów)
8. Enkoder powinien być zamontowany na kole pomiarowym z dobrym stykiem z podłożem
9. **Karta SD** powinna być sformatowana w systemie FAT32. Moduł SD współdzieli magistralę SPI z wyświetlaczem - nie wymaga dodatkowego okablowania poza jednym przewodem CS (GPIO 16)
10. Przy problemach z kartą SD sprawdź czy pin GPIO 16 nie jest używany przez inne urządzenie
11. **Przycisk "Start od przerwy"** może być wbudowanym przyciskiem enkodera (SW) lub osobnym przyciskiem monostabilnym podłączonym do GPIO 7 i GND
