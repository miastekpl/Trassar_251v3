# TrassarV3 - API serwera WWW v2.52.0

## Informacje ogólne

- **Adres:** `http://192.168.4.1`
- **Port:** 80
- **Tryb WiFi:** Access Point
- **SSID:** TrassarV3
- **Hasło:** 12345678
- **Max klientów:** 4

## Endpointy

### GET /

Zwraca stronę HTML panelu sterowania.

**Odpowiedź:** `text/html` - pełna strona z interfejsem graficznym

Panel zawiera:
- Status maszyny z animowanym wskaźnikiem
- Informacje: wzorzec, prędkość, dystans, powierzchnia, czas
- Przyciski START / PAUZA / STOP / **START OD PRZERWY**
- **Selektor trybu pracy** — 3 przyciski: AUTO / SEMI / RĘCZNY
- 16 przycisków wzorców pogrupowanych: P-1x, P-2x, P-3x, P-4/P-6, P-7x, WŁASNY
- **Edytor wzorca własnego** — konfiguracja 6 pistoletów, kreska/przerwa, 3 sloty pamięci, zapis do NVS
- **Podgląd wzorca** — Canvas wizualizacja kreska/przerwa w skali
- Przycisk odwracania (dla P-3a/P-3b)
- Wskaźniki 6 pistoletów (P1-P6)
- **Przycisk "Kolejna linia"** — widoczny w trybie SEMI gdy kreska zakończona
- Sekcja kalibracji enkodera
- Sekcja alarmu prędkości (suwak konfiguracji progu max.)
- **Sekcja GPS** — fix/satelity, HDOP, pozycja, prędkość GPS
- Informacje systemowe
- **Menu serwisowe** z zakładkami:
  - **Statystyki** — dystans/powierzchnia/czas lifetime, status SD, dystans per pistolet, licznik strzałów
  - **Raporty SD** — tabela plików CSV z nazwą, rozmiarem i linkiem pobierania
- **Banner anomalii pistoletów** — pulsujący alert gdy wykryto anomalię
- Wskaźniki anomalii na kółkach pistoletów (migająca czerwona ramka)

---

### GET /api/status

Zwraca aktualny stan systemu w formacie JSON.

**Odpowiedź:** `application/json`

```json
{
    "state": "idle",
    "pattern": "P-1a",
    "patternName": "Przerywana dluga",
    "reversed": false,
    "gapStart": false,
    "speed": "0.0",
    "distance": "0.0",
    "area": "0.00",
    "elapsed": 0,
    "firmware": "2.5.0",
    "freeHeap": 245760,
    "minFreeHeap": 210000,
    "uptime": 3600,
    "clients": 1,
    "webStackHWM": 2048,
    "calibrated": true,
    "ppm": "100.0",
    "calibrating": false,
    "calPulses": "0",
    "maxSpeed": "15.0",
    "overspeed": false,
    "lowSpeed": false,
    "guns": [false, false, false, false, false, false],
    "patternPending": false,
    "pendingPattern": "P-1b",
    "gunAnomalyDetected": false,
    "gunAnomaly": [false, false, false, false, false, false],
    "mode": "auto",
    "semiLineComplete": false,
    "patternIdx": 0,
    "customValid": false,
    "activeSlot": 0,
    "slotsValid": [true, false, false],
    "smartSwitch": true,
    "gpsFix": true,
    "gpsLat": "52.229676",
    "gpsLng": "21.012229",
    "gpsSat": 8,
    "gpsSpeed": "12.5",
    "gpsHdop": "1.2"
}
```

**Pola:**

| Pole | Typ | Opis |
|------|-----|------|
| `state` | string | Stan maszyny: `idle`, `painting`, `paused`, `stopped` |
| `pattern` | string | Kod aktualnego wzorca (np. "P-1a") |
| `patternName` | string | Nazwa wzorca |
| `reversed` | bool | Czy wzorzec jest odwrócony |
| `speed` | string | Prędkość [km/h] |
| `distance` | string | Dystans sesji [m] |
| `area` | string | Powierzchnia sesji [m²] |
| `elapsed` | int | Czas malowania sesji [sekundy] |
| `firmware` | string | Wersja firmware |
| `freeHeap` | int | Wolna pamięć RAM [bajty] |
| `minFreeHeap` | int | Minimalna wolna pamięć od startu [bajty] |
| `uptime` | int | Czas pracy od uruchomienia [sekundy] |
| `webStackHWM` | int | Stack high-water mark tasku WWW [bajty] |
| `clients` | int | Liczba podłączonych klientów WiFi |
| `calibrated` | bool | Czy enkoder jest skalibrowany |
| `ppm` | string | Impulsy na metr |
| `calibrating` | bool | Czy trwa kalibracja |
| `calPulses` | string | Impulsy zebrane podczas kalibracji |
| `maxSpeed` | string | Próg alarmu przekroczenia prędkości [km/h] |
| `overspeed` | bool | Czy prędkość przekracza próg maks. |
| `lowSpeed` | bool | Czy prędkość jest poniżej 3 km/h podczas malowania |
| `guns` | array[6] | Stan pistoletów P1-P6 (true = ON) |
| `gapStart` | bool | Czy aktywny jest tryb "start od przerwy" |
| `patternPending` | bool | Czy oczekuje zmiana wzorca (smart switch) |
| `pendingPattern` | string | Kod oczekującego wzorca (obecne tylko gdy `patternPending=true`) |
| `gunAnomalyDetected` | bool | Czy wykryto anomalię pistoletów |
| `gunAnomaly` | array[6] | Flagi anomalii per pistolet (true = brak aktywności mimo konfiguracji) |
| `mode` | string | Aktualny tryb pracy: `auto`, `semi`, `manual` |
| `semiLineComplete` | bool | Czy kreska w trybie SEMI jest zakończona (czeka na START) |
| `patternIdx` | int | Indeks aktualnego wzorca (0–15) |
| `customValid` | bool | Czy wzorzec własny jest skonfigurowany i gotowy do użycia |
| `activeSlot` | int | Aktywny slot wzorca własnego (0-2) |
| `slotsValid` | array[3] | Flagi zapisanych slotów (true = slot zawiera wzorzec) |
| `smartSwitch` | bool | Tryb przełączania wzorców: true=Smart (czekaj na cykl), false=Instant (natychmiast) |
| `gpsFix` | bool | Czy GPS ma fix (lokalizacja ważna, age < 3 s) |
| `gpsLat` | string | Szerokość geograficzna (6 miejsc po przecinku) |
| `gpsLng` | string | Długość geograficzna (6 miejsc po przecinku) |
| `gpsSat` | int | Liczba widocznych satelitów |
| `gpsSpeed` | string | Prędkość z GPS [km/h] |
| `gpsHdop` | string | HDOP — dokładność pozycji (niższa = lepsza, <2.0 = dobra) |

---

### GET /api/stats

Zwraca statystyki lifetime i bieżącej sesji.

**Odpowiedź:** `application/json`

```json
{
    "lifetimeDistanceM": "12500.5",
    "lifetimeAreaM2": "3200.75",
    "lifetimePaintTimeSec": 86400,
    "sessionDistanceM": "250.3",
    "sessionAreaM2": "30.04",
    "sessionTimeSec": 180,
    "gunDistances": ["250.3", "0.0", "250.3", "0.0", "0.0", "0.0"],
    "gunShotCounts": [1250, 0, 1248, 0, 0, 0],
    "sdReady": true,
    "reportCount": 12
}
```

**Pola:**

| Pole | Typ | Opis |
|------|-----|------|
| `lifetimeDistanceM` | string | Łączny dystans malowania od początku [m] |
| `lifetimeAreaM2` | string | Łączna powierzchnia od początku [m²] |
| `lifetimePaintTimeSec` | int | Łączny czas malowania [sekundy] |
| `sessionDistanceM` | string | Dystans bieżącej sesji [m] |
| `sessionAreaM2` | string | Powierzchnia bieżącej sesji [m²] |
| `sessionTimeSec` | int | Czas bieżącej sesji [sekundy] |
| `gunDistances` | array[6] | Dystans per pistolet w sesji [m] |
| `gunShotCounts` | array[6] | Licznik strzałów per pistolet (lifetime, tranzycje OFF→ON) |
| `sdReady` | bool | Czy karta SD jest dostępna |
| `reportCount` | int | Liczba plików raportów na karcie SD |

---

### GET /api/reports

Zwraca listę plików raportów z karty SD (z cache, odświeżany co 15 s).

**Odpowiedź:** `application/json`

```json
[
    {"file": "20260216.csv", "size": 1234},
    {"file": "20260215.csv", "size": 890}
]
```

**Pola elementu tablicy:**

| Pole | Typ | Opis |
|------|-----|------|
| `file` | string | Nazwa pliku raportu (format RRRRMMDD.csv) |
| `size` | int | Rozmiar pliku [bajty] |

> **Uwaga:** Lista sortowana malejąco (najnowsze pierwsze). Cache odświeżany co 15 s na Core 1 (bezpieczny dostęp SPI/SD).

---

### GET /api/reports/download

Pobiera plik raportu CSV z karty SD (strumieniowo, 512 B chunki).

**Parametry (query string):**

| Parametr | Wymagany | Opis |
|----------|----------|------|
| `file` | Tak | Nazwa pliku do pobrania (np. `20260219.csv`) |

**Odpowiedź:** `text/csv` z nagłówkiem `Content-Disposition: attachment`

**Walidacja:** Nazwa pliku może zawierać tylko litery, cyfry, `.`, `_`, `-`. Pozostałe znaki są odrzucane (HTTP 400).

**Kody odpowiedzi:**

| Kod | Opis |
|-----|------|
| 200 | Plik pobrany pomyślnie |
| 400 | Brak parametru `file` lub nieprawidłowa nazwa |
| 404 | Plik nie istnieje na karcie SD |
| 500 | Błąd otwarcia pliku |

**Przykład:**
```bash
# Pobierz raport z 19 lutego 2026
curl -O http://192.168.4.1/api/reports/download?file=20260219.csv
```

---

### POST /api/control

Wysyła komendę sterującą do systemu.

**Parametry (form-urlencoded):**

| Parametr | Wymagany | Opis |
|----------|----------|------|
| `action` | Tak | Komenda do wykonania |
| `value` | Zależy od akcji | Wartość parametru |

**Dostępne akcje:**

| Akcja | Wartość | Opis |
|-------|---------|------|
| `start` | - | Rozpocznij malowanie lub wznów po pauzie |
| `pause` | - | Zapauzuj malowanie |
| `stop` | - | Zatrzymaj malowanie |
| `start_from_gap` | - | **Rozpocznij malowanie od przerwy** (przesuwa punkt startowy o długość kreski) |
| `set_pattern` | 0-15 | Ustaw wzorzec (indeks PatternID, 15 = WŁASNY). **Podczas malowania** zależy od trybu: **Smart** — zmiana kolejkowana do końca cyklu; **Instant** — natychmiastowa zmiana (ucięcie bieżącego wzorca). |
| `toggle_reverse` | - | Odwróć wzorzec (P-3a/P-3b) |
| `set_mode` | 0-2 | Ustaw tryb pracy: 0=AUTO, 1=SEMI, 2=RĘCZNY (zapis do NVS) |
| `semi_next_line` | - | Wyzwól kolejną kreskę w trybie SEMI (działa tylko gdy `semiLineComplete=true`) |
| `save_custom_pattern` | *patrz niżej* | Zapisz wzorzec własny do wybranego slotu NVS |
| `activate_slot` | 0-2 | Przełącz aktywny slot wzorca własnego (slot musi być zapisany) |
| `set_switch_mode` | 0-1 | Tryb przełączania wzorców: 0=Smart (dokończ cykl), 1=Instant (natychmiast). Zapis do NVS |
| `cal_start` | - | Rozpocznij kalibrację enkodera |
| `cal_finish` | - | Zakończ kalibrację enkodera |
| `set_max_speed` | 5.0–30.0 | Ustaw próg alarmu prędkości [km/h] (zapis do NVS) |

**Mapowanie indeksów wzorców:**

| Indeks | Wzorzec | Indeks | Wzorzec |
|--------|---------|--------|---------|
| 0 | P-1a | 8 | P-3b |
| 1 | P-1b | 9 | P-4 |
| 2 | P-1c | 10 | P-6 |
| 3 | P-1d | 11 | P-7a |
| 4 | P-1e | 12 | P-7b |
| 5 | P-2a | 13 | P-7c |
| 6 | P-2b | 14 | P-7d |
| 7 | P-3a | 15 | WŁASNY |

**Odpowiedź:** `application/json`

```json
{
    "result": "ok"
}
```

**Parametry akcji `save_custom_pattern`:**

| Parametr | Wymagany | Opis |
|----------|----------|------|
| `g0`–`g5` | Tak | Tryb pistoletu P1–P6: 0=wyłączony, 1=ciągły, 2=przerywany |
| `ln0`–`ln5` | Tak | Długość kreski per pistolet [m] (0.1–50.0) |
| `gp0`–`gp5` | Tak | Długość przerwy per pistolet [m] (0.1–50.0) |
| `slot` | Nie | Numer slotu do zapisu (0-2, domyślnie 0) |

Każdy pistolet ustawiony jako "przerywany" (2) ma własne, niezależne parametry kreski i przerwy. Pistolety ciągłe i wyłączone ignorują te wartości.

Po zapisie wzorzec jest automatycznie aplikowany i zaznaczany jako aktywny w wybranym slocie.

**Przykłady użycia (curl):**

```bash
# Sprawdź status
curl http://192.168.4.1/api/status

# Statystyki lifetime i sesji
curl http://192.168.4.1/api/stats

# Lista raportów SD
curl http://192.168.4.1/api/reports

# Rozpocznij malowanie
curl -X POST -d "action=start" http://192.168.4.1/api/control

# Rozpocznij malowanie od przerwy
curl -X POST -d "action=start_from_gap" http://192.168.4.1/api/control

# Zapauzuj
curl -X POST -d "action=pause" http://192.168.4.1/api/control

# Zatrzymaj
curl -X POST -d "action=stop" http://192.168.4.1/api/control

# Ustaw wzorzec P-3a (indeks 7)
curl -X POST -d "action=set_pattern&value=7" http://192.168.4.1/api/control

# Odwróć wzorzec (P-3a/P-3b)
curl -X POST -d "action=toggle_reverse" http://192.168.4.1/api/control

# Rozpocznij kalibrację enkodera
curl -X POST -d "action=cal_start" http://192.168.4.1/api/control

# Zakończ kalibrację (po przejechaniu 10m)
curl -X POST -d "action=cal_finish" http://192.168.4.1/api/control

# Ustaw próg alarmu prędkości na 12 km/h
curl -X POST -d "action=set_max_speed&value=12" http://192.168.4.1/api/control

# Ustaw tryb pracy na SEMI (1)
curl -X POST -d "action=set_mode&value=1" http://192.168.4.1/api/control

# Wyzwól kolejną kreskę w trybie SEMI
curl -X POST -d "action=semi_next_line" http://192.168.4.1/api/control

# Zapisz wzorzec własny (P2 przerywany 3m/2m, P5 przerywany 1m/1m, reszta wyłączona)
curl -X POST -d "action=save_custom_pattern&g0=0&g1=2&g2=0&g3=0&g4=2&g5=0&ln0=4&gp0=8&ln1=3.0&gp1=2.0&ln2=4&gp2=8&ln3=4&gp3=8&ln4=1.0&gp4=1.0&ln5=4&gp5=8" http://192.168.4.1/api/control

# Użyj wzorca własnego (indeks 15)
curl -X POST -d "action=set_pattern&value=15" http://192.168.4.1/api/control

# Aktywuj slot 2 wzorca własnego
curl -X POST -d "action=activate_slot&value=1" http://192.168.4.1/api/control

# Zapisz wzorzec własny do slotu 3
curl -X POST -d "action=save_custom_pattern&slot=2&g0=0&g1=1&g2=0&g3=0&g4=0&g5=0&ln0=4&gp0=8&ln1=4&gp1=8&ln2=4&gp2=8&ln3=4&gp3=8&ln4=4&gp4=8&ln5=4&gp5=8" http://192.168.4.1/api/control

# Ustaw tryb przelaczania wzorcow na Instant (natychmiastowy)
curl -X POST -d "action=set_switch_mode&value=1" http://192.168.4.1/api/control

# Ustaw tryb przelaczania wzorcow na Smart (dokoncz cykl)
curl -X POST -d "action=set_switch_mode&value=0" http://192.168.4.1/api/control

# Pobierz raport CSV
curl -O http://192.168.4.1/api/reports/download?file=20260219.csv
```

## Kody odpowiedzi HTTP

| Kod | Opis |
|-----|------|
| 200 | Sukces |
| 400 | Brak wymaganego parametru `action` |
| 404 | Nieznany endpoint |

## Autorefresh panelu WWW

Panel HTML automatycznie odpytuje `/api/status` co 1 sekundę za pomocą JavaScript `fetch()`. Dane są aktualizowane w interfejsie bez przeładowania strony.

Przycisk **START OD PRZERWY** jest aktywny tylko gdy maszyna jest w stanie `idle` lub `stopped`. W trakcie malowania przycisk jest wyszarzony.

## Uwagi techniczne

- Serwer obsługuje do 4 jednoczesnych klientów WiFi
- JSON generowany przez ArduinoJson v7
- Wartości liczbowe (`speed`, `distance`, `area`, `ppm`, `calPulses`) przesyłane jako stringi dla zachowania precyzji formatowania
- Stan pistoletów (`guns`) to tablica 6 wartości boolean odpowiadających P1-P6
- Pole `gapStart` informuje panel WWW o trybie startu od przerwy (wyświetla znacznik przy statusie)

---

## WebSocket (port 81) — v2.52.0

Oprócz HTTP polling system oferuje kanał WebSocket na porcie **81**. Panel WWW automatycznie łączy się i otrzymuje broadcast statusu co 500 ms.

**Adres:** `ws://192.168.4.1:81`

**Format broadcastu:** Identyczny JSON jak `GET /api/status`

**Zalety:**
- Push zamiast pull — niższe opóźnienie
- Mniejsze obciążenie sieci
- Automatyczny fallback na HTTP polling jeśli WebSocket niedostępny

---

## Nowe endpointy v2.52.0

### GET /api/tracks

Zwraca listę plików tras GPS z karty SD.

**Odpowiedź:** `application/json`

```json
[
    {"file": "trasa_20260308_091500.gpx", "size": 45678},
    {"file": "trasa_20260308_091500.geojson", "size": 23456}
]
```

---

### GET /api/tracks/download

Pobiera plik trasy GPS z karty SD (strumieniowo).

**Parametry (query string):**

| Parametr | Wymagany | Opis |
|----------|----------|------|
| `file` | Tak | Nazwa pliku do pobrania (np. `trasa_20260308_091500.gpx`) |

---

### GET /api/html_reports

Zwraca listę raportów HTML sesji z karty SD.

**Odpowiedź:** `application/json`

```json
[
    {"file": "raport_20260308_091500.html", "size": 12345}
]
```

---

### GET /api/html_reports/download

Pobiera raport HTML sesji z karty SD.

**Parametry (query string):**

| Parametr | Wymagany | Opis |
|----------|----------|------|
| `file` | Tak | Nazwa pliku do pobrania |

---

### GET /api/reports/geojson

Eksport danych raportów w formacie GeoJSON.

---

## Nowe akcje POST /api/control — v2.52.0

| Akcja | Wartość | Opis |
|-------|---------|------|
| `set_min_speed` | 0.0–50.0 | Ustaw minimalny próg prędkości malowania [km/h] (zapis NVS) |
| `set_tank_capacity` | 1–9999 | Pojemność zbiornika farby [litry] (zapis NVS) |
| `set_paint_rate` | 0.01–99.0 | Współczynnik zużycia farby [l/m²] (zapis NVS) |
| `set_auto_resume` | 0–1 | Auto-wznowienie po auto-pauzie: 0=wyłącz, 1=włącz (zapis NVS) |

**Przykłady (curl):**

```bash
# Ustaw pojemność zbiornika na 150 litrów
curl -X POST -d "action=set_tank_capacity&value=150" http://192.168.4.1/api/control

# Ustaw współczynnik zużycia farby na 0.8 l/m²
curl -X POST -d "action=set_paint_rate&value=0.8" http://192.168.4.1/api/control

# Włącz auto-wznowienie
curl -X POST -d "action=set_auto_resume&value=1" http://192.168.4.1/api/control

# Ustaw minimalną prędkość na 2 km/h
curl -X POST -d "action=set_min_speed&value=2" http://192.168.4.1/api/control

# Pobierz trasę GPS (GPX)
curl -O http://192.168.4.1/api/tracks/download?file=trasa_20260308_091500.gpx

# Pobierz raport HTML sesji
curl -O http://192.168.4.1/api/html_reports/download?file=raport_20260308_091500.html
```
