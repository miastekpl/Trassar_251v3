# TrassarV3 - Komputer pokładowy malowarki pasów drogowych

Firmware komputera pokładowego malowarki pasów drogowych oparty na platformie **ESP32-S3 N16R8**.
Obsługuje **6 pistoletów natryskowych**, **16 wzorców malowania** (15 normowych + własny) zgodnych z polskimi normami oznakowania drogowego, **3 tryby pracy** (automatyczny, półautomatyczny, ręczny), kalibrację enkodera i obliczanie powierzchni malowanej.

## Funkcje

- **6 pistoletów natryskowych** (P1-P6) sterowanych przekaźnikami
- **16 wzorców malowania** (P-1a...P-7d + własny) - polskie normy oznakowania
- **3 tryby pracy** - automatyczny, półautomatyczny, ręczny
- **Wzorzec własny** - definiowany przez operatora z panelu WWW, 3 sloty pamięci, zapis do NVS
- **Kalibracja enkodera** - procedura 10m z zapisem do NVS
- **Obliczanie powierzchni** - na podstawie dystansu i szerokości pistoletów
- **Przełączanie wzorców Smart/Instant** - wybór: dokończ cykl (Smart) lub zmień natychmiast (Instant)
- **Odwracanie wzorców P-3a/P-3b** (zamiana ciągła ↔ przerywana)
- **Wyświetlacz TFT 2.8" ILI9341** (240x320, SPI) - 11 ekranów interfejsu
- **Serwer WWW na Core 0** - panel sterowania przez WiFi AP (nie blokuje krytycznej pętli)
- **Menu serwisowe WWW** - statystyki lifetime, lista raportów SD, wskaźniki anomalii pistoletów
- **Wybór trybu pracy z WWW i z urządzenia** - selektor trybu AUTO/SEMI/RĘCZNY
- **Edytor wzorca własnego w WWW** - konfiguracja 6 pistoletów, kreska/przerwa, zapis do NVS
- **Zegar RTC DS1307** - czas rzeczywisty z podtrzymaniem bateryjnym
- **Statystyki** - sesja + łączne (dystans, powierzchnia, czas pracy)
- **Pamięć trwała NVS** - kalibracja, statystyki, ostatni wzorzec, próg prędkości, tryb pracy
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
- **Mutex Core 0/1** - spinlock na g_state eliminujący race conditions między rdzeniami
- **Wersjonowanie NVS** - automatyczna migracja danych przy aktualizacji firmware
- **3 sloty wzorców własnych** - 3 niezależne presety zamiast jednego
- **Eksport raportów CSV** - pobieranie plików CSV bezpośrednio z panelu WWW
- **Licznik strzałów pistoletów** - lifetime count per pistolet (planowanie serwisu dysz)
- **Podgląd wzorca Canvas** - multi-gun wizualizacja kreska/przerwa z szerokościami na panelu WWW
- **Moduł GPS GY-NEO6MV2** - pozycja, prędkość GPS, satelity, HDOP — dane w WWW, API i raportach CSV
- **Ekran przygotowania (SETUP)** - tryb pracy + Smart/Instant + start normalny/od przerwy — jeden ekran, bez telefonu
- **Reset etapu (sesji)** - zerowanie liczników sesji z menu serwisowego po zakończeniu etapu pracy
- **Stałe layoutu wyświetlacza** - ~40 nazwanych `#define` zamiast magic numbers, łatwiejsza konserwacja UI
- **Joystick analogowy KY-023** - nawigacja menu góra/dół/lewo/prawo + przycisk, auto-repeat, uzupełnia fizyczne przyciski
- **Ekran podsumowania etapu** - po zatrzymaniu malowania wyświetla statystyki (dystans, powierzchnia, czas, śr. prędkość, GPS) z opcjami: kontynuuj / nowy etap / HOME

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
| WŁASNY | Wzorzec własny | definiowany | P1-P6 | — |

## Tryby pracy

| Tryb | Opis |
|------|------|
| **Automatyczny (AUTO)** | Pistolety sterowane automatycznie wg wzorca (kreska/przerwa na podstawie dystansu) |
| **Półautomatyczny (SEMI)** | Linia malowana automatycznie do pełnej długości, przerwa kontrolowana przez operatora (START = kolejna linia) |
| **Ręczny (MANUAL)** | Pistolety aktywne tylko gdy operator trzyma przycisk START |

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
| GPS GY-NEO6MV2 | Moduł GPS NEO-6M z anteną (UART2) |
| Joystick KY-023 | Joystick analogowy 2-osiowy + przycisk (ADC1) |

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
│   ├── patterns.h/cpp          # 16 wzorców malowania (15 + własny)
│   ├── guns.h/cpp              # Kontroler 6 przekaźników
│   ├── encoder_distance.h/cpp  # Pomiar dystansu, prędkości, kalibracja
│   ├── painting_engine.h/cpp   # Silnik malowania (maszyna stanów)
│   ├── statistics.h/cpp        # Statystyki (sesja + lifetime)
│   ├── storage.h/cpp           # Pamięć trwała NVS (Preferences)
│   ├── display_manager.h/cpp   # Obsługa wyświetlacza (11 ekranów, stałe layoutu)
│   ├── button_handler.h/cpp    # Obsługa przycisków BS-33B
│   ├── rtc_handler.h/cpp       # Obsługa zegara RTC DS1307
│   ├── web_server.h/cpp        # Serwer WWW (WiFi AP + REST API)
│   ├── menu.h/cpp              # System menu (nawigacja 11 ekranów)
│   ├── buzzer.h/cpp            # Sygnalizacja dźwiękowa (LEDC PWM)
│   ├── gps_handler.h/cpp      # Obsługa GPS NEO-6M (UART2, TinyGPS++)
│   └── joystick.h/cpp         # Joystick analogowy KY-023 (ADC + przycisk)
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

Aktualna wersja firmware: **v2.15.0**

## Rekomendacje rozwoju

Poniżej lista rekomendowanych usprawnień i nowych funkcji, które warto rozważyć w kolejnych wersjach:

### Architektura i kod
- **Migracja z `#define` na `constexpr`** — zastąpienie makr preprocessora typowanymi stałymi C++17 (lepsza diagnostyka kompilatora, namespace'y)
- **Refaktoring display_manager na klasy ekranów** — każdy ekran jako osobna klasa dziedzicząca po `Screen`, eliminacja rozbudowanego switch/case w `menu.cpp`
- **Event system (kolejka zdarzeń)** — zamiast bezpośredniego wywoływania handlerów z `loop()`, kolejka `xQueueSend` między Core 0 (WWW) a Core 1 (logika)
- **OTA (Over-The-Air) update** — aktualizacja firmware przez WiFi bez kabla USB, z panelu WWW
- **Unit testy** — testy logiki `painting_engine`, `patterns`, `statistics` na hoście x86 (PlatformIO native)

### Interfejs i UX
- **Joystick analogowy (KY-023)** — zastąpienie sekwencji SELEKTOR/STOP jednym joystickiem do nawigacji menu (góra/dół/lewo/prawo + przycisk)
- **Ekran podsumowania etapu** — po STOP wyświetlanie podsumowania: dystans, powierzchnia, czas, wzorzec, GPS — z opcją "Kontynuuj" lub "Nowy etap"
- **Podgląd na żywo wzorca na TFT** — wizualizacja kreska/przerwa na wyświetlaczu (obecnie tylko w panelu WWW)
- **Jasność wyświetlacza** — regulacja z panelu WWW lub menu serwisowego (obecnie stała wartość PWM)
- **Dźwięki konfigurowalne** — włączanie/wyłączanie poszczególnych sygnałów buzzera z panelu WWW

### Pomiary i precyzja
- **Podwójny enkoder (kwadraturowy)** — wykorzystanie obu kanałów A+B dla x2/x4 rozdzielczości (lepsza precyzja kresek)
- **Fuzja GPS + enkoder** — korekcja dryfu enkodera na podstawie dystansu GPS na długich odcinkach
- **Automatyczna kalibracja z GPS** — kalibracja impulsów/metr na podstawie dystansu GPS (bez taśmy mierniczej)
- **Zapis trasy GPS (GPX/KML)** — ciągły zapis koordynatów podczas malowania, eksport pliku trasy

### Komunikacja i integracja
- **Bluetooth Low Energy (BLE)** — komunikacja z tabletem/telefonem bez WiFi (mniejsze zużycie energii)
- **MQTT / IoT** — wysyłanie danych do chmury (monitoring floty maszyn, dashboard operatora)
- **Integracja z systemami GIS** — eksport raportów z GPS do formatów GIS (GeoJSON, Shapefile)
- **REST API v2 z WebSocket** — push notifications zamiast pollingu co 1 s (mniejszy ruch, szybsza reakcja panelu)

### Bezpieczeństwo i niezawodność
- **Backup NVS na SD** — periodyczny eksport ustawień NVS na kartę SD (odzyskiwanie po awarii Flash)
- **Podwójny watchdog** — osobny WDT dla Core 0 (serwer WWW) obok istniejącego na Core 1
- **Szyfrowanie WiFi WPA2-Enterprise** — dla zastosowań komercyjnych z wieloma maszynami
- **Log zdarzeń na SD** — chronologiczny log startów/stopów/błędów/anomalii (poza raportami CSV)

### Sprzęt
- **Czujnik poziomu farby** — ultradźwiękowy lub pływakowy, alarm niskiego poziomu w panelu
- **Czujnik temperatury farby** — kontrola lepkości farby drogowej (ważne dla farb termoplastycznych)
- **Czujnik ciśnienia w układzie** — monitoring ciśnienia w linii natryskowej
- **Moduł GSM/LTE (SIM800L / SIM7600)** — zdalna telemetria poza zasięgiem WiFi

## Licencja

Projekt prywatny. Wszelkie prawa zastrzeżone.
