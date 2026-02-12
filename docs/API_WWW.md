# TrassarV3 - API serwera WWW

## Informacje ogólne

- **Adres:** `http://192.168.4.1`
- **Port:** 80
- **Tryb WiFi:** Access Point
- **SSID:** TrassarV3
- **Hasło:** 12345678

## Endpointy

### GET /

Zwraca stronę HTML panelu sterowania.

**Odpowiedź:** `text/html` - pełna strona z interfejsem graficznym

---

### GET /api/status

Zwraca aktualny stan systemu w formacie JSON.

**Odpowiedź:** `application/json`

```json
{
    "state": "idle",
    "speed": 50,
    "passes": 1,
    "currentPass": 0,
    "freeHeap": 245760,
    "uptime": 3600,
    "firmware": "1.0.0",
    "clients": 1,
    "elapsed": 0
}
```

**Pola:**

| Pole | Typ | Opis |
|------|-----|------|
| `state` | string | Stan maszyny: `idle`, `running`, `paused`, `stopped`, `error` |
| `speed` | int | Prędkość malowania 0-100 [%] |
| `passes` | int | Ustawiona liczba przejść |
| `currentPass` | int | Aktualny numer przejazdu |
| `freeHeap` | int | Wolna pamięć RAM [bajty] |
| `uptime` | int | Czas pracy od uruchomienia [sekundy] |
| `firmware` | string | Wersja firmware |
| `clients` | int | Liczba podłączonych klientów WiFi |
| `elapsed` | int | Czas trwania malowania [sekundy] |

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
| `set_speed` | 0-100 | Ustaw prędkość malowania [%] |
| `set_passes` | 1-99 | Ustaw liczbę przejść |

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

# Zapauzuj
curl -X POST -d "action=pause" http://192.168.4.1/api/control

# Zatrzymaj
curl -X POST -d "action=stop" http://192.168.4.1/api/control

# Ustaw prędkość na 75%
curl -X POST -d "action=set_speed&value=75" http://192.168.4.1/api/control

# Ustaw 3 przejazdy
curl -X POST -d "action=set_passes&value=3" http://192.168.4.1/api/control
```

## Kody odpowiedzi HTTP

| Kod | Opis |
|-----|------|
| 200 | Sukces |
| 400 | Brak wymaganego parametru |
| 404 | Nieznany endpoint |

## Autorefresh panelu WWW

Panel HTML automatycznie odpytuje `/api/status` co 1 sekundę za pomocą JavaScript `fetch()`. Dane są aktualizowane w interfejsie bez przeładowania strony.
