# Changelog - TrassarV3

Wszystkie istotne zmiany w projekcie są dokumentowane w tym pliku.

Format oparty na [Keep a Changelog](https://keepachangelog.com/pl/1.0.0/).
Wersjonowanie zgodne z [Semantic Versioning](https://semver.org/lang/pl/).

---

## [2.21.0] - 2026-02-24

### Dodano - Konfigurowalna prędkość minimalna

- **Prędkość minimalna** — konfigurowalny próg poniżej którego pistolety wyłączają się automatycznie
  - Nowe pole `minSpeedKmh` w `PaintingEngine` z setterem `setMinSpeed()` i getterem `getMinSpeed()`
  - Zapis/odczyt z NVS: `storage.saveMinSpeed()` / `loadMinSpeed()` (klucz `min_spd`)
  - Panel WWW: drugi suwak (0–10 km/h, krok 0.5) w karcie "Alarm prędkości"
  - Ostrzeżenie niskiej prędkości aktualizowane dynamicznie z wartości minSpeed
  - Status JSON: nowe pole `minSpeed`
  - Nowa akcja API: `set_min_speed` (value: 0.0–10.0)
  - Backup/restore prędkości minimalnej w `nvs_backup.cpp` (klucz `minspd`)
  - Ładowanie z NVS przy starcie w `main.cpp`

### Zmieniono
- `MIN_PAINT_SPEED_KMH` → `DEFAULT_MIN_PAINT_SPEED_KMH` (wartość runtime zamiast stałej kompilacji)
- `PaintingEngine::update()` — warunek prędkości używa `minSpeedKmh` z instancji zamiast `#define`
- Wersja firmware: 2.20.0 → **2.21.0** (zaktualizowana we wszystkich plikach źródłowych)

#### Nowe metody API
| Akcja | Parametry | Opis |
|-------|-----------|------|
| `set_min_speed` | value=0.0–10.0 | Ustawienie progu minimalnej prędkości [km/h] |

---

## [2.20.0] - 2026-02-24

### Dodano - Backup NVS, podwójny watchdog, log zdarzeń

#### 1) Backup NVS na kartę SD (`nvs_backup.h/cpp`)
- Serializacja wszystkich ustawień NVS do `/backup/nvs_backup.json` co 30 min
- Pełny backup JSON: kalibracja, statystyki lifetime, wzorce własne (3 sloty), liczniki strzałów, prędkość max, tryb pracy, tryb przełączania
- Auto-restore przy starcie jeśli NVS pusty/wyczyszczony a backup istnieje na SD
- Flaga `nvs_init` w NVS do detekcji świeżego urządzenia vs dane obecne
- Walidacja wersji backupu (`NVS_DATA_VERSION`) — niekompatybilne backupy ignorowane

#### 2) Podwójny watchdog — Core 0 (dual watchdog)
- Task WebServer na Core 0 subskrybuje Task WDT (`esp_task_wdt_add`)
- Opóźnienie 5 s na starcie tasku (czeka na inicjalizację WDT w `setup()`)
- `esp_task_wdt_reset()` w każdej iteracji task loop
- Oba rdzenie nadzorowane: Core 1 (`loop()`) + Core 0 (WebServer)
- Timeout 3 s → auto-reset przy zawieszeniu dowolnego rdzenia

#### 3) Log zdarzeń na SD (`event_log.h/cpp`)
- Zapis zdarzeń systemowych do `/logs/RRRRMMDD.log` (jeden plik na dzień)
- Format: `HH:MM:SS [KATEGORIA] treść zdarzenia`
- Limit 64 KB na plik dzienny (potem stop do następnego dnia)
- Zintegrowane punkty logowania:
  - `[SYSTEM]` — start firmware z podsumowaniem konfiguracji
  - `[ENGINE]` — start/stop/pauza/resume z danymi sesji, overspeed, keepalive
  - `[ANOMALY]` — detekcja anomalii pistoletów z listą dotkniętych
  - `[BACKUP]` — okresowy backup NVS, auto-restore

#### Nowe pliki
| Plik | Opis |
|------|------|
| `src/event_log.h/cpp` | Moduł logowania zdarzeń na SD |
| `src/nvs_backup.h/cpp` | Backup/restore NVS na kartę SD (JSON) |

---

## [2.19.0] - 2026-02-24

### Dodano - WebSocket push, integracja GIS/GeoJSON

#### 1) WebSocket push (port 81)
- Zastąpienie pollingu `setInterval(fetch, 1000)` przez WebSocket push co 500 ms
- `WebSocketsServer` na porcie 81 (biblioteka `links2004/WebSockets@^2.4.1`)
- Broadcast pełnego status JSON do wszystkich podłączonych klientów
- Frontend: auto-connect z fallback na REST polling 2 s przy braku WS
- Współdzielona funkcja `applyStatus(d)` między WS push a REST fetch
- Wskaźnik statusu WS w panelu informacji systemowych
- Stack tasku WebServer: 12 KB → 16 KB (overhead WS + GeoJSON)

#### 2) Integracja GIS / GeoJSON
- `GET /api/reports/geojson?file=RRRRMMDD.csv` — konwersja CSV sesji → GeoJSON Points
  - Chunked HTTP streaming: pamięciooszczędny (brak bufora całego pliku)
  - GeoJSON FeatureCollection z Point geometry per sesja
- Eksport trasy GPS w dwóch formatach: `.gpx` + `.geojson` do `/tracks/`
  - Wspólny timestamp w nazwie obu plików
  - GeoJSON LineString z koordynatami [lng, lat, alt]
- `GET /api/tracks` — lista plików tras GPS (JSON)
- `GET /api/tracks/download?file=...` — pobieranie plików tras (GPX/GeoJSON)
- Nowa zakładka "Trasy GPS" w menu serwisowym WWW z listą tras i linkami pobierania
- Link do GeoJSON obok CSV w tabeli raportów

#### Nowe endpointy API
| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/api/reports/geojson` | GET | CSV → GeoJSON Points (chunked) |
| `/api/tracks` | GET | Lista plików tras GPS |
| `/api/tracks/download` | GET | Pobierz plik trasy (GPX/GeoJSON) |

---

## [2.18.0] - 2026-02-24

### Dodano - Zapis trasy GPS (GPX) na SD

- **Nagrywanie trasy GPS** podczas malowania (`gps_track.h/cpp`)
  - Bufor punktów w PSRAM: do 4320 punktów (32 B × 4320 = ~135 KB, ~6 h przy 5 s interwale)
  - Fallback na RAM: 300 punktów (~9.6 KB, ~25 min) gdy brak PSRAM
  - Punkt trasy: `lat`, `lng`, `alt`, `speed`, Unix timestamp (z RTC)
  - Zapis GPX 1.1 XML do `/tracks/` na SD — kompatybilny z Google Earth, QGIS, Strava
  - Flush co 100 punktów (ochrona przed utratą danych)
- Stałe konfiguracji: `GPX_RECORD_INTERVAL_MS` (5000 ms), `GPX_MAX_POINTS` (4320)
- Nowy getter `gps_handler.h`: `getAltitude()`
- Automatyczne nagrywanie: START → `startRecording()`, STOP → `stopRecording()` + zapis na SD
- Konwersja czasu: `dateTimeToUnix()` i `unixToISO8601()` (prosty UTC, lata 2000–2099)

#### Nowe pliki
| Plik | Opis |
|------|------|
| `src/gps_track.h/cpp` | Klasa GpsTrack — bufor PSRAM, writer GPX 1.1 |

---

## [2.17.0] - 2026-02-24

### Zmieniono - Enkoder kwadraturowy x4

- **Pełne dekodowanie kwadraturowe x4** obu kanałów A (CLK) + B (DT)
  - Tablica stanów 4×4 (Gray code) do niezawodnego dekodowania kierunku
  - ISR `CHANGE` na obu pinach CLK i DT — 4× wyższa rozdzielczość
  - Bezpośredni odczyt rejestru GPIO (`GPIO.in` / `GPIO.in1.val`) — ~50 ns
  - Debounce ISR: min 200 μs między impulsami (`ENC_ISR_DEBOUNCE_US`)
  - Przejścia stanów: CW `00→01→11→10→00` (+1), CCW `00→10→11→01→00` (−1)
- Zamiana poprzedniego odczytu jednokierunkowego (tylko CLK) na pełny kwadraturowy

---

## [2.16.0] - 2026-02-24

### Dodano - Podgląd na żywo wzorca na TFT

- **Wskaźnik pozycji** na wizualizacji wzorca podczas malowania:
  - Czerwona linia pozioma przesuwająca się po kolumnach wizualizacji
  - Trójkątny marker po lewej stronie wskazujący dokładną pozycję
  - Pozycja zawijana w cyklu wzorca (cykliczny podgląd kreska/przerwa)
- Nowa metoda `getPatternDistance()` w `PaintingEngine` — zwraca dystans od startu wzorca
- Parametr `positionM` dodany do `drawPatternVisualization()` i `drawPaintingScreen()`
- Na ekranie HOME wizualizacja bez markera (statyczny podgląd)
- Na ekranie PAINTING marker porusza się w czasie rzeczywistym z ruchem maszyny
- Pozwala operatorowi wizualnie śledzić, w którym miejscu cyklu kreska/przerwa aktualnie się znajduje

---

## [2.15.0] - 2026-02-24

### Dodano - Ekran podsumowania etapu (SCREEN_SUMMARY)

- Nowy ekran `SCREEN_SUMMARY` wyświetlany po naciśnięciu STOP podczas malowania
- Zamiast bezpośredniego powrotu do HOME, operator widzi podsumowanie:
  - Kod wzorca użytego podczas etapu
  - Dystans sesji (m / km)
  - Powierzchnia sesji (m²)
  - Czas malowania (HH:MM:SS)
  - Średnia prędkość (km/h)
  - Pozycja GPS (jeśli fix dostępny)
- **Opcje na ekranie podsumowania:**
  - `START` = kontynuuj malowanie (wznów z zachowaniem liczników sesji)
  - `STOP` (krótki) = nowy etap (reset liczników, powrót do HOME)
  - `STOP` (1 s) = powrót do HOME bez resetu liczników
- Nowa metoda `drawSummaryScreen()` w display_manager
- Nowa metoda `handleSummary()` w menu z zachowaniem danych sesji

---

## [2.14.0] - 2026-02-24

### Dodano - Joystick analogowy KY-023

- Nowy moduł `joystick.h/cpp` — driver joysticka analogowego KY-023
- **Piny:** VRx = GPIO 19 (ADC2_CH8), VRy = GPIO 20 (ADC2_CH9), SW = GPIO 46 (strap pin)
- **Mapowanie osi na zdarzenia menu:**
  - Góra → `EVT_STOP_SHORT` (poprzednia pozycja)
  - Dół → `EVT_SELECT_SHORT` (następna pozycja)
  - Prawo → `EVT_SELECT_LONG` (wejdź / zmień wartość)
  - Lewo → `EVT_STOP_LONG` (cofnij / powrót)
- **Mapowanie przycisku SW:**
  - Krótkie naciśnięcie → `EVT_SELECT_LONG` (potwierdź)
  - Długie naciśnięcie (1 s) → `EVT_START_LONG` (np. SETUP z HOME)
- **Auto-repeat:** góra/dół z opóźnieniem 400 ms i powtarzaniem co 200 ms
- **Bez auto-repeat:** lewo/prawo — jednorazowe zdarzenie na wychylenie (zapobieganie przypadkowemu wielokrotnemu wchodzeniu/cofaniu)
- **Strefa martwa:** ±500 z centrum ADC (2048), eliminuje szum i mikro-ruchy
- Joystick uzupełnia fizyczne przyciski — nie zastępuje ich
- Integracja w `main.cpp`: `joystick.begin()` w setup, `joystick.update()` + `getEvent()` w loop
- Dodano joystick do BOM, schematu podłączeń, mapy GPIO, instrukcji obsługi

---

## [2.13.0] - 2026-02-23

### Dodano - Ekran przygotowania (SETUP), stałe layoutu, reset etapu

#### 1) Ekran przygotowania (SETUP) — zastępuje MODE SELECT
- Nowy ekran `SCREEN_SETUP` (zastępuje `SCREEN_MODE_SELECT`) z nagłówkiem "PRZYGOTOWANIE"
- 3 konfigurowalne opcje na jednym ekranie:
  - **Tryb pracy:** AUTO → SEMI-AUTO → RECZNY (cykl)
  - **Przełączanie:** Smart ↔ Instant (toggle)
  - **Start:** Normalny ↔ Od przerwy (toggle, jednorazowy)
- Nawigacja identyczna jak menu serwisowe: SEL=dalej, STOP=cofnij, SEL(1s)=zmień
- START na ekranie SETUP = rozpocznij malowanie z wybranymi ustawieniami
- Tryb i Smart/Instant zapisywane do NVS przy starcie malowania
- Usunięto osobny toggle Smart/Instant z HOME (SEL long) — przeniesiony do SETUP

#### 2) Stałe layoutu wyświetlacza (`display_manager.cpp`)
- Wyekstrahowano ~40 nazwanych stałych `#define` z "magic numbers" rozsianych po kodzie rysowania ekranów
- Stałe zorganizowane w kategorie: marginesy, nagłówek, layout 3-kolumnowy, pozycje Y lewej/prawej kolumny, prostokąty pistoletów, wizualizacja wzorca, menu serwisowe, splash screen
- Ułatwia konserwację i modyfikację layoutu UI bez szukania wartości w kodzie

#### 3) Reset etapu (sesji) — "Reset etapu" w menu serwisowym
- Nowa 5. pozycja w menu serwisowym: **Reset etapu**
- Nowy ekran `SCREEN_SESSION_RESET` z potwierdzeniem (START=TAK, STOP=NIE)
- Wyświetla bieżące statystyki sesji przed zerowaniem: dystans [m], powierzchnia [m²], czas [s]
- Po potwierdzeniu: zerowanie liczników sesji (`stats.resetSession()`), dystansu enkodera, powrót do HOME
- Sygnał buzzera (2 kHz, 150 ms) potwierdza reset

#### 4) Poprawka const-correctness GpsHandler
- Usunięto kwalifikator `const` z metod getterów `GpsHandler` (`hasFix()`, `getLat()`, `getLng()` itd.)
- Metody TinyGPSPlus wewnętrznie modyfikują flagę `updated` — nie mogą być wywoływane na obiekcie `const`

### Zmieniono
- `SCREEN_MODE_SELECT` → **`SCREEN_SETUP`** — ekran przygotowania z 3 opcjami zamiast tylko trybu pracy
- `drawModeSelect()` → **`drawSetupScreen()`** — nowy layout z etykietami i wartościami
- `handleModeSelect()` → **`handleSetup()`** — nowa logika z nawigacją SEL/STOP i startem malowania
- Usunięto `EVT_SELECT_LONG` (toggle Smart/Instant) z handlera HOME — przeniesiony do SETUP
- `display_manager.cpp`: ~40 stałych `#define` layoutu zamiast magic numbers; nowy ekran `drawSessionResetScreen()`
- `menu.h`: `SERVICE_MENU_ITEMS` z 4 → 5; nowe zmienne `setupCursor`, `setupMode`, `setupSmart`, `setupGapStart`
- `config.h`: `SCREEN_SETUP` + `SCREEN_SESSION_RESET` w enum `ScreenID`; wersja → 2.13.0
- `gps_handler.h`: usunięto `const` z getterów klasy `GpsHandler`

#### Nowe ekrany
| Ekran | ScreenID | Opis |
|-------|----------|------|
| Przygotowanie | `SCREEN_SETUP` | Tryb, Smart/Instant, Start normalny/od przerwy |
| Reset etapu | `SCREEN_SESSION_RESET` | Potwierdzenie zerowania liczników sesji |

---

## [2.12.0] - 2026-02-19

### Dodano - Moduł GPS, przełączanie Smart/Instant przyciskiem fizycznym

#### 1) Obsługa modułu GPS GY-NEO6MV2 (NEO-6M)
- Moduł GPS podłączony przez UART2: GPIO 47 (RX ← GPS TX), GPIO 48 (TX → GPS RX), 9600 baud
- Biblioteka TinyGPSPlus do parsowania danych NMEA
- Klasa `GpsHandler` z metodami: `hasFix()`, `getLat()`, `getLng()`, `getSatellites()`, `getGpsSpeed()`, `getHdop()`
- Dane GPS w panelu WWW: sekcja "GPS" z fix/satelity, HDOP, pozycja, prędkość GPS
- Dane GPS w statusie API: pola `gpsFix`, `gpsLat`, `gpsLng`, `gpsSat`, `gpsSpeed`, `gpsHdop`
- Koordynaty GPS zapisywane w raportach CSV: dodane kolumny `lat`, `lon` w nagłówku i danych
- GPS aktualizowany w pętli głównej (`gpsHandler.update()` w `loop()`)

#### 2) Przełączanie Smart/Instant przyciskiem fizycznym
- **SELEKTOR (1 s)** na ekranie HOME → przełącza tryb Smart/Instant bez telefonu
- Krótki sygnał buzzera (1500 Hz, 80 ms) potwierdza zmianę
- Ustawienie zapisywane trwale w NVS
- Log na Serial: `[MENU] Tryb przelaczania: SMART/INSTANT`

### Zmieniono
- Wersja firmware: 2.11.0 → **2.12.0**
- `report_logger.h/cpp`: sygnatura `logSession()` rozszerzona o `lat`, `lng`; CSV z kolumnami GPS
- `painting_engine.cpp`: przekazuje koordynaty GPS do `reportLogger.logSession()` przy STOP
- `menu.cpp`: nowy handler `EVT_SELECT_LONG` na `SCREEN_HOME` — toggle Smart/Instant
- `web_server.cpp`: dane GPS w JSON statusu + sekcja GPS w HTML panelu + JS rendering
- `platformio.ini`: dodano bibliotekę `mikalhart/TinyGPSPlus@^1.0.3`
- `config.h`: piny GPS (47/48), stała `GPS_BAUD=9600`

#### Nowe pliki
| Plik | Opis |
|------|------|
| `src/gps_handler.h` | Deklaracja klasy `GpsHandler` (wrapper TinyGPSPlus) |
| `src/gps_handler.cpp` | Implementacja: UART2 init, parsowanie NMEA |

#### Nowe stałe w config.h
| Stała | Wartość | Opis |
|-------|---------|------|
| `PIN_GPS_RX` | 47 | ESP32 RX ← GPS TX (UART2) |
| `PIN_GPS_TX` | 48 | ESP32 TX → GPS RX (UART2) |
| `GPS_BAUD` | 9600 | Domyślny baudrate NEO-6M |

#### Nowe pola API status
| Pole | Typ | Opis |
|------|-----|------|
| `gpsFix` | bool | Czy GPS ma fix (lokalizacja ważna, age < 3 s) |
| `gpsLat` | string | Szerokość geograficzna (6 miejsc po przecinku) |
| `gpsLng` | string | Długość geograficzna (6 miejsc po przecinku) |
| `gpsSat` | int | Liczba widocznych satelitów |
| `gpsSpeed` | string | Prędkość z GPS [km/h] |
| `gpsHdop` | string | HDOP (dokładność pozycji) |

---

## [2.11.0] - 2026-02-19

### Dodano - Podgląd multi-gun, tryb przełączania Smart/Instant

#### 1) Podgląd wzorca Canvas — multi-gun + szerokość
- Wzorce wielopistoletowe (P-3a, P-3b, P-4) wyświetlają osobny rząd per pistolet
- Każdy rząd: nazwa pistoletu, badge szerokości (12cm/24cm wąska/szeroka), wizualizacja kreska/przerwa
- Kolory badge: niebieski = 12cm (wąska), pomarańczowy = 24cm (szeroka)
- Wzorce ciągłe: wypełniony prostokąt z etykietą "Ciagly"
- Dynamiczna wysokość canvasu w zależności od liczby pistoletów
- Spójna skala kreska/przerwa dla wszystkich pistoletów w tym samym wzorcu

#### 2) Tryb przełączania wzorców — Smart vs Instant
- **Smart (domyślny):** zmiana wzorca podczas malowania czeka na koniec bieżącego cyklu (kreska+przerwa)
- **Instant:** natychmiastowa zmiana wzorca — ucina bieżący wzorzec w dowolnym momencie
- Wybór z panelu WWW: dwa przyciski Smart/Instant pod podglądem wzorca
- Smart podświetlony na zielono, Instant na pomarańczowo
- Opis kontekstowy: "dokonczy cykl przed zmiana" / "natychmiastowa zmiana wzorca"
- Ustawienie zapisywane trwale w NVS — przetrwa restart urządzenia
- Nowa akcja API: `set_switch_mode` (value=0 Smart, value=1 Instant)
- Nowe pole API status: `smartSwitch` (bool)

### Zmieniono
- Wersja firmware: 2.10.0 → **2.11.0**
- `PAT_DEFS` JS: z `[line,gap]` na `[[gun,width,line,gap],...]` — multi-gun
- `drawPatPreview()`: pełna przeróbka — osobny rząd per pistolet z labelami i szerokością
- `painting_engine`: `setPattern()` obsługuje tryb Smart i Instant
- `painting_engine.h`: dodano `smartSwitch`, `setSmartSwitch()`, `isSmartSwitch()`
- `storage`: dodano `saveSwitchMode()`/`loadSwitchMode()`
- `main.cpp`: ładowanie trybu przełączania z NVS przy starcie

---

## [2.10.0] - 2026-02-19

### Dodano - Bezpieczeństwo wielordzeniowe, sloty wzorców, eksport CSV, podgląd wzorca

#### 1) Mutex Core 0/1 — eliminacja race conditions
- Dodano `portMUX_TYPE g_stateMux` ze spinlockiem `taskENTER_CRITICAL`/`taskEXIT_CRITICAL`
- Makra `STATE_LOCK()` / `STATE_UNLOCK()` do bezpiecznego dostępu do `g_state` z obu rdzeni
- Ochrona zapisów `machineMode`, `displayNeedsUpdate` z Core 0 (serwer WWW)
- Atomowy snapshot pól `g_state` w `getStateJson()` — brak częściowych odczytów

#### 2) Walidacja NVS z wersją — migracja danych przy upgrade
- Pole `NVS_DATA_VERSION` (uint8_t) w NVS — bieżąca wersja: 2
- Przy starcie `storage.begin()` sprawdza wersję i migruje dane:
  - v0/v1 → v2: kasuje stare klucze wzorców własnych (zmiana formatu struct)
- Automatyczna aktualizacja wersji NVS po migracji
- Zabezpieczenie przed deserializacją niekompatybilnych danych po update firmware

#### 3) 3 sloty pamięci wzorców własnych
- Zamiast jednego wzorca własnego — 3 niezależne sloty (Slot 1/2/3)
- Panel WWW: 3 zakładki slotów z wizualnym wskaźnikiem aktywnego i zapisanych
- Klucze NVS: `cust_p0`, `cust_p1`, `cust_p2`
- Nowa akcja API: `activate_slot` (value=0-2) — przełączanie aktywnego slotu
- Akcja `save_custom_pattern` rozszerzona o parametr `slot` (0-2)
- Nowe pola API status: `activeSlot`, `slotsValid[3]`

#### 4) Eksport raportów CSV przez WWW — pobieranie plików
- Nowy endpoint: `GET /api/reports/download?file=FILENAME`
- Strumieniowe wysyłanie pliku z karty SD (512 B chunki — brak bufora w RAM)
- Nagłówek `Content-Disposition: attachment` — automatyczne pobieranie pliku
- Walidacja nazwy pliku: tylko alfanumeryczne + `.`, `_`, `-`
- Tabela raportów w panelu WWW: nowa kolumna "Pobierz" z linkami CSV

#### 5) Licznik strzałów pistoletów (lifetime)
- Detekcja tranzycji OFF→ON per pistolet (zliczanie strzałów)
- Liczniki zapisywane trwale w NVS (`gun_shots` — tablica 6 × uint32_t)
- Automatyczny zapis przy `saveLifetime()` (co 60 s podczas malowania)
- Nowe pole API stats: `gunShotCounts[6]`
- Panel WWW: wyświetlanie liczników w menu serwisowym (zakładka Statystyki)

#### 6) Podgląd wzorca na WWW — Canvas wizualizacja
- Element `<canvas>` pod sekcją wyboru wzorca
- Rysowanie wzorca kreska/przerwa w skali z wymiarami (np. "4m / 8m")
- Wzorce ciągłe: wypełniony prostokąt z etykietą "Ciągły"
- Automatyczna aktualizacja przy zmianie wzorca (co 1 s z `fetchStatus`)
- Tablica `PAT_DEFS` z definicjami kreska/przerwa dla 16 wzorców

### Zmieniono
- Wersja firmware: 2.9.0 → **2.10.0**
- `config.h`: `STATE_LOCK()/STATE_UNLOCK()`, `NVS_DATA_VERSION=2`, `NUM_CUSTOM_SLOTS=3`
- `storage`: wersjonowanie NVS, slotowe wzorce, liczniki strzałów
- `patterns`: 3 sloty wzorców własnych, `activateSlot()`, `isSlotValid()`
- `statistics`: detekcja tranzycji OFF→ON, `gunShotCounts`, zapis do NVS
- `web_server`: mutex, sloty, canvas preview, CSV download, gun shot display
- `main.cpp`: definicja `portMUX_TYPE g_stateMux`

#### Nowe stałe/typy w config.h
| Typ/Stała | Wartość | Opis |
|-----------|---------|------|
| `NVS_DATA_VERSION` | 2 | Wersja formatu danych NVS |
| `NUM_CUSTOM_SLOTS` | 3 | Liczba slotów wzorców własnych |
| `STATE_LOCK()` | makro | Wejście w sekcję krytyczną g_state |
| `STATE_UNLOCK()` | makro | Wyjście z sekcji krytycznej g_state |

#### Nowe endpointy/akcje API
| Endpoint/Akcja | Opis |
|----------------|------|
| `GET /api/reports/download?file=X` | Pobierz plik CSV raportu |
| `activate_slot` (value=0-2) | Przełącz aktywny slot wzorca własnego |
| `save_custom_pattern` (+slot) | Zapis wzorca do wybranego slotu |

#### Nowe pola API
| Pole | Endpoint | Typ | Opis |
|------|----------|-----|------|
| `activeSlot` | /api/status | int | Aktywny slot wzorca własnego (0-2) |
| `slotsValid` | /api/status | array[3] | Flagi zapisanych slotów |
| `gunShotCounts` | /api/stats | array[6] | Licznik strzałów per pistolet (lifetime) |

---

## [2.9.0] - 2026-02-18

### Dodano - Tryby pracy (Auto/Semi/Manual) + Wzorzec własny

#### 1) Trzy tryby pracy maszyny
- **Automatyczny (AUTO)** — pełna automatyka dystansowa (dotychczasowe zachowanie)
- **Półautomatyczny (SEMI-AUTO)** — linia malowana automatycznie do długości ze wzorca, przerwa ręczna (operator naciska START aby rozpocząć kolejną linię)
- **Ręczny (MANUAL)** — pistolety strzelają gdy operator trzyma przycisk START (jak czyszczenie dysz, ale z pełnymi statystykami i wzorcem)

#### 2) Wybór trybu pracy na urządzeniu
- Długie przytrzymanie START na ekranie HOME (idle) → ekran wyboru trybu
- Krótkie kliknięcia START przełączają: AUTO → SEMI → MANUAL
- Długie przytrzymanie START zatwierdza wybór + sygnał buzzer (2 kHz, 150 ms)
- STOP anuluje i wraca do HOME bez zmiany
- Tryb zapisywany trwale w NVS i wczytywany przy starcie

#### 3) Wskaźnik trybu na wyświetlaczu
- Ekran HOME: etykieta `[AUTO]`, `[SEMI]` lub `[RECZNY]` pod statusem "Gotowy"
- Ekran PAINTING: etykieta trybu pod dystansem sesji
- Ekran SCREEN_MODE_SELECT: 3 opcje z opisami, bieżący tryb oznaczony `*`

#### 4) Wzorzec własny (PAT_CUSTOM)
- Nowy 16. wzorzec: użytkownik definiuje konfigurację pistoletów z indywidualnymi parametrami
- Edytor w panelu WWW: 6 rozwijanych list (Wyłączony/Ciągły/Przerywany) + per-pistolet pola kreska/przerwa [m]
- Każdy pistolet "Przerywany" ma własne, niezależne parametry długości kreski i przerwy
- Przycisk "Zapisz wzorzec" zapisuje do NVS, "Użyj wzorca" aktywuje PAT_CUSTOM
- Ograniczenia: linia/przerwa 0.1–50.0 m, walidacja po stronie serwera

#### 5) Tryb pracy w panelu WWW
- Nowa sekcja "Tryb pracy" z 3 przyciskami: AUTO / SEMI / RECZNY
- Opis trybu wyświetlany pod przyciskami
- Przycisk "NASTĘPNA LINIA" widoczny w trybie SEMI gdy linia zakończona
- Nowe pola API: `mode`, `semiLineComplete`, `patternIdx`, `customValid`
- Nowe akcje API: `set_mode`, `save_custom_pattern`, `semi_next_line`

#### 6) Logika trybu półautomatycznego w silniku malowania
- Śledzenie dystansu linii (`semiLineDist`) z akumulacją delta dystansu
- Pistolety DASHED: automatyczne wyłączenie po osiągnięciu `lineLen`
- Pistolety CONTINUOUS: zawsze aktywne (niezależne od fazy linii)
- Sygnał buzzer (1 kHz, 50 ms) po zakończeniu linii
- `semiNextLine()`: zeruje dystans linii i flagę zakończenia

#### 7) Logika trybu ręcznego w silniku malowania
- Pistolety strzelają gdy `buttons.isStartHeld()` && prędkość >= 3 km/h
- Konfiguracja pistoletów pobierana z bieżącego wzorca (GUN_OFF = wyłączony)
- Pełne statystyki sesji (dystans, powierzchnia, czas)
- EVT_START_SHORT na ekranie malowania ignorowany (nie przełącza pauzy)

### Zmieniono
- Wersja firmware: 2.8.0 → **2.9.0**
- `config.h`: enum `MachineMode`, `SCREEN_MODE_SELECT`, `PAT_CUSTOM`, `CustomPatternCfg`
- `button_handler`: dodany `EVT_START_LONG` (priorytet nad EVT_START_SHORT)
- `storage`: `saveMode()`/`loadMode()`, `saveCustomPattern()`/`loadCustomPattern()`
- `patterns`: tablica 15 predefiniowanych + 1 mutowalny custom, bezpieczne `getPattern()`
- `painting_engine`: 3-trybowy `update()`, `semiNextLine()`, `isSemiLineComplete()`
- `menu`: `handleModeSelect()`, trybo-świadomy `handlePaintingScreen()`
- `display_manager`: `drawModeSelect()`, `modeStr()`, wskaźnik trybu na HOME/PAINTING
- `web_server`: sekcja trybu, edytor wzorca własnego, nowe akcje API
- `main.cpp`: wczytywanie trybu z NVS przy starcie

#### Nowe stałe/typy w config.h
| Typ/Stała | Wartość | Opis |
|-----------|---------|------|
| `MachineMode` | enum 0-2 | MODE_AUTO, MODE_SEMI_AUTO, MODE_MANUAL |
| `SCREEN_MODE_SELECT` | ScreenID | Nowy ekran wyboru trybu |
| `PAT_CUSTOM` | PatternID 15 | Wzorzec własny użytkownika |
| `CustomPatternCfg` | struct | gunModes[6], lineLen[6], gapLen[6], valid |

#### Nowe akcje API
| Akcja | Parametry | Opis |
|-------|-----------|------|
| `set_mode` | value=0-2 | Zmiana trybu pracy (0=AUTO, 1=SEMI, 2=MANUAL) |
| `save_custom_pattern` | g0..g5, ln0..ln5, gp0..gp5 | Zapis wzorca własnego (per-gun) |
| `semi_next_line` | — | Wyzwolenie kolejnej linii w trybie SEMI |

---

## [2.8.0] - 2026-02-17

### Dodano - Inteligentne przełączanie wzorców (Smart Pattern Switch)

#### Logika przełączania podczas malowania
- Zmiana wzorca podczas malowania **NIE przerywa** bieżącego cyklu linia+przerwa
- Bieżąca linia jest domalowywana do pełnej długości
- Przerwa po linii jest dokańczana
- Nowy wzorzec zaczyna się dopiero po zakończeniu pełnego cyklu starego
- Wzorce ciągłe (P-2a, P-2b, P-4, P-7b, P-7d) przełączają się natychmiast
- PAUZA i STOP mogą przerwać w dowolnym momencie (jak dotychczas)

#### Wizualne potwierdzenie w panelu WWW
- Oczekujący wzorzec miga pomarańczowo (klasa CSS `.pbtn.pending`)
- Po przełączeniu: krótki sygnał dźwiękowy 1500 Hz (80 ms)
- Nowe pola API: `patternPending` (bool), `pendingPattern` (string)

#### Zachowanie w stanach specjalnych
- **STOP**: wymusza zastosowanie oczekującego wzorca (żeby po STOP był aktywny nowy)
- **START**: resetuje kolejkowanie (nowa sesja = czysta karta)
- **Anulowanie**: kliknięcie bieżącego aktywnego wzorca anuluje oczekującą zmianę

---

## [2.7.0] - 2026-02-16

### Dodano - Menu serwisowe w panelu WWW

#### 1) Sekcja "Menu serwisowe" z zakładkami
- Nowa sekcja w panelu HTML z dwoma zakładkami: **Statystyki** i **Raporty SD**
- Zakładka **Statystyki**: dystans calkowity, powierzchnia, czas malowania lifetime
- Wyświetla status karty SD i liczbę raportów
- Dystans per pistolet (sesja) — 6 kółek z wartościami w metrach
- Auto-odświeżanie statystyk co 10 sekund (gdy zakładka aktywna)

#### 2) Zakładka Raporty SD
- Lista plików raportów CSV z karty SD (nazwa + rozmiar)
- Tabela sortowana malejąco (najnowsze pierwsze)
- Przycisk "Odśwież" do ręcznego odświeżenia listy
- Dane pobierane z istniejącego endpointu `/api/reports`

#### 3) Wskaźniki anomalii pistoletów w panelu WWW
- Banner ostrzegawczy "ANOMALIA PISTOLETU" (pulsujący czerwony) gdy wykryto anomalię
- Kółka pistoletów z anomalią migają czerwoną ramką (klasa CSS `.anom`)
- Animacja `anomBlink` z box-shadow dla wyraźnej sygnalizacji

### Zmieniono
- Wersja firmware: 2.6.0 → **2.7.0**
- Przebudowany auto-refresh: `setInterval` z wewnętrznym fetch (eliminuje podwójne wywołanie)
- CSS: dodane style `.svc-tabs`, `.svc-tab`, `.rep-tbl`, `.anom-warn`, `.gun-circle.anom`

---

## [2.6.0] - 2026-02-16

### Dodano - Rozszerzone API, detekcja anomalii, raporty SD

#### 1) Endpoint `/api/stats` — statystyki lifetime i sesji
- Nowy endpoint `GET /api/stats` zwracający JSON z pełnymi statystykami
- Dane lifetime: łączny dystans, powierzchnia, czas malowania
- Dane sesji: dystans, powierzchnia, czas, dystans per pistolet (`gunDistances[6]`)
- Status karty SD i liczba raportów

#### 2) Stack webservera 8192 → 12288 B
- Zwiększono rozmiar stosu tasku HTTP na Core 0 z 8192 do 12288 bajtów
- Eliminuje ryzyko stack overflow przy dużych odpowiedziach JSON

#### 3) Endpoint `/api/reports` — lista raportów SD
- Nowy endpoint `GET /api/reports` zwracający JSON z listą plików CSV
- Odpowiedź: `[{"file":"RRRRMMDD.csv","size":1234},...]`
- Lista sortowana malejąco (najnowsze pierwsze, max 50)
- Cache raportów odświeżany co 15 s na Core 1 (bezpieczny dostęp SPI/SD)

#### 4) Detekcja anomalii pistoletów
- Sprawdzanie co 10 s podczas malowania (`GUN_ANOMALY_CHECK_MS`)
- Aktywacja: dystans sesji ≥ 50 m (`GUN_ANOMALY_DISTANCE_M`)
- Logika: pistolet skonfigurowany ale dystans <1 m → anomalia
- Buzzer: nowy sygnał `BUZ_GUN_ANOMALY` (800/1200 Hz, niski-wysoki-niski)
- Alert jednokrotny (buzzer + Serial log)
- Reset anomalii przy zatrzymaniu malowania
- Nowe pola API: `gunAnomalyDetected`, `gunAnomaly[6]`

### Zmieniono
- Wersja firmware: 2.5.0 → **2.6.0**
- `web_server`: stack 12288 B, 5 endpointów, anomalia w status JSON
- `report_logger`: cache raportów z odświeżaniem co 15 s
- `buzzer`: nowy sygnał `BUZ_GUN_ANOMALY`
- `config.h`: stałe anomalii, struct `GunAnomalyState`
- `main.cpp`: detekcja anomalii (10), cache raportów (11)

#### Nowe stałe/timery
| Stała | Wartość | Opis |
|-------|---------|------|
| `GUN_ANOMALY_DISTANCE_M` | 50.0 | Min dystans sesji do detekcji anomalii [m] |
| `GUN_ANOMALY_CHECK_MS` | 10000 | Interwał sprawdzania anomalii [ms] |
| `REPORT_CACHE_MS` | 15000 | Interwał odświeżania cache raportów SD [ms] |

#### Nowe endpointy API
| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/api/stats` | GET | Statystyki lifetime + sesja + per-gun |
| `/api/reports` | GET | Lista plików raportów CSV z karty SD |

---

## [2.5.0] - 2026-02-16

### Dodano - Optymalizacja wielordzeniowa i diagnostyka

#### A) Wielordzeniowość FreeRTOS (dual-core ESP32-S3)
- Serwer WWW przeniesiony na **Core 0** jako osobny task FreeRTOS (`xTaskCreatePinnedToCore`)
- Krytyczna pętla `loop()` (enkoder, pistolety, buzzer, wyświetlacz) działa na **Core 1**
- Stack tasku WWW: 8192 B, priorytet 1
- Eliminuje blokowanie krytycznej pętli przez klientów HTTP (np. szybkie odświeżanie)

#### B) Okresowy zapis statystyk lifetime co 60 s
- Nowy timer `LIFETIME_SAVE_MS = 60000` w `loop()` — statystyki zapisywane automatycznie co 60 s podczas malowania
- Zabezpiecza przed utratą danych przy resecie watchdoga, zaniku zasilania, awarii w trakcie sesji
- Timer resetowany gdy maszyna nie maluje

#### D) Anti-flicker na WSZYSTKICH ekranach
- Usunięto `clear()` z ekranów: Menu serwisowe, Kalibracja, Pomiar dystansu, Raporty, Czyszczenie dysz
- Zastosowano `setTextPadding()` + `fillRect()` na stałych pozycjach Y — tekst nadpisywany bez migania
- Początkowe czyszczenie ekranu obsługiwane przez `forceFullRedraw` w `menu.cpp`

#### E) PROGMEM + chunked transfer strony HTML
- Strona HTML (~7 KB) podzielona na 2 bloki `static const char[] PROGMEM`
- Wysyłanie fragmentami: `sendContent_P()` — brak alokacji całej strony w RAM
- Jedyna dynamiczna wstawka: `FW_VERSION` (5 bajtów)

#### F) Szybszy ISR enkodera — bezpośredni odczyt GPIO
- Zamiana `digitalRead()` (~2–3 μs) na makro `FAST_GPIO_READ()` (~50 ns)
- Bezpośredni dostęp do rejestru `GPIO.in` / `GPIO.in1.val`
- Eliminuje gubienie impulsów przy dużych prędkościach z gęstym enkoderem

#### H) Diagnostyka i monitoring systemowy
- Nowy log `[DIAG]` co 30 s: heap wolny/łączny, min free heap, fragmentacja %, stack HWM tasku WWW, numer core
- Nowe pola API JSON: `minFreeHeap`, `webStackHWM`
- Stack high-water mark tasku WWW: `TrassarWebServer::getTaskStackHWM()`

#### I) Czas sesji i dystans na ekranie malowania
- Ekran `SCREEN_PAINTING`: nowe pola w lewej kolumnie pod statusem
  - Czas sesji (format MM:SS lub H:MM:SS) — `stats.getSessionTimeSec()`
  - Dystans sesji (m lub km przy ≥1000 m) — `stats.getSessionDistance()`
- Rozszerzona sygnatura `drawPaintingScreen()` o parametry `sessionTimeSec` i `sessionDistM`

### Zmieniono
- Wersja firmware: 2.4.0 → **2.5.0**
- `web_server`: Core 0 task, chunked HTML, nowe pola JSON diagnostyczne
- `encoder_distance`: ISR z bezpośrednim GPIO zamiast digitalRead
- `display_manager`: wersja nagłówka v2.5.0, anti-flicker na 5 ekranach, czas/dystans na painting
- `main.cpp`: dual-core opis, timery diagnostyki i lifetime save, include `esp_heap_caps.h`
- `config.h`: wersja 2.5.0
- `menu.cpp`: nowe parametry w wywołaniu `drawPaintingScreen()`

#### Nowe stałe/timery
| Stała | Wartość | Opis |
|-------|---------|------|
| `DIAG_PRINT_MS` | 30000 | Interwał diagnostyki Serial [ms] |
| `LIFETIME_SAVE_MS` | 60000 | Interwał zapisu statystyk do NVS [ms] |

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
