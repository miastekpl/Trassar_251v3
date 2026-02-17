# TrassarV3 - Komputer pokładowy malowarki pasów drogowych

Firmware komputera pokładowego malowarki pasów drogowych oparty na platformie **ESP32-S3 N16R8**.
Obsługuje **6 pistoletów natryskowych**, **15 wzorców malowania** zgodnych z polskimi normami oznakowania drogowego, kalibrację enkodera i obliczanie powierzchni malowanej.

## Funkcje

- **6 pistoletów natryskowych** (P1-P6) sterowanych przekaźnikami
- **15 wzorców malowania** (P-1a...P-7d) - polskie normy oznakowania
- **Kalibracja enkodera** - procedura 10m z zapisem do NVS
- **Obliczanie powierzchni** - na podstawie dystansu i szerokości pistoletów
- **Inteligentne przełączanie wzorców** (Smart Switch) - dokończ cykl przed zmianą
- **Odwracanie wzorców P-3a/P-3b** (zamiana ciągła ↔ przerywana)
- **Wyświetlacz TFT 2.8" ILI9341** (240x320, SPI) - 9 ekranów interfejsu
- **Serwer WWW na Core 0** - panel sterowania przez WiFi AP (nie blokuje krytycznej pętli)
- **Menu serwisowe WWW** - statystyki lifetime, lista raportów SD, wskaźniki anomalii pistoletów
- **Zegar RTC DS1307** - czas rzeczywisty z podtrzymaniem bateryjnym
- **Statystyki** - sesja + łączne (dystans, powierzchnia, czas pracy)
- **Pamięć trwała NVS** - kalibracja, statystyki, ostatni wzorzec, próg prędkości
- **Watchdog timer** (3 s) - auto-reset ESP32 przy zawieszeniu loop()
- **Gun keepalive** (300 ms) - awaryjne wyłączenie pistoletów przy braku update
- **Alarm przekroczenia prędkości** - migający wyświetlacz + buzzer, próg konfigurowalny z WWW
- **Buzzer** (GPIO 8) - sygnalizacja dźwiękowa: start/stop, niska prędkość, overspeed, błędy
- **Dual-core FreeRTOS** - Core 0: WiFi/HTTP, Core 1: enkoder/pistolety/buzzer/display
- **Anti-flicker** - ekrany TFT bez migania (setTextPadding na wszystkich ekranach)
- **Diagnostyka** - heap monitoring, stack HWM, fragmentacja, logi co 30 s
- **Okresowy zapis statystyk** - lifetime stats co 60 s (ochrona przed utratą danych)
- **Szybki ISR enkodera** - bezpośredni odczyt rejestru GPIO (~50 ns vs ~2 μs)
- **API statystyk lifetime** (`/api/stats`) - dystans, powierzchnia, czas pracy przez WWW
- **API raportów SD** (`/api/reports`) - lista plików raportów CSV przez WWW
- **Detekcja anomalii pistoletów** - alert gdy pistolet nie strzela mimo aktywnej konfiguracji

## Pistolety i ich zastosowanie

| Pistolet | Szerokość | Opis | GPIO |
|----------|-----------|------|------|
| **P1** | 12 cm | Oś jezdni - lewy | 41 |
| **P2** | 12 cm | Oś jezdni - środek | 42 |
| **P3** | 12 cm | Oś jezdni - prawy | 1 |
| **P4** | 24 cm | Oś jezdni - szeroki | 2 |
| **P5** | 12 cm | Krawędź - wąska | 3 |
| **P6** | 24 cm | Krawędź - szeroka | 4 |

## Wzorce malowania

| Wzorzec | Nazwa | Kreska/Przerwa | Pistolet | Szer. |
|---------|-------|----------------|----------|-------|
| P-1a | Przerywana długa | 6m / 6m | P2 | 12cm |
| P-1b | Przerywana krótka | 3m / 3m | P2 | 12cm |
| P-1c | Wydzielająca | 3m / 1.5m | P2 | 12cm |
| P-1d | Prowadząca wąska | 1m / 1m | P2 | 12cm |
| P-1e | Prowadząca szeroka | 1m / 1m | P4 | 24cm |
| P-2a | Ciągła wąska | ciągła | P2 | 12cm |
| P-2b | Ciągła szeroka | ciągła | P4 | 24cm |
| P-3a | Przekraczalna długa | ciągła + 6m/6m | P1+P3 | 12cm |
| P-3b | Przekraczalna krótka | ciągła + 3m/3m | P1+P3 | 12cm |
| P-4 | Podwójna ciągła | ciągła + ciągła | P1+P3 | 12cm |
| P-6 | Ostrzegawcza | 1m / 1m | P5 | 12cm |
| P-7a | Krawędziowa przeryw. szer. | 1m / 2m | P6 | 24cm |
| P-7b | Krawędziowa ciągła szer. | ciągła | P6 | 24cm |
| P-7c | Krawędziowa przeryw. wąska | 1m / 2m | P5 | 12cm |
| P-7d | Krawędziowa ciągła wąska | ciągła | P5 | 12cm |

## Komponenty sprzętowe

| Komponent | Opis |
|-----------|------|
| ESP32-S3 N16R8 | Płytka deweloperska, 16MB Flash, 8MB PSRAM |
| ILI9341 2.8" | Wyświetlacz LCD 240x320 SPI |
| DS1307 | Zegar RTC z baterią |
| BS-33B x3 | Przyciski monostabilne (Start/Pauza, Stop, Selektor) |
| Enkoder obrotowy | Pomiar dystansu + nawigacja menu |
| Moduły przekaźnikowe x6 | Sterowanie pistoletami P1-P6 |
| Buzzer pasywny | Sygnalizacja dźwiękowa (GPIO 8) |

## Podłączenie WiFi

- **SSID:** `TrassarV3`
- **Hasło:** `12345678`
- **IP panelu:** `192.168.4.1`
- **URL:** `http://192.168.4.1`

## Kompilacja

Projekt wykorzystuje **PlatformIO**. Aby skompilować:

```bash
# Instalacja PlatformIO CLI (jeśli brak)
pip install platformio

# Kompilacja
pio run

# Upload na ESP32-S3
pio run --target upload

# Monitor szeregowy
pio device monitor
```

## Struktura projektu

```
TrassarV3/
├── platformio.ini              # Konfiguracja PlatformIO
├── src/
│   ├── main.cpp                # Główny plik programu (11 modułów)
│   ├── config.h                # Definicje pinów, enumów, struktur
│   ├── patterns.h/cpp          # 15 wzorców malowania
│   ├── guns.h/cpp              # Kontroler 6 przekaźników
│   ├── encoder_distance.h/cpp  # Pomiar dystansu, prędkości, kalibracja
│   ├── painting_engine.h/cpp   # Silnik malowania (maszyna stanów)
│   ├── statistics.h/cpp        # Statystyki (sesja + lifetime)
│   ├── storage.h/cpp           # Pamięć trwała NVS (Preferences)
│   ├── display_manager.h/cpp   # Obsługa wyświetlacza (9 ekranów)
│   ├── button_handler.h/cpp    # Obsługa przycisków BS-33B
│   ├── rtc_handler.h/cpp       # Obsługa zegara RTC DS1307
│   ├── web_server.h/cpp        # Serwer WWW (WiFi AP + REST API)
│   ├── menu.h/cpp              # System menu (nawigacja 9 ekranów)
│   └── buzzer.h/cpp            # Sygnalizacja dźwiękowa (LEDC PWM)
├── docs/
│   ├── INSTRUKCJA_OBSLUGI.md   # Instrukcja obsługi
│   ├── SCHEMAT_PODLACZEN.md    # Schemat podłączeń
│   └── API_WWW.md              # Dokumentacja API serwera
├── CHANGELOG.md                # Historia zmian
└── README.md                   # Ten plik
```

## Dokumentacja

- [Instrukcja obsługi](docs/INSTRUKCJA_OBSLUGI.md)
- [Schemat podłączeń](docs/SCHEMAT_PODLACZEN.md)
- [API serwera WWW](docs/API_WWW.md)
- [Historia zmian](CHANGELOG.md)

## Wersja

Aktualna wersja firmware: **v2.8.0**

## Licencja

Projekt prywatny. Wszelkie prawa zastrzeżone.
