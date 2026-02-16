# Changelog - TrassarV3

Wszystkie istotne zmiany w projekcie są dokumentowane w tym pliku.

Format oparty na [Keep a Changelog](https://keepachangelog.com/pl/1.0.0/).
Wersjonowanie zgodne z [Semantic Versioning](https://semver.org/lang/pl/).

---

## [2.4.0] - 2025-02-16

### Dodano - Bezpieczeństwo i sygnalizacja dźwiękowa

#### Watchdog timer (3 s)
- Sprzętowy watchdog ESP32 (Task WDT) z timeoutem 3 sekund i automatycznym resetem
- `esp_task_wdt_init(3, true)` i `esp_task_wdt_add(NULL)` w `setup()`
- `esp_task_wdt_reset()` na początku każdego cyklu `loop()`
- Zabezpieczenie: jeśli `loop()` się zawiesi, ESP32 zrestartuje się po 3 s — piny GPIO wracają do LOW, pistolety się zamykają

#### Gun keepalive (300 ms)
- Niezależna warstwa bezpieczeństwa — awaryjne wyłączenie pistoletów jeśli `paintEngine.update()` nie zostanie wywołane przez 300 ms
- Metoda `PaintingEngine::checkGunKeepAlive()` wywoływana w `loop()` niezależnie od `update()`
- Pole `lastGunUpdateMs` aktualizowane w każdym cyklu sterowania pistoletami
- Zabezpiecza przed scenariuszem: loop działa (watchdog karmiony), ale painting engine nie steruje pistoletami

#### Alarm przekroczenia prędkości
- Próg domyślny: 15 km/h (`DEFAULT_MAX_PAINT_SPEED_KMH`)
- Próg konfigurowalny z panelu WWW (suwak 5–30 km/h, krok 0.5), zapisywany trwale do NVS
- Wyświetlacz: prędkość miga na czerwono (cykl 300 ms) + etykieta "km/h" też miga
- Buzzer: trojkowy alarm 3 kHz powtarzany co 2 s
- Niska prędkość (<3 km/h podczas malowania): żółty kolor prędkości, buzzer co 3 s
- Nowe pola API JSON: `maxSpeed`, `overspeed`, `lowSpeed`
- Nowa akcja API: `set_max_speed` (value: 5.0–30.0)

#### Buzzer (sygnalizacja dźwiękowa)
- Nowy moduł: `buzzer.h/cpp`
- Pasywny buzzer na GPIO 8, sterowany LEDC PWM (kanał 1)
- Non-blocking: sekwencje tonów zarządzane w `buzzer.update()` — nie blokuje `loop()`
- Sygnały dźwiękowe:
  - **Start malowania / wznowienie** — krótki beep 2 kHz 100 ms
  - **Pauza / stop malowania** — podwójny beep 2 kHz 80 ms
  - **Niska prędkość (<3 km/h)** — podwójny puls 1.5 kHz (powtarzany co 3 s)
  - **Przekroczenie prędkości maks.** — trojkowy alarm 3 kHz (powtarzany co 2 s)
  - **Błąd (brak SD, RTC niedostępny)** — opadający ton 1000→800→600 Hz
- Metody: `play(BuzzerSignal)`, `beep(freq, duration)`, `stop()`, `update()`

### Zmieniono
- Wersja firmware: 2.3.0 → **2.4.0**
- `painting_engine`: rozbudowana o keepalive, alarmy prędkości i sygnały buzzera
- `display_manager::drawPaintingScreen()`: nowe parametry `overspeed` i `lowSpeed` do sterowania kolorem prędkości
- `storage`: nowe metody `saveMaxSpeed()` / `loadMaxSpeed()`
- `web_server`: nowe pole API `maxSpeed`, `overspeed`, `lowSpeed`; nowa akcja `set_max_speed`; nowa sekcja HTML "Alarm prędkości" z suwakiem
- `main.cpp`: dodano inicjalizację watchdoga, buzzera, gun keepalive; sygnał błędu przy braku RTC/SD

#### Nowe pliki
- `src/buzzer.h` — deklaracja klasy `BuzzerController`
- `src/buzzer.cpp` — implementacja buzzera z sekwencjami tonów LEDC

#### Nowe stałe w config.h
| Stała | Wartość | Opis |
|-------|---------|------|
| `PIN_BUZZER` | 8 | GPIO pinu buzzera |
| `BUZZER_LEDC_CH` | 1 | Kanał LEDC (0 = podświetlenie TFT) |
| `DEFAULT_MAX_PAINT_SPEED_KMH` | 15.0 | Domyślny próg alarmu prędkości [km/h] |
| `WDT_TIMEOUT_SEC` | 3 | Timeout watchdoga [s] |
| `GUN_KEEPALIVE_TIMEOUT_MS` | 300 | Timeout keepalive pistoletów [ms] |

---

## [2.0.0] - 2025-02-12

### Dodano - Komputer pokładowy malowarki pasów drogowych

Kompletna przebudowa firmware z prostego sterownika malarskiego na pełny komputer pokładowy malowarki pasów drogowych zgodny z polskimi normami oznakowania.

#### 6 pistoletów natryskowych (przekaźniki)
- **P1** - oś jezdni lewy (12 cm)
- **P2** - oś jezdni środek (12 cm)
- **P3** - oś jezdni prawy (12 cm)
- **P4** - oś jezdni szeroki (24 cm)
- **P5** - krawędź wąska (12 cm)
- **P6** - krawędź szeroka (24 cm)
- Sterowanie przez moduły przekaźnikowe (aktywne HIGH)
- Piny: P1=GPIO41, P2=GPIO42, P3=GPIO1, P4=GPIO2, P5=GPIO3, P6=GPIO4

#### 15 wzorców malowania (polskie normy)
- **P-1a** - Przerywana długa (6m kreska / 6m przerwa, P2, 12cm)
- **P-1b** - Przerywana krótka (3m / 3m, P2, 12cm)
- **P-1c** - Wydzielająca (3m / 1.5m, P2, 12cm)
- **P-1d** - Prowadząca wąska (1m / 1m, P2, 12cm)
- **P-1e** - Prowadząca szeroka (1m / 1m, P4, 24cm)
- **P-2a** - Ciągła wąska (P2, 12cm)
- **P-2b** - Ciągła szeroka (P4, 24cm)
- **P-3a** - Przekraczalna długa (ciągła + 6m/6m, P1+P3, 12cm) - ODWRACALNA
- **P-3b** - Przekraczalna krótka (ciągła + 3m/3m, P1+P3, 12cm) - ODWRACALNA
- **P-4** - Podwójna ciągła (P1+P3, 12cm)
- **P-6** - Ostrzegawcza (1m / 1m, P5, 12cm)
- **P-7a** - Krawędziowa przerywana szeroka (1m / 2m, P6, 24cm)
- **P-7b** - Krawędziowa ciągła szeroka (P6, 24cm)
- **P-7c** - Krawędziowa przerywana wąska (1m / 2m, P5, 12cm)
- **P-7d** - Krawędziowa ciągła wąska (P5, 12cm)

#### Silnik malowania (painting_engine)
- Maszyna stanów: IDLE → PAINTING → PAUSED → STOPPED
- Logika linii przerywanych: `fmodf(dystans, kreska+przerwa) < kreska`
- Zmiana wzorca w trakcie malowania (on-the-fly)
- Odwracanie P-3a/P-3b (zamiana P1↔P3: ciągła ↔ przerywana)

#### Kalibracja enkodera
- Procedura: Start → przejedź 10 m → Zakończ
- Zapis impulsów/metr do pamięci NVS
- Domyślna wartość: 100 imp/m

#### Obliczanie powierzchni
- Wzór: dystans_pistolet_ON × szerokość_pistoletu [m²]
- Akumulacja osobno per pistolet
- Statystyki sesji i łączne (lifetime w NVS)

#### Nowe moduły firmware
- `patterns.h/cpp` - menadżer 15 wzorców z konfiguracją pistoletów
- `guns.h/cpp` - kontroler 6 przekaźników
- `encoder_distance.h/cpp` - pomiar dystansu, prędkości, kalibracja
- `storage.h/cpp` - trwały zapis do NVS (Preferences)
- `statistics.h/cpp` - statystyki malowania (sesja + lifetime)
- `painting_engine.h/cpp` - silnik malowania z maszyną stanów

#### 9 ekranów interfejsu graficznego
1. **Ekran główny** - wzorzec, prędkość, dystans, kalibracja, czas
2. **Ekran malowania** - status, wzorzec, V, dystans, powierzchnia, czas, wskaźniki pistoletów
3. **Menu główne** - 6 opcji z nawigacją enkoderem
4. **Wybór wzorca** - lista 15 wzorców z paskiem przewijania
5. **Kalibracja** - procedura kalibracji enkodera
6. **Statystyki** - sesja + łączne (dystans, powierzchnia, czas)
7. **Informacje WiFi** - SSID, IP, hasło, klienci
8. **Info systemowe** - firmware, RAM, uptime, platforma
9. **Czas i data** - edycja 6 pól (h/m/s, d/m/r)

#### Panel WWW v2.0.0
- 15 przycisków wzorców pogrupowanych: P-1x, P-2x, P-3x, P-4/P-6, P-7x
- Przycisk odwracania (P-3a/P-3b)
- Wskaźniki 6 pistoletów (ON/OFF z animacją)
- Sekcja kalibracji enkodera
- Ciemny motyw z gradientami i animacjami
- Auto-odświeżanie co 1 sekundę

#### API REST v2.0.0
- Nowe akcje: `set_pattern`, `toggle_reverse`, `cal_start`, `cal_finish`
- Rozszerzony JSON statusu: wzorzec, pistolety, kalibracja, dystans, powierzchnia
- Kompatybilność z istniejącymi akcjami: `start`, `pause`, `stop`

### Zmieniono
- Kompletna przebudowa architektury firmware z 5 do 11 modułów
- Enkoder pełni podwójną rolę: pomiar dystansu (ISR) + nawigacja menu (consumeDelta)
- Display manager obsługuje 9 ekranów (wcześniej 6)
- Button handler deleguje ISR enkodera do modułu encoder_distance
- Menu system z pełną nawigacją dla 9 ekranów

---

## [1.0.1] - 2025-02-12

### Naprawiono
- **KRYTYCZNE:** Zmiana pinów przycisków z GPIO 35/36/37 na GPIO 38/39/40 - piny 33-37 są zajęte przez Octal PSRAM modułu N16R8 (powodowały Guru Meditation Error: StoreProhibited)
- Zamiana `analogWrite` na ESP32 LEDC API (`ledcSetup`/`ledcAttachPin`/`ledcWrite`) dla podświetlenia wyświetlacza - kompatybilność z Arduino-ESP32 2.x
- Usunięcie flagi `ARDUINO_USB_CDC_ON_BOOT` - Serial teraz trafia na UART0 (ten sam port co crash dump)
- Dodanie `USE_HSPI_PORT=1` dla TFT_eSPI - izolacja magistrali SPI3 od potencjalnych konfliktów z PSRAM
- Obniżenie częstotliwości SPI z 40MHz do 27MHz dla większej stabilności
- Dodanie opóźnienia 100ms przed inicjalizacją wyświetlacza (stabilizacja zasilania)
- Zwiększenie opóźnienia startowego do 1000ms (stabilizacja UART)

### Zmieniono
- Przyciski BS-33B: Start/Pauza=GPIO38, Stop=GPIO39, Selektor=GPIO40
- Zaktualizowana dokumentacja podłączeń i instrukcja obsługi

---

## [1.0.0] - 2025-02-12

### Dodano
- Pierwsza wersja firmware TrassarV3
- Obsługa wyświetlacza ILI9341 2.8" (240x320) przez SPI
- Interfejs graficzny z systemem menu:
  - Ekran główny z czasem, datą i statusem maszyny
  - Menu główne z 4 opcjami
  - Ustawienia malowania (prędkość, liczba przejść)
  - Ustawienia czasu i daty (edycja przez enkoder)
  - Informacje WiFi (SSID, IP, hasło, klienci)
  - Informacje systemowe (firmware, RAM, uptime)
  - Ekran malowania z postępem
- Serwer WWW jako Access Point WiFi:
  - SSID: TrassarV3, hasło: 12345678
  - Responsywny panel sterowania HTML
  - API REST (JSON) do statusu i sterowania
  - Auto-odświeżanie statusu co 1 sekundę
- Obsługa 3 przycisków monostabilnych BS-33B:
  - Start/Pauza - uruchamianie i pauzowanie malowania
  - Stop - zatrzymanie procesu, długie naciśnięcie = wejście w menu
  - Selektor - nawigacja po menu (krótko = następna opcja, długo = wejście)
- Obsługa enkodera obrotowego z przyciskiem:
  - Obrót = przewijanie menu / regulacja wartości
  - Przycisk = potwierdzenie wyboru
- Zegar RTC DS1307:
  - Wyświetlanie czasu i daty
  - Edycja czasu i daty przez menu
  - Automatyczne ustawienie czasu kompilacji przy pierwszym uruchomieniu
- Konfiguracja pinów dla ESP32-S3 N16R8
- Pełna dokumentacja: instrukcja obsługi, schemat podłączeń, API WWW

### Platforma
- ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM)
- Framework: Arduino (PlatformIO)
- Biblioteki: TFT_eSPI, RTClib, ArduinoJson
