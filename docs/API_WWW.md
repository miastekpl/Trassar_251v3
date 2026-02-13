# TrassarV3 - API serwera WWW v2.3.0

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
- 15 przycisków wzorców pogrupowanych: P-1x, P-2x, P-3x, P-4/P-6, P-7x
- Przycisk odwracania (dla P-3a/P-3b)
- Wskaźniki 6 pistoletów (P1-P6)
- Sekcja kalibracji enkodera
- Informacje systemowe

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
    "speed": "0.0",
    "distance": "0.0",
    "area": "0.00",
    "elapsed": 0,
    "firmware": "2.3.0",
    "freeHeap": 245760,
    "uptime": 3600,
    "clients": 1,
    "calibrated": true,
    "ppm": "100.0",
    "calibrating": false,
    "calPulses": "0",
    "guns": [false, false, false, false, false, false],
    "gapStart": false
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
| `uptime` | int | Czas pracy od uruchomienia [sekundy] |
| `clients` | int | Liczba podłączonych klientów WiFi |
| `calibrated` | bool | Czy enkoder jest skalibrowany |
| `ppm` | string | Impulsy na metr |
| `calibrating` | bool | Czy trwa kalibracja |
| `calPulses` | string | Impulsy zebrane podczas kalibracji |
| `guns` | array[6] | Stan pistoletów P1-P6 (true = ON) |
| `gapStart` | bool | Czy aktywny jest tryb "start od przerwy" |

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
| `set_pattern` | 0-14 | Ustaw wzorzec (indeks PatternID) |
| `toggle_reverse` | - | Odwróć wzorzec (P-3a/P-3b) |
| `cal_start` | - | Rozpocznij kalibrację enkodera |
| `cal_finish` | - | Zakończ kalibrację enkodera |

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
| 7 | P-3a | | |

**Odpowiedź:** `application/json`

```json
{
    "result": "ok"
}
```

**Przykłady użycia (curl):**

```bash
# Sprawdź status
curl http://192.168.4.1/api/status

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
