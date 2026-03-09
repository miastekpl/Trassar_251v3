# TrassarV3 - Instrukcja obsługi v2.23.0

## Spis treści

0. [SZYBKI START — pierwsze uruchomienie](#0-szybki-start--pierwsze-uruchomienie)
1. [Opis ogólny](#1-opis-ogólny)
2. [Dane techniczne](#2-dane-techniczne)
3. [Panel sterowania](#3-panel-sterowania)
4. [Pistolety natryskowe](#4-pistolety-natryskowe)
5. [Wzorce malowania](#5-wzorce-malowania)
6. [Tryby pracy](#6-tryby-pracy)
7. [Ekrany interfejsu](#7-ekrany-interfejsu)
8. [Funkcja "Start od przerwy"](#8-funkcja-start-od-przerwy)
9. [Panel WWW (zdalny dostęp)](#9-panel-www-zdalny-dostęp)
10. [Kalibracja enkodera](#10-kalibracja-enkodera)
11. [Raporty na karcie SD](#11-raporty-na-karcie-sd)
12. [Zabezpieczenia](#12-zabezpieczenia)
13. [Sygnalizacja dźwiękowa (buzzer)](#13-sygnalizacja-dźwiękowa-buzzer)
14. [Architektura wielordzeniowa](#14-architektura-wielordzeniowa)
15. [Diagnostyka systemowa](#15-diagnostyka-systemowa)
16. [Detekcja anomalii pistoletów](#16-detekcja-anomalii-pistoletów)
17. [API statystyk i raportów SD](#17-api-statystyk-i-raportów-sd)
18. [Menu serwisowe w panelu WWW](#18-menu-serwisowe-w-panelu-www)
19. [Moduł GPS](#19-moduł-gps)
20. [Fizyczne przyciski wzorców (MCP23017)](#20-fizyczne-przyciski-wzorców-mcp23017)
21. [Przykłady zastosowania](#21-przykłady-zastosowania)
22. [Rozwiązywanie problemów](#22-rozwiązywanie-problemów)
36. [Bezpieczeństwo i BHP](#36-bezpieczeństwo-i-bhp)
37. [Konserwacja i przeglądy](#37-konserwacja-i-przeglądy)
38. [Aktualizacja firmware](#38-aktualizacja-firmware)
39. [Specyfikacja zgodności z normami drogowymi](#39-specyfikacja-zgodności-z-normami-drogowymi)
40. [Słownik pojęć](#40-słownik-pojęć)
41. [Karta gwarancyjna i dane kontaktowe](#41-karta-gwarancyjna-i-dane-kontaktowe)

---

## 0. SZYBKI START — pierwsze uruchomienie

### Krok 1: Montaż sprzętowy
Przed pierwszym uruchomieniem upewnij się, że:
- Wszystkie moduły są podłączone wg schematu podłączeń (SCHEMAT_PODLACZEN.md)
- Karta MicroSD (FAT32, min. 1 GB) jest włożona do slotu w module ILI9341
- Bateria CR2032 jest włożona do modułu RTC DS1307
- Koło pomiarowe z enkoderem jest zamontowane i obraca się swobodnie
- Antena GPS jest zamontowana z widocznością nieba (na zewnątrz kabiny)

### Krok 2: Pierwsze włączenie
1. Podłącz zasilacz USB-C (min. 5V/1.5A) do ESP32-S3
2. Poczekaj ~3 s na uruchomienie — pojawi się ekran powitalny **TrassarV3**
3. Ekran **POST** (diagnostyka) pokaże status wszystkich modułów:
   - **SD: OK** / FAIL — czy karta SD jest zamontowana
   - **RTC: OK** / FAIL — czy zegar działa
   - **GPS: BRAK** — normalne przy pierwszym uruchomieniu (cold start 30–60 s)
   - **MCP: OK** / FAIL — czy ekspander przycisków odpowiada
   - **ENK: Domyślny** — normalne, wymaga kalibracji
4. Naciśnij **START** lub poczekaj 5 s — system przejdzie na ekran HOME

### Krok 3: Kalibracja enkodera (OBOWIĄZKOWA przed pierwszym malowaniem!)
1. Odmierz na podłożu dokładnie **10 metrów** taśmą mierniczą
2. Przytrzymaj **STOP (1 s)** → menu serwisowe
3. Wybierz "Kalibracja enkodera" → SELEKTOR (1 s)
4. Ustaw maszynę na początku odcinka → naciśnij **START**
5. Jedź maszyną dokładnie 10 m → naciśnij **START**
6. Sprawdź wynik: "Imp/metr" i "Status: OK"
7. Powrót: STOP (1 s) → STOP (1 s)

### Krok 4: Połączenie z panelem WWW
1. Na telefonie wyszukaj sieć WiFi **TrassarV3**
2. Hasło: **12345678**
3. Otwórz przeglądarkę → **http://192.168.4.1**
4. Panel sterowania ładuje się automatycznie

### Krok 5: Pierwsze malowanie
1. W panelu WWW wybierz wzorzec (np. **P-1a**)
2. Na maszynie naciśnij **START**
3. Ruszaj — po 3 km/h pistolety zaczną malować automatycznie
4. **STOP** — zakończ, raport zapisuje się na kartę SD

### Skrócona mapa przycisków

```
┌────────────────────────────────────────────────────────────┐
│                   MAPA SZYBKICH KOMEND                     │
├────────────────────────────────────────────────────────────┤
│  Na ekranie HOME:                                         │
│    START (krótko)     → Rozpocznij malowanie               │
│    START (1.5 s)      → Ekran SETUP (tryb/smart/start)     │
│    GAP (GPIO 7)       → Start od przerwy                   │
│    SELEKTOR           → Odwróć P-3a/P-3b                   │
│    SELEKTOR (1.5 s)   → Przełącz Smart/Instant             │
│    STOP (1.5 s)       → Menu serwisowe                     │
│    START + STOP (1 s) → Combo: menu serwisowe              │
│                                                            │
│  Podczas malowania:                                        │
│    START              → Pauza (AUTO) / Kolejna linia (SEMI)│
│    STOP               → Zatrzymaj + raport                 │
│    SELEKTOR           → Odwróć P-3a/P-3b                   │
│                                                            │
│  W menu serwisowym:                                        │
│    SELEKTOR (krótko)  → Następna pozycja                   │
│    STOP (krótko)      → Poprzednia pozycja                 │
│    SELEKTOR (1.5 s)   → Wejdź w opcję                     │
│    STOP (1.5 s)       → Powrót                             │
│                                                            │
│  Joystick (alternatywa):                                   │
│    Góra/Dół           → Nawigacja w menu                   │
│    Prawo              → Wejdź / zmień                      │
│    Lewo               → Cofnij                             │
│    Przycisk           → Potwierdź                          │
└────────────────────────────────────────────────────────────┘
```

---

## 1. Opis ogólny

**TrassarV3** to komputer pokładowy malowarki pasów drogowych oparty na mikrokontrolerze ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM). System steruje **6 pistoletami natryskowymi** (P1–P6) poprzez moduły przekaźnikowe, obsługuje **16 wzorców malowania** (15 normowych + własny) zgodnych z polskimi normami oznakowania poziomego dróg, oferuje **3 tryby pracy** (automatyczny, półautomatyczny, ręczny) i mierzy dystans oraz prędkość za pomocą enkodera obrotowego montowanego na kole pomiarowym.

System zapewnia:
- Trzy tryby pracy: automatyczny, półautomatyczny i ręczny — wybierane z urządzenia lub panelu WWW
- Automatyczne sterowanie pistoletami w czasie malowania (ciągłe, przerywane, mieszane)
- Wzorzec własny — definiowany przez operatora z panelu WWW, z dowolną konfiguracją pistoletów
- Odwracanie wzorców przekraczalnych P-3a i P-3b jednym naciśnięciem przycisku
- Start od przerwy — kontynuację istniejącego oznakowania od punktu, w którym powinna być przerwa
- Zabezpieczenie prędkości minimalnej 3 km/h (pistolety nie włączą się na postoju)
- Rejestrację raportów CSV na karcie SD z datą, wzorcem, dystansem i powierzchnią
- Zdalny panel sterowania WWW dostępny przez WiFi z telefonu lub laptopa

---

## 2. Dane techniczne

| Parametr | Wartość |
|----------|---------|
| Mikrokontroler | ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM) |
| Firmware | v2.23.0 |
| Wyświetlacz | ILI9341 2.8" TFT, 320×240 px, tryb landscape |
| Interfejs SPI | HSPI (SPI3), 27 MHz |
| Zegar RTC | DS1307 z baterią CR2032 |
| Enkoder | Obrotowy, ISR na pinie CLK (CHANGE) |
| Przyciski | 4 szt. monostabilne (START, STOP, SELEKTOR, GAP) |
| Joystick | KY-023 analogowy 2-osiowy + przycisk (ADC1, GPIO 9/10/11) |
| Przekaźniki | 6 szt. (pistolety P1–P6), logika HIGH = ON |
| Buzzer | Pasywny, GPIO 8, LEDC PWM kanał 1 |
| Karta SD | Slot zintegrowany w module wyświetlacza, FAT32 |
| WiFi | Access Point, SSID: TrassarV3, hasło: 12345678 |
| Serwer WWW | HTTP port 80, max 4 klientów, auto-refresh 1 s |
| Prędkość min. | 3 km/h (zabezpieczenie pistoletów) |
| Prędkość maks. | Domyślnie 15 km/h (alarm, konfigurowalny z WWW) |
| Watchdog | 3 s timeout, auto-reset ESP32 |
| Gun keepalive | 300 ms — awaryjne wyłączenie pistoletów |
| Kalibracja | Odcinek 10 m, zapis do NVS |
| GPS | GY-NEO6MV2 (NEO-6M), UART2, 9600 baud |
| Przyciski wzorców | 15 szt. via MCP23017 I2C (ekspander 16-bit, adres 0x20) |
| Zasilanie | USB-C 5V (ESP32-S3 DevKit) |

---

## 3. Panel sterowania

### 3.1 Przyciski funkcyjne

System posiada **4 przyciski fizyczne**. Każdy przycisk obsługuje krótkie naciśnięcie (klik) oraz — w wybranych przypadkach — długie naciśnięcie (przytrzymanie 1 s).

| Przycisk | GPIO | Krótkie naciśnięcie | Długie naciśnięcie (1 s) |
|----------|------|---------------------|--------------------------|
| **START** | 38 | Start malowania / Pauza / Wznowienie | Wybór trybu pracy (na ekranie HOME w bezruchu) |
| **STOP** | 39 | Zatrzymanie malowania / Cofnij w menu | Wejście w menu serwisowe / Powrót |
| **SELEKTOR** | 40 | *Zależy od ekranu (patrz niżej)* | HOME: przełącz Smart/Instant; Menu: wejdź w opcję |
| **GAP (od przerwy)** | 7 | Start od przerwy (na ekranie HOME) | — |

### 3.2 Funkcja selektora w zależności od ekranu

Przycisk **SELEKTOR** pełni różne funkcje w zależności od aktualnie wyświetlanego ekranu:

| Ekran | Krótkie naciśnięcie | Długie naciśnięcie (1 s) |
|-------|---------------------|--------------------------|
| **Ekran główny (HOME)** | Odwróć wzorzec (tylko P-3a / P-3b)* | **Przełącz tryb Smart/Instant** (zmiana wzorców) |
| **Ekran malowania** | Odwróć wzorzec (tylko P-3a / P-3b)* | — |
| **Menu serwisowe** | **Następna pozycja w menu** | **Wejdź w wybraną opcję** |
| **Czyszczenie dysz** | Następny wzorzec | Poprzedni wzorzec |

> \* Na ekranie głównym i ekranie malowania selektor służy **wyłącznie** do odwracania wzorców P-3a i P-3b. Dla pozostałych wzorców krótkie naciśnięcie jest ignorowane. Zmiana wzorca odbywa się wyłącznie przez panel WWW.

### 3.3 Enkoder obrotowy

Enkoder obrotowy (piny CLK = GPIO 5, DT = GPIO 6) służy **wyłącznie** do pomiaru dystansu i prędkości. **Nie jest używany do nawigacji ani sterowania interfejsem.** Obrót enkodera jest rejestrowany przez przerwanie sprzętowe (ISR) na pinie CLK.

Wbudowany przycisk enkodera (pin SW = GPIO 7) pełni funkcję dedykowanego przycisku **"Start od przerwy"**.

### 3.4 Joystick analogowy KY-023

System posiada joystick analogowy **KY-023** (piny VRx = GPIO 19, VRy = GPIO 20, SW = GPIO 46) umożliwiający intuicyjną nawigację po menu i ekranach bez użycia przycisków.

**Mapowanie joysticka na zdarzenia przycisków:**

| Ruch joysticka | Odpowiednik przycisku | Funkcja |
|----------------|----------------------|---------|
| **Góra** | STOP (krótko) | Poprzednia pozycja w menu |
| **Dół** | SELEKTOR (krótko) | Następna pozycja w menu |
| **Prawo** | SELEKTOR (1 s) | Wejdź / zmień wartość |
| **Lewo** | STOP (1 s) | Cofnij / powrót |
| **Przycisk SW (krótko)** | SELEKTOR (1 s) | Wejdź / potwierdź |
| **Przycisk SW (1 s)** | START (1 s) | Np. otwórz ekran SETUP z HOME |

**Cechy:**
- **Auto-repeat** — przytrzymanie góra/dół automatycznie powtarza zdarzenie (opóźnienie 400 ms, powtarzanie co 200 ms)
- **Bez auto-repeat** — ruch lewo/prawo generuje zdarzenie jednorazowo (zapobieganie przypadkowemu wielokrotnemu cofaniu/wchodzeniu)
- **Strefa martwa** — ±500 z centrum ADC (2048) eliminuje przypadkowe ruchy
- **Współpraca z przyciskami** — joystick uzupełnia fizyczne przyciski, nie zastępuje ich

> **Wskazówka:** Joystick jest szczególnie wygodny do szybkiej nawigacji po menu serwisowym i ekranie SETUP. Przyciski fizyczne nadal działają normalnie.

---

## 4. Pistolety natryskowe

### 4.1 Opis pistoletów

Maszyna obsługuje 6 niezależnych pistoletów natryskowych sterowanych przekaźnikami:

| Pistolet | Szerokość pasa | GPIO | Zastosowanie |
|----------|---------------|------|-------------|
| **P1** | 12 cm | 41 | Oś jezdni — lewy |
| **P2** | 12 cm | 42 | Oś jezdni — środek |
| **P3** | 12 cm | 1 | Oś jezdni — prawy |
| **P4** | 24 cm | 2 | Oś jezdni — szeroki |
| **P5** | 12 cm | 3 | Krawędź — wąska |
| **P6** | 24 cm | 4 | Krawędź — szeroka |

### 4.2 Przypisanie pistoletów do wzorców

Każdy wzorzec malowania aktywuje określony zestaw pistoletów:

- **P-1a, P-1b, P-1c, P-1d** → P2 (przerywane, 12 cm)
- **P-1e** → P4 (przerywana szeroka, 24 cm)
- **P-2a** → P2 (ciągła wąska, 12 cm)
- **P-2b** → P4 (ciągła szeroka, 24 cm)
- **P-3a, P-3b** → P1 + P3 jednocześnie (ciągła + przerywana) — **ODWRACALNE**
- **P-4** → P1 + P3 (podwójna ciągła)
- **P-6** → P5 (ostrzegawcza)
- **P-7a** → P6 (krawędziowa przerywana szeroka, 24 cm)
- **P-7b** → P6 (krawędziowa ciągła szeroka, 24 cm)
- **P-7c** → P5 (krawędziowa przerywana wąska, 12 cm)
- **P-7d** → P5 (krawędziowa ciągła wąska, 12 cm)

### 4.3 Kolory wskaźników pistoletów na wyświetlaczu

Na dole ekranu wyświetlane jest 6 prostokątów reprezentujących pistolety P1–P6:

| Kolor prostokąta | Znaczenie |
|-------------------|-----------|
| **Żółty** | Pistolet przypisany do wzorca (gotowy do malowania) |
| **Zielony** | Pistolet aktualnie maluje (strzela farbą) |
| **Żółty migający** (500 ms) | Pauza — pistolet we wzorcu, ale malowanie wstrzymane |
| **Szary** | Pistolet nieużywany w aktualnym wzorcu |

---

## 5. Wzorce malowania

### 5.1 Tabela wzorców

System obsługuje 16 wzorców (15 normowych + wzorzec własny) zgodnych z polskimi normami oznakowania poziomego:

| Kod | Nazwa | Pistolet | Typ | Kreska [m] | Przerwa [m] | Szer. |
|-----|-------|----------|-----|------------|-------------|-------|
| **P-1a** | Przerywana długa | P2 | DASHED | 4.0 | 8.0 | 12 cm |
| **P-1b** | Przerywana krótka | P2 | DASHED | 2.0 | 4.0 | 12 cm |
| **P-1c** | Wydzielająca | P2 | DASHED | 2.0 | 2.0 | 12 cm |
| **P-1d** | Prowadząca wąska | P2 | DASHED | 1.0 | 1.0 | 12 cm |
| **P-1e** | Prowadząca szeroka | P4 | DASHED | 1.0 | 1.0 | 24 cm |
| **P-2a** | Ciągła wąska | P2 | CONTINUOUS | — | — | 12 cm |
| **P-2b** | Ciągła szeroka | P4 | CONTINUOUS | — | — | 24 cm |
| **P-3a** | Przekraczalna długa | P1+P3 | MIXED | P1: ciągła, P3: 4.0/2.0 | — | 12 cm |
| **P-3b** | Przekraczalna krótka | P1+P3 | MIXED | P1: ciągła, P3: 1.0/1.0 | — | 12 cm |
| **P-4** | Podwójna ciągła | P1+P3 | CONTINUOUS | — | — | 24 cm |
| **P-6** | Ostrzegawcza | P5 | DASHED | 4.0 | 2.0 | 12 cm |
| **P-7a** | Krawędziowa przeryw. szer. | P6 | DASHED | 1.0 | 1.0 | 24 cm |
| **P-7b** | Krawędziowa ciągła szer. | P6 | CONTINUOUS | — | — | 24 cm |
| **P-7c** | Krawędziowa przeryw. wąska | P5 | DASHED | 1.0 | 1.0 | 12 cm |
| **P-7d** | Krawędziowa ciągła wąska | P5 | CONTINUOUS | — | — | 12 cm |
| **WŁASNY** | Wzorzec własny | P1–P6 | Definiowany | Definiowany | Definiowany | — |

### 5.2 Wzorzec własny (WŁASNY)

Oprócz 15 predefiniowanych wzorców normowych, system umożliwia zdefiniowanie **wzorca własnego** z dowolną konfiguracją pistoletów, długością kreski i przerwy.

**Konfiguracja wzorca własnego:**
- Dostępna wyłącznie przez **panel WWW** (sekcja "Wzorzec własny")
- **3 niezależne sloty pamięci** (Slot 1/2/3) — każdy przechowuje oddzielny wzorzec
- Dla każdego pistoletu (P1–P6) można wybrać tryb: **Wyłączony**, **Ciągły** lub **Przerywany**
- Każdy pistolet ustawiony jako **Przerywany** ma własne, niezależne parametry: długość kreski i przerwy (0.1 – 50.0 m)
- Dzięki temu różne pistolety mogą malować z różnym wzorem (np. P2: 3m/2m, P5: 1m/1m)
- Wzorce zapisywane trwale w NVS — przetrwają restart urządzenia
- Przełączanie między slotami: kliknij zakładkę slotu w panelu WWW

**Ograniczenia:**
- Wzorzec własny nie jest dostępny przez przycisk SELEKTOR na urządzeniu — tylko przez panel WWW
- Wzorzec własny nie pojawia się w rotacji wzorców na urządzeniu

### 5.3 Odwracanie wzorców (P-3a, P-3b)

Wzorce przekraczalne P-3a i P-3b składają się z dwóch linii:
- **P1** — linia ciągła (domyślnie po lewej stronie maszyny)
- **P3** — linia przerywana (domyślnie po prawej stronie maszyny)

Funkcja odwracania zamienia role pistoletów P1 i P3:

| Tryb | P1 (lewy) | P3 (prawy) |
|------|-----------|------------|
| **Normalny** | Ciągła | Przerywana |
| **Odwrócony [ODW]** | Przerywana | Ciągła |

**Aktywacja odwracania:**
- **Na urządzeniu:** Krótkie naciśnięcie **SELEKTORA** na ekranie głównym lub ekranie malowania
- **W panelu WWW:** Przycisk "Odwróć" w sekcji wzorców

Na wyświetlaczu pojawia się znacznik **[ODW]** informujący o aktywnym odwróceniu.

> **Uwaga:** Przycisk SELEKTOR odwraca wzorzec tylko gdy aktywny jest wzorzec P-3a lub P-3b. Dla wszystkich pozostałych wzorców naciśnięcie selektora jest ignorowane.

---

## 6. Tryby pracy

System oferuje 3 tryby pracy, które określają sposób sterowania pistoletami podczas malowania.

### 6.1 Tryb automatyczny (AUTO)

Tryb domyślny. Pistolety sterowane są automatycznie na podstawie dystansu i konfiguracji wzorca:
- **Ciągły:** pistolet maluje nieprzerwanie (gdy prędkość ≥ 3 km/h)
- **Przerywany:** cykl kreska/przerwa obliczany automatycznie z dystansu (fmod)
- **Inteligentne przełączanie wzorców** — zmiana wzorca czeka na koniec bieżącego cyklu

### 6.2 Tryb półautomatyczny (SEMI)

Linia malowana automatycznie do pełnej długości kreski, ale **przerwa kontrolowana przez operatora**:
- Po namalowaniu pełnej kreski pistolety wyłączają się automatycznie + krótki sygnał buzzera
- Operator decyduje kiedy rozpocząć kolejną kreskę naciskając **START**
- Pistolety z trybem ciągłym (CONTINUOUS) działają normalnie (bez przerw)
- Idealne do malowania w trudnych warunkach (skrzyżowania, przejścia dla pieszych)

**Sygnalizacja:**
- Krótki beep (1000 Hz, 50 ms) — kreska zakończona, czekam na START
- Krótki beep (1500 Hz, 80 ms) — potwierdzenie rozpoczęcia kolejnej kreski

### 6.3 Tryb ręczny (MANUAL)

Operator ma pełną kontrolę nad pistoletami:
- Pistolety aktywne **tylko gdy operator trzyma przycisk START** i prędkość ≥ 3 km/h
- Puszczenie START natychmiast wyłącza pistolety
- Statystyki (dystans, powierzchnia) naliczane normalnie
- Pistolety wyłączone we wzorcu (GUN_OFF) pozostają wyłączone

### 6.4 Wybór trybu pracy

**Na urządzeniu:**
1. Na ekranie HOME (maszyna w bezruchu) przytrzymaj **START (1 s)**
2. Pojawi się ekran wyboru trybu z 3 opcjami: AUTO / SEMI / RĘCZNY
3. Klikaj **START (krótko)** aby przełączać między trybami
4. Przytrzymaj **START (1 s)** aby zatwierdzić wybór — buzzer potwierdzi
5. Naciśnij **STOP** aby anulować i wrócić do HOME

**W panelu WWW:**
- Sekcja "Tryb pracy" — 3 przyciski: AUTO / SEMI / RĘCZNY
- Aktywny tryb podświetlony na zielono
- Zmiana natychmiastowa z zapisem do NVS

Wybrany tryb jest zapisywany trwale w NVS — przetrwa restart urządzenia.

---

## 7. Ekrany interfejsu

### 7.1 Ekran główny (HOME)

Wyświetla się po uruchomieniu systemu. Ekran w trybie landscape (320×240 px):

```
┌──────────────────────────────────────────┐
│ P-1a              (duży)    12.5 (duży)  │  ← wzorzec / prędkość
│ Przerywana dluga            km/h         │  ← nazwa / jednostka
│ [ODW]                       45.2 m2      │  ← flaga / powierzchnia
│──────────────────────────────────────────│
│                                          │
│               Gotowy                     │  ← status (duży, zielony)
│                                          │
│  ┌─P1─┐ ┌─P2─┐ ┌─P3─┐ ┌─P4─┐ ┌─P5─┐ ┌─P6─┐ │
│  │    │ │████│ │    │ │    │ │    │ │    │ │  ← prostokąty pistoletów
│  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ │
└──────────────────────────────────────────┘
```

**Elementy ekranu:**
- **Lewy górny róg:** Kod wzorca (duża czcionka 24 pt), nazwa wzorca pod spodem, znacznik [ODW] jeśli wzorzec odwrócony
- **Prawy górny róg:** Prędkość w km/h (duża czcionka 24 pt), etykieta "km/h", powierzchnia malowania w m²
- **Środek:** Status systemu "Gotowy" (duża czcionka 18 pt, kolor zielony)
- **Dół:** 6 prostokątów pistoletów P1–P6 z kolorami wg stanu

**Dostępne akcje na ekranie HOME:**

| Przycisk | Akcja |
|----------|-------|
| **START (krótko)** | Rozpocznij malowanie od początku wzorca |
| **START (1 s)** | Ekran przygotowania (SETUP) — tryb, smart/instant, start |
| **GAP** (GPIO 7) | Start od przerwy — szybki start bez wchodzenia w SETUP |
| **SELEKTOR** | Odwróć wzorzec (tylko P-3a / P-3b) |
| **STOP (1 s)** | Wejdź do menu serwisowego |

Na ekranie wyświetlany jest aktualny tryb pracy: **[AUTO]**, **[SEMI]** lub **[RECZNY]**.

### 7.2 Ekran malowania (PAINTING)

Wyświetla się automatycznie po rozpoczęciu malowania. Układ identyczny jak HOME, ale z dynamicznymi informacjami:

```
┌──────────────────────────────────────────┐
│ P-3a              (duży)    8.3  (duży)  │
│ [ODW] [GAP]                 km/h         │  ← flagi stanu
│                             122.4 m2     │
│──────────────────────────────────────────│
│                                          │
│             Malowanie                    │  ← status (zielony)
│               lub                        │
│              Pauza                       │  ← status (żółty)
│                                          │
│  ┌─P1─┐ ┌─P2─┐ ┌─P3─┐ ┌─P4─┐ ┌─P5─┐ ┌─P6─┐ │
│  │████│ │    │ │████│ │    │ │    │ │    │ │
│  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ │
└──────────────────────────────────────────┘
```

**Elementy ekranu malowania:**
- **Wzorzec:** Kod wzorca dużą czcionką (24 pt)
- **Flagi:** [ODW] — wzorzec odwrócony, [GAP] — start od przerwy
- **Prędkość:** Aktualna prędkość maszyny (24 pt)
- **Powierzchnia:** Namalowana powierzchnia w m²
- **Status:** "Malowanie" (zielony) lub "Pauza" (żółty) — duża czcionka 18 pt w środku ekranu
- **Prostokąty pistoletów:** Zielone gdy malują, żółte migające na pauzie, szare gdy nieużywane

Na ekranie wyświetlany jest aktualny tryb pracy: **[AUTO]**, **[SEMI]** lub **[RECZNY]**.

**Sterowanie na ekranie malowania (zależy od trybu):**

| Przycisk | Tryb AUTO | Tryb SEMI | Tryb RĘCZNY |
|----------|-----------|-----------|-------------|
| **START** | Pauza / Wznowienie | Kolejna linia (po zakończeniu kreski) / Wznów z pauzy | Pistolety ON (trzymaj) |
| **STOP** | Zatrzymanie, zapis raportu | Zatrzymanie, zapis raportu | Zatrzymanie, zapis raportu |
| **SELEKTOR** | Odwróć (P-3a/P-3b) | Odwróć (P-3a/P-3b) | Odwróć (P-3a/P-3b) |

> **Zabezpieczenie:** Pistolety włączają się automatycznie dopiero po osiągnięciu prędkości **3 km/h**. Poniżej tej prędkości pistolety pozostają wyłączone nawet w stanie "Malowanie".

### 7.3 Ekran przygotowania (SETUP)

Ekran dostępny po przytrzymaniu **START (1 s)** na ekranie HOME (maszyna w bezruchu). Pozwala ustawić tryb pracy, tryb przełączania wzorców i rodzaj startu — wszystko w jednym miejscu, bez panelu WWW.

```
┌──────────────────────────────────────────┐
│            PRZYGOTOWANIE                 │
│                                          │
│  ► Tryb pracy:              AUTO         │
│    Przelaczanie:            Smart        │
│    Start:                   Normalny     │
│                                          │
│  SEL=dalej SEL(1s)=zmien START=maluj     │
│  STOP(1s)=wroc                           │
└──────────────────────────────────────────┘
```

**Opcje ekranu SETUP:**

| # | Opcja | Wartości (cykl) | Zapis NVS |
|---|-------|-----------------|-----------|
| 0 | **Tryb pracy** | AUTO → SEMI-AUTO → RECZNY → AUTO | Tak |
| 1 | **Przełączanie** | Smart ↔ Instant | Tak |
| 2 | **Start** | Normalny ↔ Od przerwy | Nie (jednorazowy) |

**Sterowanie (identyczne jak w menu serwisowym):**

| Przycisk | Akcja |
|----------|-------|
| **SELEKTOR (krótko)** | Kursor w dół (0→1→2→0) |
| **STOP (krótko)** | Kursor w górę (2→1→0→2) |
| **SELEKTOR (1 s)** | Zmień wartość wybranej opcji (cyklicznie) |
| **START** | **Rozpocznij malowanie** z bieżącymi ustawieniami |
| **STOP (1 s)** | Powrót do HOME bez zmian |

Wybrany element oznaczony jest kursorem **►**. Wartości kolorowane: zielony = domyślne/bezpieczne, żółty = zmienione/specjalne.

> **Przykład obsługi:** START (1 s) na HOME → ekran SETUP → SELEKTOR (1 s) zmienia tryb na SEMI → SELEKTOR (krótko) na "Start" → SELEKTOR (1 s) zmienia na "Od przerwy" → START = maszyna rusza w trybie SEMI od przerwy.

### 7.4 Menu serwisowe

Dostęp: **STOP (1 s)** na ekranie głównym.

Ekran wyświetla 5 pozycji z nagłówkiem "SERWIS":

| # | Pozycja | Opis |
|---|---------|------|
| 1 | **Kalibracja enkodera** | Procedura kalibracyjna na odcinku 10 m |
| 2 | **Pomiar dystansu** | Ręczny pomiar odległości (niezależny od malowania) |
| 3 | **Raporty** | Przeglądanie raportów z karty SD |
| 4 | **Czyszczenie dysz** | Ręczne uruchamianie pistoletów |
| 5 | **Reset etapu** | Zerowanie liczników sesji (dystans, powierzchnia, czas) |

**Nawigacja w menu serwisowym:**

| Przycisk | Akcja |
|----------|-------|
| **SELEKTOR (krótko)** | Następna pozycja (w dół) |
| **STOP (krótko)** | Poprzednia pozycja (w górę) |
| **SELEKTOR (1 s)** | Wejdź w wybraną opcję |
| **STOP (1 s)** | Powrót do ekranu głównego |

### 7.5 Kalibracja enkodera

Ekran procedury kalibracyjnej (szczegóły → [sekcja 10](#10-kalibracja-enkodera)):

- **Status:** POMIAR... (żółty, duża czcionka) lub GOTOWY
- **Licznik impulsów** (widoczny podczas pomiaru)
- **Impulsy/metr** (aktualna wartość)
- **Status kalibracji:** OK / Domyślny
- **Podpowiedzi:** START=rozpocznij/zakończ pomiar, STOP(1s)=powrót

### 7.6 Pomiar dystansu

Ręczny pomiar odległości (niezależny od malowania):

- **Status:** POMIAR... lub GOTOWY
- **Wynik:** Duża czcionka — w metrach (do 1000 m) lub kilometrach (powyżej)
- **Dodatkowa informacja:** Wartość w centymetrach

| Przycisk | Akcja |
|----------|-------|
| **START** | Rozpocznij / Wstrzymaj pomiar |
| **STOP (krótko)** | Resetuj licznik do 0 |
| **STOP (1 s)** | Powrót do menu serwisowego |

### 7.7 Raporty

Wyświetla informacje o raportach zapisanych na karcie SD:

- **Status karty SD:** OK (zielony) / BRAK (czerwony)
- **Liczba plików raportów**
- **Ostatni wpis:** Data, godzina, wzorzec, dystans, powierzchnia

Powrót: **STOP (1 s)**

### 7.8 Czyszczenie dysz

Tryb ręcznego testowania i czyszczenia pistoletów:

1. Wybierz wzorzec przyciskiem **SELEKTOR** (krótko = następny, długo = poprzedni)
2. **Trzymaj przycisk START** — pistolety przypisane do wybranego wzorca włączą się
3. **Puść START** — pistolety natychmiast się wyłączą

Na ekranie: Nagłówek "CZYSZCZENIE DYSZ", kod i nazwa wzorca, legenda kolorów, 6 prostokątów pistoletów.

> **Ważne:** W trybie czyszczenia dysz zabezpieczenie prędkości minimalnej jest **wyłączone** — pistolety działają nawet na postoju. Działają TYLKO gdy trzymasz przycisk START.

Powrót: **STOP (1 s)**

### 7.9 Podsumowanie etapu (SUMMARY)

Po naciśnięciu **STOP** podczas malowania, zamiast bezpośredniego powrotu do ekranu HOME, wyświetlany jest ekran podsumowania z wynikami bieżącego etapu.

```
┌──────────────────────────────────────────┐
│       PODSUMOWANIE ETAPU                 │
│                                          │
│              P-1a                        │
│  ─────────────────────────────────       │
│  Dystans:             1250.5 m           │
│  Powierzchnia:        150.06 m2          │
│  Czas:                0:30:45            │
│  Sred. predkosc:      2.5 km/h          │
│  GPS:  52.229676, 21.012229             │
│                                          │
│  START=kontynuuj STOP=nowy STOP(1s)=HOME│
└──────────────────────────────────────────┘
```

**Sterowanie:**

| Przycisk | Akcja |
|----------|-------|
| **START** | Kontynuuj malowanie — wznów z zachowaniem liczników sesji |
| **STOP** (krótki) | Nowy etap — zeruje liczniki sesji (dystans, powierzchnia, czas), powrót do HOME |
| **STOP** (1 s) | Powrót do HOME bez zerowania liczników |

**Wyświetlane dane:**
- Kod wzorca użytego podczas etapu
- Dystans sesji w metrach (lub km gdy > 1000 m)
- Powierzchnia sesji w m²
- Czas malowania w formacie HH:MM:SS
- Średnia prędkość w km/h
- Pozycja GPS (jeśli fix dostępny)

> **Wskazówka:** Ekran podsumowania pozwala szybko zdecydować, czy kontynuować bieżący etap (START), czy rozpocząć nowy z wyzerowanymi licznikami (STOP). Długie STOP wraca do HOME zachowując statystyki do dalszej analizy.

---

### 7.10 Reset etapu (sesji)

Ekran dostępny z menu serwisowego → pozycja 5 "Reset etapu". Służy do zerowania liczników sesji między etapami pracy bez wyłączania urządzenia.

```
┌──────────────────────────────────────────┐
│         RESET ETAPU                      │
│                                          │
│  Dystans sesji:      1250.5 m            │
│  Powierzchnia sesji: 150.06 m2           │
│  Czas sesji:         1845 s              │
│                                          │
│  Wyzerowac liczniki sesji?               │
│                                          │
│  START = TAK        STOP = NIE           │
└──────────────────────────────────────────┘
```

**Sterowanie:**

| Przycisk | Akcja |
|----------|-------|
| **START** | Potwierdź reset — zeruje liczniki sesji, sygnał buzzera, powrót do HOME |
| **STOP** | Anuluj — powrót do menu serwisowego bez zmian |

**Co jest zerowane:**
- Dystans sesji (enkoder)
- Powierzchnia sesji
- Czas sesji
- Stan maszyny ustawiany na IDLE

**Co NIE jest zerowane:**
- Statystyki lifetime (łączny dystans, powierzchnia, czas)
- Kalibracja enkodera
- Ustawienia (wzorzec, tryb pracy, próg prędkości)
- Raporty na karcie SD

> **Typowy scenariusz:** Zakończ etap malowania (STOP → raport zapisany na SD) → wejdź w menu serwisowe (STOP 1 s) → wybierz "Reset etapu" → potwierdź → zacznij nowy etap z czystymi licznikami.

---

## 8. Funkcja "Start od przerwy"

### 8.1 Opis

Funkcja "Start od przerwy" pozwala rozpocząć malowanie nie od kreski, ale od przerwy we wzorcu. Jest to niezbędne przy kontynuacji istniejącego oznakowania drogowego — maszyna może dojechać do miejsca, gdzie linia przerywana powinna mieć przerwę, i rozpocząć pracę z właściwą fazą wzorca.

### 8.2 Jak działa

**Normalny start** (przycisk START) — cykl zaczyna się od kreski:
```
▮▮▮▮░░░░░░░░▮▮▮▮░░░░░░░░▮▮▮▮░░░░░░░░
kreska  przerwa  kreska  przerwa  kreska  przerwa
```

**Start od przerwy** (przycisk GAP) — cykl zaczyna się od przerwy:
```
░░░░░░░░▮▮▮▮░░░░░░░░▮▮▮▮░░░░░░░░▮▮▮▮
przerwa  kreska  przerwa  kreska  przerwa  kreska
```

Technicznie system przesuwa punkt startowy wzorca o długość kreski, dzięki czemu funkcja `fmod()` obliczająca pozycję w cyklu trafia od razu w fazę przerwy.

### 8.3 Aktywacja

> **Uwaga:** W trybie **SEMI-AUTO** start od przerwy oznacza rozpoczęcie od fazy oczekiwania na START (operator musi nacisnąć START aby rozpocząć pierwszą kreskę). W trybie **MANUAL** start od przerwy działa identycznie jak normalny start.

| Sposób | Jak |
|--------|-----|
| **Na urządzeniu** | Naciśnij dedykowany przycisk **GAP** (GPIO 7) na ekranie HOME |
| **W panelu WWW** | Naciśnij żółty przycisk **START OD PRZERWY** |

Na ekranie malowania pojawi się znacznik **[GAP]** informujący o aktywnym trybie.

### 8.4 Kiedy używać

- Kontynuacja istniejącej linii przerywanej (np. po przerwie w pracy)
- Malowanie od punktu, gdzie powinna być przerwa a nie kreska
- Synchronizacja z istniejącym oznakowaniem na jezdni
- Rozpoczynanie pracy na środku istniejącego odcinka oznakowania

> **Uwaga:** Dla wzorców ciągłych (P-2a, P-2b, P-4, P-7b, P-7d) start od przerwy działa identycznie jak normalny start, ponieważ te wzorce nie posiadają przerw.

---

## 9. Panel WWW (zdalny dostęp)

### 9.1 Połączenie

1. Na telefonie lub komputerze wyszukaj sieć WiFi **TrassarV3**
2. Połącz się hasłem: **12345678**
3. Otwórz przeglądarkę i wejdź na: **http://192.168.4.1**

Panel automatycznie odświeża dane co 1 sekundę bez przeładowania strony.

### 9.2 Funkcje panelu WWW

Panel sterowania w przeglądarce oferuje pełną kontrolę nad maszyną:

| Sekcja | Opis |
|--------|------|
| **Status** | Stan maszyny z kolorowym pulsującym wskaźnikiem (Gotowy / Malowanie / Pauza / Zatrzymany) |
| **Informacje** | Wzorzec, nazwa, prędkość, dystans, powierzchnia, czas sesji, odwrócenie, kalibracja |
| **Sterowanie** | Przyciski START / PAUZA / STOP / START OD PRZERWY |
| **Tryb pracy** | 3 przyciski: AUTO / SEMI / RĘCZNY — aktywny podświetlony na zielono |
| **Wybór wzorca** | 16 przycisków pogrupowanych: P-1x, P-2x, P-3x, P-4/P-6, P-7x, WŁASNY |
| **Podgląd wzorca** | Canvas wizualizacja kreska/przerwa w skali — osobny rząd per pistolet, szerokość (12/24cm) |
| **Tryb przełączania** | Smart (dokończ cykl) / Instant (natychmiast) — wybór z panelu, zapis do NVS |
| **Wzorzec własny** | Edytor: 6 pistoletów, kreska/przerwa, 3 sloty pamięci, zapis do NVS |
| **Odwracanie** | Przycisk "Odwróć" — aktywny tylko dla P-3a / P-3b |
| **Pistolety** | 6 kółek P1–P6 (zielone = ON, szare = OFF, **czerwone migające** = anomalia) |
| **Semi: kolejna linia** | Przycisk widoczny w trybie SEMI gdy kreska zakończona |
| **Anomalia pistoletów** | Pulsujący banner ostrzegawczy gdy wykryto anomalię — widoczny automatycznie |
| **Kalibracja** | Przycisk rozpoczęcia/zakończenia, licznik impulsów, impulsy/metr |
| **Alarm prędkości** | Bieżący próg maks., suwak konfiguracji (5–30 km/h), przycisk zapisu do NVS |
| **System** | Wersja firmware, wolna RAM, uptime, liczba klientów WiFi |
| **Menu serwisowe** | Dwie zakładki: **Statystyki** (lifetime + sesja) i **Raporty SD** (lista plików CSV) |

### 9.3 Zmiana wzorca przez panel WWW

W panelu WWW dostępne jest **16 przycisków wzorców** (15 normowych + WŁASNY). Aktywny wzorzec jest podświetlony na zielono. Jest to **jedyny sposób zmiany wzorca** — na fizycznym panelu sterowania (ekran HOME i malowania) przycisk SELEKTOR służy wyłącznie do odwracania P-3a/P-3b.

> **Uwaga:** Przycisk WŁASNY jest aktywny dopiero po skonfigurowaniu i zapisaniu wzorca własnego w edytorze.

### 9.4 Przełączanie wzorców (Smart / Instant)

System oferuje dwa tryby przełączania wzorców **podczas malowania**, wybierane z panelu WWW:

**Tryb Smart (domyślny)** — zmiana wzorca NIE przerywa bieżącego cyklu:

1. Bieżąca linia jest domalowywana do pełnej długości
2. Przerwa po linii jest dokańczana w całości
3. Nowy wzorzec zaczyna się dopiero po zakończeniu pełnego cyklu starego

**Wizualne potwierdzenie:**
- Oczekujący wzorzec **miga pomarańczowo** w panelu WWW
- Po faktycznym przełączeniu: krótki sygnał dźwiękowy (1500 Hz, 80 ms)
- Aktywny wzorzec zmienia podświetlenie z zielonego na nowy

**Wyjątki:**
- **Wzorce ciągłe** (P-2a, P-2b, P-4, P-7b, P-7d) — przełączają się natychmiast (brak cyklu)
- **PAUZA / STOP** — mogą przerwać malowanie w dowolnym momencie
- **Anulowanie** — kliknięcie bieżącego aktywnego wzorca anuluje oczekującą zmianę

**Przykład (Smart):** Malując P-1a (4 m linia + 8 m przerwa), klikasz P-1b. Maszyna domalowuje bieżącą linię 4 m, przejeżdża pełną przerwę 8 m, a potem zaczyna P-1b (2 m linia + 4 m przerwa).

**Tryb Instant** — natychmiastowa zmiana wzorca:

Zmiana wzorca **podczas malowania** następuje natychmiast:

1. Bieżąca kreska jest ucinana w miejscu zmiany
2. Nowy wzorzec zaczyna się od razu
3. Nie ma oczekiwania na koniec cyklu

**Przykład (Instant):** Malując P-3a (ciągła + przerywana), klikasz P-2a. Maszyna natychmiast przełącza się — nawet w połowie kreski.

**Wybór trybu przełączania:**
- **Na urządzeniu:** Przytrzymaj **SELEKTOR (1 s)** na ekranie HOME — krótki beep potwierdzi zmianę
- **W panelu WWW:** Pod podglądem wzorca — dwa przyciski: **Smart** / **Instant**
- Aktywny tryb jest podświetlony
- Wybór jest zapisywany trwale w NVS — przetrwa restart urządzenia

---

## 10. Kalibracja enkodera

### 10.1 Dlaczego kalibracja jest ważna

Enkoder obrotowy mierzy obroty koła pomiarowego, ale fabryczna wartość impulsów/metr (100.0) może nie odpowiadać rzeczywistemu obwodowi koła. Prawidłowa kalibracja zapewnia dokładne pomiary dystansu, prędkości i precyzyjne odwzorowanie długości kresek i przerw we wzorcach.

### 10.2 Procedura kalibracji

1. Wejdź w **Menu serwisowe** → **Kalibracja enkodera**
2. Odmierz na podłożu dokładnie **10 metrów** (np. taśmą mierniczą)
3. Ustaw maszynę na początku odcinka
4. Naciśnij **START** — na ekranie pojawi się "POMIAR..." i licznik impulsów
5. Przejedź maszyną **dokładnie 10 m** po prostej
6. Naciśnij **START** — system obliczy impulsy/metr i zapisze w pamięci NVS
7. Sprawdź wynik: "Imp/metr" i "Status: OK"

Wynik kalibracji jest zapisywany trwale w pamięci NVS (Non-Volatile Storage) i przetrwa restart urządzenia.

### 10.3 Anulowanie kalibracji

Naciśnij **STOP (1 s)** w trakcie pomiaru — kalibracja zostanie anulowana, poprzednia wartość pozostanie bez zmian.

---

## 11. Raporty na karcie SD

### 11.1 Format raportów

Raporty zapisywane są automatycznie po każdym zatrzymaniu malowania (STOP) na kartę SD w formacie CSV:

- **Lokalizacja:** `/reports/RRRRMMDD.csv` (np. `/reports/20250612.csv`)
- **Nagłówek:** `data,godzina,wzorzec,dystans_m,powierzchnia_m2,lat,lon`
- **Jeden wiersz** na każdą sesję malowania
- Kolumny `lat` i `lon` zawierają koordynaty GPS w momencie zakończenia sesji (0 gdy brak fix)

**Przykład zawartości pliku `/reports/20250612.csv`:**
```csv
data,godzina,wzorzec,dystans_m,powierzchnia_m2,lat,lon
2025-06-12,08:30:15,P-1a,1250.5,150.06,52.229676,21.012229
2025-06-12,10:45:22,P-3a,875.3,210.07,52.230100,21.013500
2025-06-12,14:10:08,P-2b,430.0,103.20,0.000000,0.000000
```

### 11.2 Przeglądanie raportów

- **Na urządzeniu:** Menu serwisowe → Raporty — status SD, liczba plików, ostatni wpis
- **Na komputerze:** Wyjmij kartę SD i otwórz pliki CSV w dowolnym arkuszu kalkulacyjnym

### 11.3 Wymagania karty SD

- Format: **FAT32**
- Slot: Zintegrowany w module wyświetlacza ILI9341
- Współdzieli magistralę SPI z wyświetlaczem (osobne linie CS)

---

## 12. Zabezpieczenia

### 12.1 Minimalna prędkość malowania

System wymaga prędkości minimum **3 km/h** do włączenia pistoletów. Poniżej tej prędkości:
- Pistolety pozostają wyłączone (nawet w stanie "Malowanie")
- Na ekranie nadal widoczny jest status "Malowanie"
- Po przyspieszeniu powyżej 3 km/h pistolety włączają się automatycznie
- Pozycja w cyklu wzorca (kreska/przerwa) jest obliczana na bieżąco z dystansu

**Wyjątek:** Tryb czyszczenia dysz omija zabezpieczenie prędkości — pistolety działają na postoju.

### 12.2 Automatyczne wyłączanie pistoletów

Pistolety wyłączają się natychmiast przy:
- Zatrzymaniu malowania (STOP)
- Pauzie malowania (START podczas malowania)
- Spadku prędkości poniżej 3 km/h
- Wyjściu z trybu czyszczenia dysz
- Puszczeniu przycisku START w trybie czyszczenia dysz
- Zadziałaniu mechanizmu gun keepalive (brak aktualizacji silnika malowania >300 ms)

### 12.3 Watchdog timer

System posiada sprzętowy watchdog timer ESP32 z timeoutem **3 sekund**. Jeśli pętla główna (`loop()`) zawiesi się z dowolnej przyczyny:
- Watchdog zrestartuje mikrokontroler po 3 sekundach
- Wszystkie piny GPIO wracają do stanu LOW — pistolety się zamykają
- System uruchamia się od nowa z zapisanymi ustawieniami z NVS

> **Uwaga:** Reset watchdoga jest widoczny w monitorze szeregowym jako komunikat restartu.

### 12.4 Gun keepalive (300 ms)

Niezależna od watchdoga warstwa bezpieczeństwa. Chroni przed scenariuszem, w którym pętla `loop()` działa (watchdog jest karmiony), ale silnik malowania z jakiegoś powodu nie steruje pistoletami:
- Jeśli silnik malowania nie zaktualizuje stanu pistoletów przez **300 ms**, system wykonuje awaryjne wyłączenie wszystkich pistoletów (`guns.allOff()`)
- Informacja o zadziałaniu keepalive jest logowana na port szeregowy

### 12.5 Alarm przekroczenia prędkości

Przy zbyt dużej prędkości jakość malowania spada (farba się rozpryskuje, linie nie są równe). System alarmuje operatora:

- **Próg domyślny:** 15 km/h
- **Konfiguracja:** Panel WWW → sekcja "Alarm prędkości" → suwak 5–30 km/h → przycisk "Zapisz"
- **Próg jest zapisywany trwale w NVS** — przetrwa restart urządzenia

**Sygnalizacja przy przekroczeniu:**
- **Wyświetlacz:** Prędkość miga na czerwono (cykl 300 ms)
- **Buzzer:** Trojkowy alarm 3 kHz, powtarzany co 2 sekundy
- **Panel WWW:** Napis "PRZEKROCZENIE!" w sekcji alarmu prędkości, kolor prędkości na czerwono

**Sygnalizacja przy niskiej prędkości (<3 km/h podczas malowania):**
- **Wyświetlacz:** Prędkość wyświetlana na żółto
- **Buzzer:** Podwójny puls 1.5 kHz, powtarzany co 3 sekundy

### 12.6 Odszumianie enkodera

Podczas inicjalizacji systemu enkoder może rejestrować drobne drgania. Po zakończeniu inicjalizacji system automatycznie zeruje licznik dystansu (`resetDistance()`), eliminując szum nazbierany podczas startu.

---

## 13. Sygnalizacja dźwiękowa (buzzer)

System wyposażony jest w pasywny buzzer (GPIO 8) generujący sygnały dźwiękowe informujące operatora o zdarzeniach. Sygnały są szczególnie przydatne w hałaśliwym środowisku pracy (maszyna drogowa, ruch uliczny).

### 13.1 Tabela sygnałów dźwiękowych

| Zdarzenie | Sygnał | Częstotliwość | Opis |
|-----------|--------|---------------|------|
| **Start malowania** | 1× krótki beep | 2 kHz, 100 ms | Potwierdzenie rozpoczęcia malowania |
| **Wznowienie po pauzie** | 1× krótki beep | 2 kHz, 100 ms | Potwierdzenie wznowienia |
| **Pauza malowania** | 2× krótki beep | 2 kHz, 80 ms + 80 ms | Potwierdzenie pauzy |
| **Stop malowania** | 2× krótki beep | 2 kHz, 80 ms + 80 ms | Potwierdzenie zatrzymania |
| **Niska prędkość** | 2× puls | 1.5 kHz, 150 ms + 150 ms | Prędkość <3 km/h podczas malowania (co 3 s) |
| **Przekroczenie prędkości** | 3× alarm | 3 kHz, 60 ms × 3 | Prędkość > próg maks. (co 2 s) |
| **Anomalia pistoletu** | Niski-wysoki-niski | 800→1200→800 Hz | Pistolet nie strzela mimo konfiguracji (po 50 m) |
| **Błąd (RTC/SD)** | Opadający ton | 1000→800→600 Hz | Brak karty SD lub RTC niedostępny przy starcie |
| **Semi: kreska gotowa** | 1× krótki beep | 1 kHz, 50 ms | Kreska zakończona, czekam na START (tryb SEMI) |
| **Semi: kolejna linia** | 1× krótki beep | 1.5 kHz, 80 ms | Potwierdzenie rozpoczęcia nowej kreski (tryb SEMI) |
| **Potwierdzenie trybu** | 1× krótki beep | 1.5 kHz, 80 ms | Zatwierdzenie wyboru trybu pracy |

### 13.2 Specyfikacja techniczna buzzera

| Parametr | Wartość |
|----------|---------|
| Typ | Pasywny (wymaga sygnału PWM) |
| GPIO | 8 |
| Sterowanie | LEDC PWM, kanał 1, duty cycle 50% |
| Tryb pracy | Non-blocking (sekwencje zarządzane w pętli głównej) |

### 13.3 Uwagi

- Buzzer nie blokuje pracy systemu — sekwencje tonów są przetwarzane w tle
- Alarmy prędkości powtarzają się cyklicznie dopóki warunek jest spełniony
- Sygnał błędu odtwarzany jest jednokrotnie przy uruchomieniu systemu (jeśli wykryto problem)

---

## 14. Architektura wielordzeniowa

TrassarV3 v2.16.0 wykorzystuje oba rdzenie procesora ESP32-S3 i obsługuje 10 ekranów interfejsu:

| Rdzeń | Zadania |
|-------|---------|
| **Core 0** | Serwer WWW (WiFi, obsługa HTTP, API REST) |
| **Core 1** | Krytyczna pętla: enkoder, pistolety, buzzer, wyświetlacz, statystyki |

### Korzyści
- Obciążenie serwera HTTP (np. szybkie odświeżanie panelu) **nie wpływa** na czas reakcji pistoletów
- **Mutex spinlock** (`portMUX_TYPE`) chroni współdzielone dane `g_state` przed race conditions
- Gun keepalive (300 ms) jest niezawodny nawet przy wielu klientach HTTP
- Watchdog monitoruje tylko Core 1 (krytyczny)

### Okresowy zapis statystyk
- Podczas malowania statystyki lifetime zapisywane automatycznie do NVS **co 60 sekund**
- Ochrona przed utratą danych przy: watchdog reset, zanik zasilania, awaria sprzętu

---

## 15. Diagnostyka systemowa

### Logi diagnostyczne (Serial, co 30 s)
```
[DIAG] Heap: 185000/327680 B (min: 165000)  Frag: 12%  WWW-stack: 2048  Core: 1
```

| Pole | Opis |
|------|------|
| Heap | Wolna / łączna pamięć RAM |
| min | Minimalna wolna RAM od startu |
| Frag | Fragmentacja heap (%) |
| WWW-stack | Stack high-water mark tasku WWW (Core 0) |
| Core | Numer rdzenia dla loop() |

### API JSON (diagnostyka)
Nowe pola w `GET /api/status`:
- `minFreeHeap` — minimalna wolna RAM od uruchomienia [bajty]
- `webStackHWM` — stack high-water mark tasku serwera HTTP

### Ekran malowania
Na ekranie malowania wyświetlane są dodatkowe informacje w lewej kolumnie:
- **Czas sesji** — format MM:SS (lub H:MM:SS dla sesji >1h)
- **Dystans sesji** — w metrach (lub km przy dystansie ≥1000 m)

---

## 16. Detekcja anomalii pistoletów

System automatycznie wykrywa sytuacje, gdy pistolet jest skonfigurowany we wzorcu (tryb CONTINUOUS lub DASHED), ale w praktyce nie maluje (dystans strzału < 1 m po 50 m jazdy).

### 16.1 Jak działa

- **Aktywacja:** Po przejechaniu 50 m w sesji malowania
- **Sprawdzanie:** Co 10 sekund podczas malowania
- **Warunek anomalii:** Pistolet skonfigurowany we wzorcu, ale jego dystans strzału < 1 m
- **Sygnalizacja:** Buzzer (niski-wysoki-niski: 800→1200→800 Hz) + log na porcie szeregowym
- **Reset:** Automatyczny po zatrzymaniu malowania (STOP)

### 16.2 Możliwe przyczyny anomalii

| Przyczyna | Rozwiązanie |
|-----------|-------------|
| Uszkodzony przekaźnik | Sprawdź moduł przekaźnikowy |
| Poluzowany przewód GPIO | Sprawdź podłączenie pinu GPIO pistoletu |
| Zatkana dysza | Wyczyść dysze w trybie Czyszczenie dysz |
| Pusty zbiornik farby | Uzupełnij farbę |

### 16.3 Informacje w API

Pola w `GET /api/status`:
- `gunAnomalyDetected` — `true` jeśli wykryto anomalię
- `gunAnomaly` — tablica `[false, true, false, ...]` wskazująca anomalne pistolety

---

## 17. API statystyk i raportów SD

### 17.1 Statystyki (`GET /api/stats`)

Endpoint zwraca łączne (lifetime) i bieżące (sesja) statystyki malowania:

```bash
curl http://192.168.4.1/api/stats
```

Zawiera: łączny dystans i powierzchnię, czas malowania, dystans per pistolet, status karty SD.

### 17.2 Raporty SD (`GET /api/reports`)

Endpoint zwraca listę plików raportów CSV z karty SD:

```bash
curl http://192.168.4.1/api/reports
```

Odpowiedź: `[{"file":"20260216.csv","size":1234},...]` — sortowane malejąco (najnowsze pierwsze).

> **Uwaga:** Lista raportów jest cache'owana i odświeżana co 15 sekund. Nie wymaga bezpośredniego dostępu do karty SD — dane serwowane z pamięci RAM.

---

## 18. Menu serwisowe w panelu WWW

Panel WWW posiada sekcję **Menu serwisowe** na dole strony, z dwoma zakładkami:

### 18.1 Zakładka "Statystyki"

Wyświetla dane zebrane przez cały czas pracy urządzenia (lifetime) oraz bieżącą sesję:

| Pole | Opis |
|------|------|
| **Dyst. całkowity** | Łączny dystans namalowany przez maszynę [m] |
| **Pow. całkowita** | Łączna powierzchnia namalowana [m²] |
| **Czas malowania** | Sumaryczny czas malowania (lifetime) |
| **Karta SD** | Status karty SD + liczba raportów (np. "OK (12)") |
| **Dystans per pistolet** | 6 kółek P1–P6 z dystansem w metrach (bieżąca sesja) |
| **Licznik strzałów** | 6 kółek P1–P6 z liczbą strzałów (lifetime) — planowanie serwisu dysz |

Dane odświeżają się automatycznie co 10 sekund.

### 18.2 Zakładka "Raporty SD"

Wyświetla tabelę plików raportów CSV z karty SD:

| Kolumna | Opis |
|---------|------|
| **Plik** | Nazwa pliku (format: `RRRRMMDD.csv`) |
| **Rozmiar** | Rozmiar pliku (w B lub KB) |
| **Pobierz** | Link do pobrania pliku CSV bezpośrednio z przeglądarki |

Lista jest sortowana malejąco — najnowsze raporty na górze. Przycisk **Odśwież** ładuje ponownie listę z cache. Kliknięcie linku **CSV** pobiera plik raportu na urządzenie.

### 18.3 Wskaźniki anomalii pistoletów

Gdy system wykryje anomalię pistoletów (skonfigurowany pistolet nie maluje po 50 m jazdy):

- Pulsujący **czerwony banner** "ANOMALIA PISTOLETU - sprawdź dysze!" pojawia się automatycznie
- Kółka pistoletów z anomalią **migają czerwoną ramką**
- Banner znika automatycznie po wznowieniu prawidłowej pracy

---

## 19. Moduł GPS

### 19.1 Opis

System obsługuje moduł GPS **GY-NEO6MV2** (chip NEO-6M) z anteną ceramiczną. Moduł dostarcza dane o pozycji geograficznej, prędkości i dokładności sygnału.

### 19.2 Podłączenie

| Pin GPS | Pin ESP32-S3 | GPIO | Opis |
|---------|-------------|------|------|
| VCC | 3V3 | — | Zasilanie 3.3V |
| GND | GND | — | Masa |
| TX | GPIO 47 | 47 | GPS TX → ESP32 RX (UART2) |
| RX | GPIO 48 | 48 | GPS RX ← ESP32 TX (UART2) |

> **Uwaga:** Moduł GPS komunikuje się przez UART2 z prędkością 9600 baud (domyślna NEO-6M). Antena GPS musi mieć widoczność nieba — montuj na zewnątrz kabiny maszyny.

### 19.3 Dane GPS w panelu WWW

Sekcja "GPS" w panelu sterowania wyświetla:

| Pole | Opis |
|------|------|
| **Fix / Satelity** | Status fix (TAK/BRAK) i liczba widocznych satelitów |
| **HDOP** | Dokładność pozycji (niższa = lepsza, <2.0 = dobra) |
| **Pozycja** | Szerokość i długość geograficzna (gdy fix aktywny) |
| **Prędkość GPS** | Prędkość mierzona przez GPS [km/h] |

### 19.4 GPS w raportach CSV

Po zatrzymaniu malowania (STOP) koordynaty GPS są automatycznie zapisywane w raporcie CSV:
- Kolumny `lat` i `lon` w nagłówku i danych
- Wartości 0.000000 gdy moduł GPS nie ma fix w momencie zapisu

### 19.5 Rozwiązywanie problemów GPS

| Problem | Możliwa przyczyna | Rozwiązanie |
|---------|-------------------|-------------|
| Brak fix (BRAK) | Antena nie widzi nieba | Zamontuj antenę na zewnątrz, z widocznością nieba |
| Mało satelitów (<4) | Słaby sygnał | Poczekaj 1-2 min na cold start, popraw pozycję anteny |
| HDOP > 5 | Niska dokładność | Za mało satelitów lub odbicia sygnału od budynków |
| Brak danych w panelu | Złe podłączenie UART | Sprawdź piny GPIO 47 (RX) i GPIO 48 (TX) |

---

## 20. Fizyczne przyciski wzorców (MCP23017)

### 20.1 Opis

System wyposażony jest w **15 dedykowanych przycisków fizycznych** do natychmiastowego wyboru wzorca malowania — po jednym na każdy wzorzec predefiniowany (P-1a do P-7d). Przyciski podłączone są do ekspandera I2C **MCP23017**, który komunikuje się z ESP32-S3 na tej samej magistrali I2C co zegar RTC DS1307.

Dzięki przyciskom wzorców operator może zmieniać wzorzec **bez konieczności korzystania z telefonu czy panelu WWW** — wystarczy nacisnąć odpowiedni przycisk na panelu sterowania maszyny.

### 20.2 Mapowanie przycisków

| Przycisk | Wzorzec | Nazwa | Typ linii |
|----------|---------|-------|-----------|
| 1 | P-1a | Przerywana długa | 4m / 8m, P2, 12cm |
| 2 | P-1b | Przerywana krótka | 2m / 4m, P2, 12cm |
| 3 | P-1c | Wydzielająca | 2m / 2m, P2, 12cm |
| 4 | P-1d | Prowadząca wąska | 1m / 1m, P2, 12cm |
| 5 | P-1e | Prowadząca szeroka | 1m / 1m, P4, 24cm |
| 6 | P-2a | Ciągła wąska | ciągła, P2, 12cm |
| 7 | P-2b | Ciągła szeroka | ciągła, P4, 24cm |
| 8 | P-3a | Przekraczalna długa | ciągła + 4m/2m, P1+P3 |
| 9 | P-3b | Przekraczalna krótka | ciągła + 1m/1m, P1+P3 |
| 10 | P-4 | Podwójna ciągła | ciągła + ciągła, P1+P3 |
| 11 | P-6 | Ostrzegawcza | 4m / 2m, P5, 12cm |
| 12 | P-7a | Krawędziowa przeryw. szer. | 1m / 1m, P6, 24cm |
| 13 | P-7b | Krawędziowa ciągła szer. | ciągła, P6, 24cm |
| 14 | P-7c | Krawędziowa przeryw. wąska | 1m / 1m, P5, 12cm |
| 15 | P-7d | Krawędziowa ciągła wąska | ciągła, P5, 12cm |

### 20.3 Zachowanie

- **Stan spoczynku (IDLE / HOME):** Naciśnięcie przycisku natychmiast zmienia aktywny wzorzec. Wyświetlacz aktualizuje się, buzzer potwierdza (1500 Hz, 80 ms). Wzorzec zapisywany do NVS.
- **Podczas malowania (PAINTING):** Zmiana wzorca respektuje tryb przełączania:
  - **Smart:** Bieżący cykl (kreska + przerwa) jest dokańczany, nowy wzorzec zaczyna się po jego zakończeniu
  - **Instant:** Wzorzec zmienia się natychmiast
- **Potwierdzenie:** Każde naciśnięcie sygnalizowane krótkim dźwiękiem buzzera i logowane na kartę SD

### 20.4 Podłączenie

Przyciski podłączane są w prostej konfiguracji: jeden styk do pinu MCP23017, drugi do GND. Wewnętrzne rezystory pull-up MCP23017 są aktywowane programowo — **nie potrzeba zewnętrznych rezystorów**.

Szczegółowy schemat → [Schemat podłączeń](SCHEMAT_PODLACZEN.md), sekcja 6.7

### 20.5 Diagnostyka

Jeśli MCP23017 nie jest dostępny (niesprawny, niepodłączony), system wyświetla komunikat w logach szeregowych:
```
[PAT_BTN] MCP23017 niedostepny na 0x20 (err=2)
```
System kontynuuje normalne działanie — przyciski wzorców są niedostępne, ale zmiana wzorca pozostaje możliwa przez panel WWW.

---

## 21. Przykłady zastosowania

### Przykład 1: Malowanie linii przerywanej P-1a na nowej drodze

**Scenariusz:** Nowo wybudowana droga wymaga namalowania osi jezdni linią przerywaną długą (4 m kreska, 8 m przerwa, 12 cm szerokości). Maszyna jest przygotowana i ustawiona na początku odcinka.

**Kroki:**

1. **Przygotowanie:**
   - Włącz urządzenie — pojawi się ekran powitalny "TrassarV3", a po chwili ekran główny
   - Sprawdź wyświetlany wzorzec w lewym górnym rogu
   - Jeśli wyświetlany wzorzec to nie P-1a, zmień go przez panel WWW: połącz się z WiFi "TrassarV3" (hasło: 12345678), otwórz http://192.168.4.1 i kliknij przycisk **P-1a**
   - Sprawdź status kalibracji w panelu WWW — powinno być "Skalibrowany"

2. **Malowanie:**
   - Na ekranie głównym naciśnij przycisk **START**
   - Ekran przełączy się na widok malowania: status "Malowanie" (zielony)
   - Ruszaj maszyną — po osiągnięciu 3 km/h pistolet P2 zacznie malować
   - System automatycznie steruje cyklem: 4 m farba → 8 m przerwa → 4 m farba → ...
   - Prostokąt P2 na ekranie świeci na zielono gdy maluje, gaśnie gdy przerwa

3. **Zakończenie:**
   - Po dojechaniu do końca odcinka naciśnij **STOP**
   - Raport zostanie automatycznie zapisany na kartę SD
   - System wróci na ekran główny ze zaktualizowaną powierzchnią

**Co widzisz na ekranie:**
- P2: zielony (maluje kreskę) → szary (przerwa) → zielony → szary → ...
- P1, P3, P4, P5, P6: szare przez cały czas (nieużywane w P-1a)

---

### Przykład 2: Malowanie linii przekraczalnej P-3a z odwracaniem

**Scenariusz:** Droga dwukierunkowa wymaga linii przekraczalnej (ciągła + przerywana). W pierwszym kierunku jazdy linia ciągła powinna być po lewej stronie maszyny (domyślnie), w powrotnym — po prawej.

**Kroki — pierwszy kierunek:**

1. Ustaw wzorzec **P-3a** przez panel WWW
2. Na ekranie głównym upewnij się, że **nie ma** znacznika [ODW] — tryb normalny:
   - P1 (lewy) = linia ciągła
   - P3 (prawy) = linia przerywana (4 m kreska / 2 m przerwa)
3. Naciśnij **START** — ruszaj maszyną
4. Oba pistolety P1 i P3 pracują jednocześnie: P1 ciągle, P3 cyklicznie
5. Na ekranie: prostokąty P1 i P3 świecą na zielono

**Kroki — kierunek powrotny (po zawróceniu):**

1. Naciśnij **STOP** aby zakończyć pierwszy odcinek
2. Na ekranie głównym naciśnij krótko **SELEKTOR** — pojawi się znacznik **[ODW]**
3. Teraz P1 (lewy) = przerywana, P3 (prawy) = ciągła (odwrócone role)
4. Naciśnij **START** i maluj w powrotnym kierunku
5. Aby zdjąć odwrócenie, naciśnij **SELEKTOR** ponownie — [ODW] zniknie

> **Wskazówka:** Odwracanie można też wykonać w trakcie malowania bez zatrzymywania maszyny.

---

### Przykład 3: Kontynuacja istniejącego oznakowania — "Start od przerwy"

**Scenariusz:** Na drodze istnieje już linia przerywana P-1b (2 m kreska, 4 m przerwa). Maszyna stoi w miejscu, gdzie kończy się kreska i zaczyna przerwa. Musimy kontynuować malowanie z zachowaniem synchronizacji faz.

**Kroki:**

1. Ustaw wzorzec **P-1b** przez panel WWW
2. Ustaw maszynę dokładnie w punkcie, gdzie zaczyna się przerwa
3. Na ekranie głównym naciśnij dedykowany przycisk **GAP** (GPIO 7) — **nie START!**
4. Na ekranie malowania pojawi się znacznik **[GAP]** i status "Malowanie"
5. Ruszaj maszyną:
   - Przez pierwsze **2 m** pistolet **NIE maluje** (faza przerwy zamiast kreski)
   - Po 2 m pistolet P2 zacznie malować kreskę (2 m)
   - Dalej normalny cykl: 4 m przerwa → 2 m kreska → 4 m przerwa → ...

**Porównanie wizualny:**
```
Istniejące oznakowanie:  ██░░░░██░░░░██░░░░
Normalny START:          ██░░░░██░░░░██░░░░  (kreska od razu)
START OD PRZERWY (GAP):  ░░░░██░░░░██░░░░██  (przerwa na początku)
```
(██ = malowanie, ░░ = przerwa)

> **Alternatywa:** Zamiast przycisku fizycznego, naciśnij żółty przycisk **START OD PRZERWY** w panelu WWW.

---

### Przykład 4: Czyszczenie dysz i testowanie pistoletów przed pracą

**Scenariusz:** Rozpoczęcie dnia pracy — trzeba sprawdzić sprawność wszystkich 6 pistoletów i oczyścić dysze po nocnym postoju.

**Kroki:**

1. **Wejście w tryb czyszczenia:**
   - Na ekranie głównym przytrzymaj **STOP (1 s)** → menu serwisowe
   - Naciśnij **SELEKTOR** 3 razy aby dojść do pozycji 4: **Czyszczenie dysz**
   - Przytrzymaj **SELEKTOR (1 s)** aby wejść

2. **Test pistoletu P2 (oś środek):**
   - Domyślnie wybrany jest aktualny wzorzec. Selektorem wybierz **P-1a** (używa P2)
   - **Trzymaj przycisk START** — P2 włączy się (prostokąt P2 zmieni się na zielony)
   - **Puść START** — P2 natychmiast się wyłączy

3. **Test pistoletów P1 i P3 jednocześnie:**
   - Naciśnij **SELEKTOR** aby przejść na wzorzec **P-4** (podwójna ciągła: P1+P3)
   - **Trzymaj START** → oba pistolety P1 i P3 włączą się jednocześnie
   - **Puść** → wyłączą się

4. **Systematyczny test wszystkich pistoletów:**

   | Wzorzec | Testowane pistolety |
   |---------|---------------------|
   | P-1a | P2 |
   | P-2b | P4 |
   | P-4 | P1 + P3 |
   | P-7a | P6 |
   | P-7c | P5 |

5. **Powrót:** STOP (1 s) → menu serwisowe → STOP (1 s) → ekran główny

> **Ważne:** W trybie czyszczenia nie obowiązuje zabezpieczenie prędkości 3 km/h — pistolety działają na postoju, ale TYLKO gdy trzymasz START.

---

### Przykład 5: Kalibracja enkodera po wymianie koła pomiarowego

**Scenariusz:** Wymieniono koło pomiarowe na inne o innym obwodzie. Trzeba ponownie skalibrować enkoder, aby pomiary dystansu i prędkości były dokładne, a długości kresek i przerw odpowiadały normom.

**Kroki:**

1. **Przygotowanie odcinka kalibracyjnego:**
   - Na podłożu odmierz dokładnie **10 metrów** taśmą mierniczą
   - Zaznacz punkt początkowy i końcowy (np. kredą)

2. **Wejście w kalibrację:**
   - STOP (1 s) → menu serwisowe → wybierz "Kalibracja enkodera" → SELEKTOR (1 s)
   - Na ekranie: "GOTOWY", aktualne Imp/metr i Status

3. **Pomiar:**
   - Ustaw maszynę na punkcie startowym
   - Naciśnij **START** — na ekranie pojawi się "POMIAR..." i licznik impulsów rosnący w czasie rzeczywistym
   - Jedź maszyną **po prostej** dokładnie do punktu 10 m
   - Naciśnij **START** — system obliczy: nowe Imp/metr = zebrane impulsy / 10

4. **Weryfikacja:**
   - Sprawdź nową wartość "Imp/metr" — powinna odpowiadać specyfikacji enkodera i obwodowi koła
   - Status zmieni się na "OK"
   - Wartość jest zapisana w pamięci NVS — przetrwa restart

5. **Test po kalibracji:**
   - Wróć na ekran główny (STOP 1 s → STOP 1 s)
   - Przejedź znany odcinek i sprawdź czy wyświetlany dystans jest prawidłowy

> **Wskazówka:** Jeśli pomiar się nie udał (np. maszyna zjechała z prostej), naciśnij STOP (1 s) aby anulować i spróbuj ponownie.

---

### Przykład 6: Malowanie w trybie półautomatycznym

**Scenariusz:** Malowanie przejścia dla pieszych — operator chce kontrolować moment rozpoczęcia każdej nowej kreski, aby precyzyjnie pozycjonować linie.

**Kroki:**

1. **Ustawienie trybu SEMI:**
   - Na ekranie HOME przytrzymaj **START (1 s)** → ekran wyboru trybu
   - Klikaj **START** aż kursor wskaże **SEMI**
   - Przytrzymaj **START (1 s)** aby zatwierdzić — krótki beep potwierdzający
   - Lub w panelu WWW: kliknij przycisk **SEMI**

2. **Malowanie:**
   - Ustaw wzorzec (np. **P-1d** — prowadząca 1m/1m) przez panel WWW
   - Naciśnij **START** na ekranie HOME → rozpocznij malowanie
   - Ruszaj maszyną — pistolet maluje kreskę 1m automatycznie
   - Po 1m słyszysz krótki beep — kreska zakończona, pistolet się wyłącza
   - Przejedź do miejsca gdzie ma zacząć się kolejna kreska
   - Naciśnij **START** → pistolet zaczyna kolejną kreskę

3. **Zakończenie:**
   - Naciśnij **STOP** aby zakończyć malowanie

### Przykład 7: Malowanie w trybie ręcznym

**Scenariusz:** Malowanie specjalnych oznaczeń (np. strzałki, symbole) — operator sam decyduje kiedy pistolet strzela.

**Kroki:**

1. **Ustawienie trybu RĘCZNY:**
   - Na ekranie HOME przytrzymaj **START (1 s)** → ekran wyboru trybu
   - Klikaj **START** aż kursor wskaże **RĘCZNY**
   - Przytrzymaj **START (1 s)** aby zatwierdzić

2. **Malowanie:**
   - Naciśnij krótko **START** na HOME aby rozpocząć sesję malowania
   - Ruszaj maszyną powyżej 3 km/h
   - **Trzymaj START** → pistolety malują
   - **Puść START** → pistolety natychmiast się wyłączają
   - Statystyki (dystans, powierzchnia) naliczane są normalnie

### Przykład 8: Praca z GPS — raport z koordynatami lokalizacji

**Scenariusz:** Zleceniodawca wymaga dokumentacji potwierdzającej, że oznakowanie zostało wykonane w konkretnym miejscu. Moduł GPS rejestruje pozycję w raporcie CSV.

**Kroki:**

1. **Przed pracą — sprawdź GPS:**
   - Połącz się z WiFi TrassarV3 i otwórz panel WWW
   - Sprawdź sekcję "GPS" — powinno być: **Fix: TAK**, satelity ≥ 4, HDOP < 3.0
   - Jeśli brak fix — poczekaj 1-2 min, upewnij się że antena GPS widzi niebo

2. **Malowanie z rejestracją GPS:**
   - Pracuj normalnie — wybierz wzorzec, naciśnij START, maluj
   - GPS automatycznie zbiera dane w tle — nie wymaga żadnych dodatkowych akcji
   - W panelu WWW na żywo widoczna jest pozycja i prędkość GPS

3. **Po zakończeniu — raport z koordynatami:**
   - Naciśnij STOP — raport CSV automatycznie zapisze się z koordynatami GPS
   - W panelu WWW → Menu serwisowe → Raporty SD → pobierz plik CSV
   - Plik zawiera kolumny `lat` i `lon` z pozycją w momencie zakończenia sesji

**Przykład raportu CSV:**
```csv
data,godzina,wzorzec,dystans_m,powierzchnia_m2,lat,lon
2026-02-19,09:15:30,P-1a,1850.0,222.00,52.229676,21.012229
2026-02-19,11:40:15,P-3a,920.5,220.92,52.231500,21.015800
```

> **Wskazówka:** Koordynaty można wkleić do Google Maps, aby potwierdzić lokalizację prac.

---

### Przykład 9: Wieloetapowa praca z resetem sesji

**Scenariusz:** Ekipa maluje 3 odcinki drogi w ciągu dnia. Każdy odcinek wymaga osobnego raportu i świeżych liczników — operator chce widzieć dystans i powierzchnię tylko dla bieżącego etapu, bez narastających wartości z poprzednich odcinków.

**Etap 1 — malowanie pierwszego odcinka:**

1. Włącz maszynę, ustaw wzorzec **P-2a** (ciągła wąska) przez panel WWW
2. Naciśnij **START** → maluj pierwszy odcinek
3. Po zakończeniu naciśnij **STOP** — raport CSV zapisze się na kartę SD
4. Na ekranie HOME widoczny jest narosły dystans i powierzchnia z etapu 1

**Reset — przygotowanie do etapu 2:**

5. Przytrzymaj **STOP (1 s)** → menu serwisowe
6. Naciśnij **SELEKTOR** 4 razy aby dojść do pozycji 5: **Reset etapu**
7. Przytrzymaj **SELEKTOR (1 s)** aby wejść na ekran resetu
8. Na ekranie widoczne statystyki etapu 1: dystans, powierzchnia, czas
9. Naciśnij **START** (TAK) → krótki beep potwierdza, liczniki wyzerowane
10. System wraca na ekran HOME z zerowym dystansem i powierzchnią

**Etap 2 — malowanie drugiego odcinka:**

11. Zmień wzorzec na **P-1a** (przerywana długa) przez panel WWW
12. Naciśnij **START** → maluj drugi odcinek
13. Wyświetlacz pokazuje dane tylko z bieżącego etapu (od zera)
14. **STOP** → raport etapu 2 zapisany na SD

**Reset i etap 3:**

15. Powtórz kroki 5–10 (reset etapu)
16. Maluj trzeci odcinek
17. **STOP** → raport etapu 3 zapisany na SD

**Wynik na karcie SD — 3 osobne raporty:**
```csv
data,godzina,wzorzec,dystans_m,powierzchnia_m2,lat,lon
2026-02-23,08:30:15,P-2a,1250.5,150.06,52.229676,21.012229
2026-02-23,10:45:22,P-1a,875.3,105.04,52.230100,21.013500
2026-02-23,14:10:08,P-1a,430.0,51.60,52.231200,21.014100
```

> **Uwaga:** Statystyki lifetime (łączne) nie są zerowane — rosną przez cały dzień. Reset dotyczy wyłącznie liczników sesji widocznych na ekranie HOME i PAINTING.

---

### Przykład 10: Malowanie krawędzi jezdni na autostradzie

**Scenariusz:** Autostrada wymaga malowania linii krawędziowych po obu stronach. Prawa strona — linia ciągła szeroka (P-7b, 24 cm), lewa strona — linia przerywana wąska (P-7c, 12 cm). Praca w trybie automatycznym z GPS.

**Kroki — prawa krawędź (linia ciągła):**

1. Sprawdź GPS w panelu WWW — Fix: TAK, satelity ≥ 4
2. Ustaw wzorzec **P-7b** (krawędziowa ciągła szeroka) przez WWW
3. Upewnij się, że tryb to **AUTO** (domyślny)
4. Naciśnij **START** → ruszaj z prędkością 5–12 km/h
5. Pistolet P6 (24 cm) maluje ciągle — prostokąt P6 na ekranie zielony
6. Po zakończeniu odcinka naciśnij **STOP** → raport z GPS na SD

**Reset i zmiana na lewą krawędź:**

7. Menu serwisowe → Reset etapu → START (TAK)
8. Zmień wzorzec na **P-7c** (krawędziowa przerywana wąska) przez WWW
9. Przejedź na lewą stronę jezdni

**Lewa krawędź (linia przerywana):**

10. Naciśnij **START** → pistolet P5 (12 cm) maluje cyklicznie 1m / 2m
11. Na ekranie: P5 świeci zielono podczas kreski, gaśnie w przerwie
12. Po zakończeniu: **STOP** → osobny raport na SD

**Co widzisz na panelu WWW:**
- Sekcja GPS: pozycja na mapie aktualizowana na żywo
- Sekcja statystyki: osobne dane dla każdego etapu (dzięki resetowi)
- 2 raporty CSV z koordynatami GPS potwierdzającymi trasę

---

## 22. Rozwiązywanie problemów

| Problem | Możliwa przyczyna | Rozwiązanie |
|---------|-------------------|-------------|
| Wyświetlacz nie świeci | Brak podświetlenia | Sprawdź pin GPIO 21 (PWM LEDC) |
| Biały/czarny ekran | Źle skonfigurowane SPI | Sprawdź piny 10, 9, 14, 11, 12, 13 |
| Brak daty i czasu | DS1307 niedostępny | Sprawdź I2C (SDA=17, SCL=18) i baterię CR2032 |
| Nie można połączyć WiFi | Poza zasięgiem AP | Zbliż się do maszyny. SSID: TrassarV3, hasło: 12345678 |
| Enkoder nie rejestruje obrotu | Uszkodzony enkoder lub okablowanie | Sprawdź piny CLK=5, DT=6, sprawdź pull-upy |
| Przyciski nie działają | Złe podłączenie | Sprawdź GPIO 38/39/40/7 do GND, wewnętrzne pull-upy |
| ESP32 restartuje się w pętli | Piny PSRAM użyte | GPIO 26–37 zajęte przez PSRAM — nie podłączać! |
| Pistolety nie włączają się | Prędkość < 3 km/h | Przyspiesz powyżej 3 km/h lub użyj trybu czyszczenia dysz |
| Pistolety nie włączają się na postoju | Zabezpieczenie prędkości | To normalne zachowanie. Użyj Menu → Czyszczenie dysz |
| Złe odczyty dystansu | Brak kalibracji | Menu serwisowe → Kalibracja enkodera (10 m) |
| Złe długości kresek/przerw | Błędna kalibracja | Powtórz kalibrację na dokładnie odmierzonym odcinku 10 m |
| SELEKTOR nie zmienia wzorca | Usunięta funkcja | Wzorce zmienia się wyłącznie przez panel WWW |
| SELEKTOR nie odwraca | Nieodpowiedni wzorzec | Odwracanie działa tylko dla P-3a i P-3b |
| Karta SD nie działa | Błąd formatu lub podłączenia | Format FAT32, pin CS=GPIO 16, sprawdź poprawne włożenie |
| Brak raportów na karcie | Raport nie został zapisany | Raporty zapisują się po naciśnięciu STOP (zakończenie malowania) |
| "Start od przerwy" nie działa | Wzorzec ciągły | Dla P-2a/P-2b/P-4/P-7b/P-7d start od przerwy = normalny start |
| Panel WWW nie odpowiada | Serwer przeciążony | Max 4 klientów. Zamknij zbędne połączenia |
| Buzzer nie działa | Brak buzzera lub zły pin | Sprawdź podłączenie buzzera pasywnego do GPIO 8 |
| ESP32 restartuje się co 3 s | Watchdog timeout | Pętla główna się zawiesza — sprawdź monitor szeregowy |
| Pistolety wyłączają się co chwilę | Gun keepalive | Silnik malowania nie nadąża — sprawdź obciążenie procesora |
| Alarm prędkości miga ciągle | Próg za niski | Panel WWW → Alarm prędkości → zwiększ próg suwakiem |
| Prędkość miga na żółto | Niska prędkość | Przyspiesz powyżej 3 km/h — to ostrzeżenie, nie błąd |
| Anomalia pistoletu (buzzer 800 Hz) | Pistolet nie strzela | Sprawdź przekaźnik, przewód GPIO, dysze, zbiornik farby |
| `/api/reports` zwraca puste `[]` | Brak cache lub SD | Odczekaj 15 s (cache) lub sprawdź kartę SD |

---

---

## 23. Predykcja zużycia farby (v2.23.0)

### 23.1 Opis

System oblicza prognozowane zużycie farby na podstawie namalowanej powierzchni i współczynnika zużycia (domyślnie 0.6 l/m²). Umożliwia monitorowanie stanu zbiornika i planowanie uzupełnień.

### 23.2 Konfiguracja (panel WWW)

| Parametr | Domyślna | Zakres | Opis |
|----------|----------|--------|------|
| **Pojemność zbiornika** | 200 l | 1–9999 l | Całkowita pojemność zbiornika farby |
| **Współczynnik zużycia** | 0.6 l/m² | 0.01–99.0 | Ile litrów farby na metr kwadratowy |

Ustawienia zapisywane trwale w NVS.

### 23.3 Informacje dostępne

- **Zużyte litry** — ile farby zużyto w bieżącej sesji
- **Pozostałe litry** — szacunkowa ilość farby w zbiorniku
- **Pozostała powierzchnia** — ile m² jeszcze można namalować
- **Pozostały dystans** — szacunkowy dystans do wyczerpania farby

### 23.4 Ostrzeżenia

Gdy pozostaje mniej niż **20 litrów** farby, raport sesji oznacza to kolorem czerwonym.

---

## 24. Czujnik temperatury (v2.23.0, opcjonalny)

### 24.1 Opis

Opcjonalny czujnik DS18B20 (OneWire) monitoruje temperaturę otoczenia. Informacja wyświetlana jest na ekranie POST przy starcie systemu.

### 24.2 Progi ostrzeżeń

| Warunek | Próg | Znaczenie |
|---------|------|-----------|
| **Za zimno** | < 5°C | Farba może nie schnąć prawidłowo |
| **Za ciepło** | > 35°C | Ryzyko przegrzania komponentów |

### 24.3 Podłączenie

Czujnik podłączony do GPIO 15 (współdzielony z Touch CS — jeśli Touch nie jest używany). Wymagany rezystor pull-up 4.7kΩ między linią danych a 3.3V.

---

## 25. Raporty HTML sesji (v2.23.0)

### 25.1 Opis

Po każdym zatrzymaniu malowania (STOP) system automatycznie generuje raport HTML ze stylizowanym podsumowaniem sesji. Raporty dostępne do pobrania z panelu WWW.

### 25.2 Zawartość raportu

- Kod i nazwa wzorca, data, godzina
- Dystans, powierzchnia, czas malowania, średnia prędkość
- Rozbicie na poszczególne wzorce (jeśli zmieniano wzorzec podczas sesji)
- Zużycie farby (litry) i szacunkowa ilość pozostała
- Koordynaty GPS (jeśli dostępne)

### 25.3 Dostęp

- **Panel WWW:** Menu serwisowe → zakładka "Raporty HTML"
- **API:** `GET /api/html_reports` (lista) i `GET /api/html_reports/download?file=...`
- **Pliki:** `/html_reports/raport_RRRRMMDD_HHMMSS.html` na karcie SD

---

## 26. Zapis trasy GPS — GPX i GeoJSON (v2.23.0)

### 26.1 Opis

Podczas malowania system automatycznie zapisuje trasę GPS co 5 sekund. Po zatrzymaniu (STOP) trasa eksportowana jest na kartę SD w dwóch formatach:
- **GPX** — kompatybilny z Google Earth, QGIS, Strava, Garmin
- **GeoJSON** — kompatybilny z narzędziami GIS i mapami webowymi

### 26.2 Bufor PSRAM

Punkty GPS buforowane są w pamięci PSRAM (max 4320 punktów = ~6 godzin ciągłej pracy). Jeśli PSRAM niedostępny — fallback na RAM (300 punktów = ~25 min).

### 26.3 Dostęp

- **Panel WWW:** Menu serwisowe → zakładka "Trasy GPS" (planowane)
- **API:** `GET /api/tracks` (lista) i `GET /api/tracks/download?file=...`
- **Pliki:** `/tracks/trasa_RRRRMMDD_HHMMSS.gpx` i `.geojson` na karcie SD

---

## 27. Backup NVS na kartę SD (v2.23.0)

### 27.1 Opis

Wszystkie ustawienia z pamięci NVS (kalibracja, statystyki, wzorce własne, progi prędkości, tryby) są automatycznie backupowane na kartę SD w formacie JSON.

### 27.2 Automatyczny backup

- Wykonywany co **30 minut** podczas pracy
- Pierwszy backup przy każdym uruchomieniu systemu
- Plik: `/backup/nvs_backup.json`

### 27.3 Automatyczne przywracanie

Jeśli pamięć NVS jest pusta (np. po resecie fabrycznym) a na karcie SD istnieje plik backupu — system automatycznie przywraca ustawienia z backupu przy starcie.

### 27.4 Walidacja

Podczas przywracania system sprawdza:
- Wersję formatu danych NVS
- Poprawność zakresów wartości (progi prędkości, indeksy wzorców)
- Spójność danych (niepoprawne wartości są odrzucane, reszta przywracana)

---

## 28. Motogodziny (v2.23.0)

### 28.1 Opis

System rejestruje czas pracy silnika malowania (motogodziny, MTH) niezależnie od czasu sesji. Motogodziny naliczane są wyłącznie podczas aktywnego malowania (STATE_PAINTING).

### 28.2 Zapis

- Automatyczny zapis do NVS co **5 minut** podczas malowania
- Przeżywa restart urządzenia
- Dostępne przez API statystyk

### 28.3 Zastosowanie

- Planowanie przeglądów okresowych maszyny
- Szacowanie żywotności komponentów (dysze, przekaźniki)
- Rozliczenie czasu pracy na zleceniach

---

## 29. Auto-pauza i auto-wznowienie (v2.23.0)

### 29.1 Opis

System automatycznie pauzuje malowanie gdy maszyna staje (np. na skrzyżowaniu) i wznawia po ruszeniu.

### 29.2 Parametry

| Parametr | Wartość | Opis |
|----------|---------|------|
| **Próg auto-pauzy** | 0.5 km/h | Prędkość poniżej której aktywuje się auto-pauza |
| **Opóźnienie** | 1.5 s | Czas oczekiwania poniżej progu przed auto-pauzą |
| **Auto-wznowienie** | Konfigurowalne | Automatyczne wznowienie po przekroczeniu progu min. prędkości |

### 29.3 Sygnalizacja

- Buzzer sygnalizuje auto-pauzę odrębnym dźwiękiem (innym niż ręczna pauza)
- Na ekranie: status "Pauza" (żółty)
- W logach: `AUTO-PAUZA: predkosc X < Y km/h`

### 29.4 Konfiguracja

Auto-wznowienie można włączyć/wyłączyć z panelu WWW:
```
POST /api/control  action=set_auto_resume&value=1  (włącz)
POST /api/control  action=set_auto_resume&value=0  (wyłącz)
```

---

## 30. Tryb DEMO (v2.23.0)

### 30.1 Opis

Tryb nauki operatora — logika identyczna jak AUTO, ale pistolety **nie strzelają fizycznie**. Wizualizacja na ekranie i panelu WWW pokazuje, które pistolety by strzelały.

### 30.2 Zastosowanie

- Szkolenie nowych operatorów bez zużywania farby
- Weryfikacja konfiguracji wzorca przed rzeczywistym malowaniem
- Demonstracja systemu dla klientów

### 30.3 Aktywacja

Tryb DEMO dostępny jako czwarta opcja w selektorze trybu pracy (AUTO → SEMI → RĘCZNY → DEMO).

---

## 31. WebSocket — aktualizacje w czasie rzeczywistym (v2.23.0)

### 31.1 Opis

Oprócz HTTP polling (co 1 s), system oferuje kanał WebSocket na porcie 81. Panel WWW automatycznie łączy się z WebSocket i otrzymuje aktualizacje stanu co 500 ms (push).

### 31.2 Zalety

- **Niższe opóźnienie** — dane przychodzą natychmiast, bez odpytywania
- **Mniejsze obciążenie sieci** — jeden kanał zamiast powtarzanych żądań HTTP
- **Szybsza reakcja UI** — przełączanie wzorców, zmiany stanu widoczne w <1 s

### 31.3 Kompatybilność

Jeśli WebSocket nie jest dostępny (starsza przeglądarka), panel automatycznie fallbackuje na HTTP polling.

---

## 32. Tryb nocny (v2.23.0)

### 32.1 Opis

Tryb nocny zmienia kolorystykę wyświetlacza TFT na ciemne tony amber, redukując oślepienie operatora podczas pracy w nocy lub o zmroku.

### 32.2 Paleta kolorów

| Element | Tryb dzienny | Tryb nocny |
|---------|-------------|------------|
| Tło | Czarny | Czarny |
| Tekst | Biały | Ciepły amber |
| Akcent | Zielony | Pomarańczowy |
| Ostrzeżenie | Żółty | Ciemny żółty |
| Błąd | Czerwony | Ciemny czerwony |
| Pistolet ON | Zielony | Pomarańczowy |

### 32.3 Aktywacja

Tryb nocny zapisywany jest w NVS i można go przełączyć z panelu WWW.

---

## 33. Ekran POST — diagnostyka startowa (v2.23.0)

### 33.1 Opis

Przy każdym uruchomieniu system wyświetla ekran Power-On Self-Test (POST) z wynikami diagnostyki wszystkich modułów sprzętowych.

### 33.2 Sprawdzane moduły

| Moduł | Status OK | Status FAIL |
|-------|-----------|-------------|
| Karta SD | Zamontowana, FAT32 | Brak lub błąd formatu |
| Zegar RTC | Działa, czas poprawny | Niedostępny na I2C |
| GPS | Fix aktywny | Brak fix (normalne przy starcie) |
| MCP23017 | Odpowiada na 0x20 | Brak odpowiedzi I2C |
| Enkoder | Skalibrowany | Domyślna wartość |
| Czujnik temp. | Wykryty, odczyt OK | Niedostępny |

### 33.3 Obsługa

- Ekran POST wyświetla się automatycznie na **0.8 sekundy**
- Następnie czeka na naciśnięcie **START** lub timeout **5 sekund**
- Po przejściu — krótki beep (2 kHz) i przejście na ekran HOME

---

## 34. Dodatkowe przykłady zastosowania

### Przykład 11: Konfiguracja wzorca własnego przez panel WWW

**Scenariusz:** Zlecenie wymaga niestandardowego oznakowania: dwa pistolety jednocześnie, P2 z cyklem 3m/2m i P5 z cyklem 1m/1m.

**Kroki:**

1. Połącz się z WiFi TrassarV3 i otwórz panel WWW
2. Przewiń do sekcji "Wzorzec własny"
3. Wybierz zakładkę **Slot 1**
4. Ustaw pistolety:
   - P1: Wyłączony
   - P2: Przerywany, kreska: 3.0 m, przerwa: 2.0 m
   - P3: Wyłączony
   - P4: Wyłączony
   - P5: Przerywany, kreska: 1.0 m, przerwa: 1.0 m
   - P6: Wyłączony
5. Kliknij **Zapisz** — wzorzec zapisany do NVS
6. Kliknij przycisk **WŁASNY** w sekcji wzorców
7. Na ekranie TFT pojawi się "WLASNY" z informacją o aktywnych pistoletach
8. Naciśnij **START** — oba pistolety malują niezależnie ze swoimi cyklami

**Rezultat:** P2 i P5 malują jednocześnie, ale z różnymi wzorami — P2 tworzy długie kreski, P5 krótkie.

---

### Przykład 12: Inteligentne przełączanie wzorców (Smart Switch) podczas malowania

**Scenariusz:** Malowanie drogi z P-1a (przerywana 4m/8m), na 300 m od startu trzeba przejść na P-1c (wydzielająca 2m/2m) bez przerywania pracy.

**Kroki (Smart Switch):**

1. Rozpocznij malowanie z wzorcem P-1a (tryb AUTO)
2. W panelu WWW kliknij **P-1c** — przycisk P-1c zacznie **migać pomarańczowo** (wzorzec kolejkowany)
3. Maszyna kontynuuje malowanie P-1a:
   - Domalowuje bieżącą kreskę (4 m) do końca
   - Przejeżdża pełną przerwę (8 m) do końca
4. Po zakończeniu pełnego cyklu P-1a:
   - Krótki sygnał buzzera (1500 Hz, 80 ms) potwierdza przełączenie
   - P-1c staje się aktywna — nowy cykl 2m kreska / 2m przerwa
   - W panelu WWW P-1c zmienia kolor na zielony

**Anulowanie:** Kliknij ponownie aktywny wzorzec (P-1a) aby anulować kolejkowaną zmianę.

**Tryb Instant (porównanie):** Gdyby tryb Smart był wyłączony, kliknięcie P-1c natychmiast przerwałoby bieżącą kreskę P-1a i rozpoczęło P-1c — potencjalnie ucięta kreska w połowie.

---

### Przykład 13: Praca z auto-pauzą na odcinku z przeszkodami

**Scenariusz:** Malowanie linii ciągłej P-2a na drodze z ruchem — maszyna musi wielokrotnie stawać na skrzyżowaniach.

**Kroki:**

1. Upewnij się, że **auto-wznowienie** jest włączone (domyślnie: tak)
2. Ustaw wzorzec P-2a, tryb AUTO, naciśnij START
3. Maluj normalnie — pistolet P2 maluje ciągle
4. Na skrzyżowaniu: zwolnij i zatrzymaj maszynę:
   - Po 1.5 s bezczynności system automatycznie pauzuje
   - Buzzer sygnalizuje auto-pauzę (odmienny dźwięk)
   - Pistolet P2 natychmiast się wyłącza
   - Na ekranie: "Pauza" (żółty)
5. Ruszaj ponownie:
   - Po przekroczeniu 3 km/h system automatycznie wznawia malowanie
   - Buzzer sygnalizuje wznowienie
   - Pistolet P2 włącza się z powrotem
6. Powtarzaj na kolejnych skrzyżowaniach — nie musisz dotykać żadnego przycisku!

**Wyłączenie auto-wznowienia:** W panelu WWW lub przez API `set_auto_resume&value=0`. Wtedy po auto-pauzie operator musi ręcznie nacisnąć START.

---

### Przykład 14: Szkolenie operatora w trybie DEMO

**Scenariusz:** Nowy operator musi nauczyć się obsługi maszyny bez zużywania farby.

**Kroki:**

1. Ustaw tryb DEMO: HOME → START (1 s) → ekran SETUP → SELEKTOR (1 s) cykluj do DEMO → START
2. Wybierz dowolny wzorzec, np. P-3a (przekraczalna)
3. Naciśnij START — system przechodzi w stan "Malowanie"
4. Ruszaj maszyną:
   - Na ekranie prostokąty P1 i P3 zmieniają kolory jak przy prawdziwym malowaniu
   - Panel WWW pokazuje animowane kółka pistoletów
   - **Przekaźniki NIE włączają się** — żadna farba nie jest zużywana
5. Operator widzi jak działa system: kiedy strzela kreska, kiedy jest przerwa, jak wygląda odwracanie wzorca (SELEKTOR)
6. Po zakończeniu: STOP — system podsumowuje sesję (dystans, czas), ale zużycie farby = 0

---

### Przykład 15: Eksport trasy GPS do Google Earth

**Scenariusz:** Zleceniodawca chce wizualizację trasy malowania na mapie satelitarnej.

**Kroki:**

1. Maluj normalnie z aktywnym GPS (Fix: TAK)
2. Po zakończeniu (STOP) system automatycznie zapisuje pliki:
   - `/tracks/trasa_20260308_091500.gpx`
   - `/tracks/trasa_20260308_091500.geojson`
3. Pobierz plik GPX z panelu WWW → Menu serwisowe → Trasy GPS → Pobierz
4. Otwórz Google Earth Pro → Plik → Otwórz → wybierz plik `.gpx`
5. Trasa malowania wyświetli się na mapie satelitarnej z punktami co 5 s

**Alternatywnie:** Plik `.geojson` można otworzyć w QGIS, geojson.io, lub dowolnym narzędziu GIS.

---

### Przykład 16: Praca z fizycznymi przyciskami wzorców na panelu

**Scenariusz:** Operator często zmienia wzorce i chce szybko przełączać bez telefonu.

**Kroki:**

1. Na panelu maszyny jest 15 dedykowanych przycisków (po jednym na wzorzec)
2. Naciśnij przycisk **P-1b** → wzorzec zmienia się natychmiast + krótki beep
3. Na ekranie TFT zmienia się kod wzorca i nazwa
4. Podczas malowania naciśnij przycisk **P-2a**:
   - Jeśli Smart Switch włączony: P-2a kolejkowane do końca cyklu
   - Jeśli Instant: natychmiastowa zmiana
5. Przycisk WŁASNY nie jest dostępny na panelu fizycznym — tylko przez WWW

> **Uwaga:** Jeśli MCP23017 jest niedostępny (niepodłączony), system działa normalnie — przyciski wzorców po prostu nie reagują, zmiana przez WWW nadal działa.

---

### Przykład 17: Backup i przywracanie ustawień po wymianie ESP32

**Scenariusz:** ESP32-S3 uległo awarii i zostało wymienione na nowe. Stare ustawienia (kalibracja, wzorce, statystyki) są na karcie SD.

**Kroki:**

1. Włóż kartę SD ze starego urządzenia do nowego ESP32-S3
2. Wgraj firmware TrassarV3 na nowe ESP32 (`pio run -t upload`)
3. Uruchom system — przy starcie system wykrywa:
   - Pamięć NVS jest pusta (nowe ESP32)
   - Na karcie SD istnieje `/backup/nvs_backup.json`
4. System automatycznie przywraca ustawienia z backupu:
   - Kalibracja enkodera
   - Ostatni wzorzec i tryb pracy
   - Progi prędkości
   - Wzorce własne (3 sloty)
   - Statystyki lifetime (dystans, powierzchnia, motogodziny)
   - Liczniki strzałów pistoletów
5. W logach: `[NVS_BACKUP] Przywrocono ustawienia z /backup/nvs_backup.json`
6. System gotowy do pracy z zachowanymi ustawieniami!

---

### Przykład 18: Monitorowanie zużycia farby na długim zleceniu

**Scenariusz:** Zlecenie na 5 km malowania P-1a. Zbiornik 200 litrów, współczynnik 0.6 l/m².

**Kroki:**

1. W panelu WWW ustaw parametry farby:
   - Pojemność zbiornika: 200 l
   - Współczynnik zużycia: 0.6 l/m²
2. Rozpocznij malowanie P-1a (12 cm, przerywana 4m/8m)
3. W trakcie pracy monitoruj w panelu:
   - Namalowana powierzchnia: np. 150 m²
   - Zużyte litry: 90 l (150 m² × 0.6)
   - Pozostało: 110 l
   - Szacunkowy pozostały dystans: ~1530 m
4. Gdy pozostaje < 20 l — system ostrzega w raporcie HTML (czerwony kolor)
5. Po każdym STOPie raport HTML zawiera sekcję zużycia farby

---

### Przykład 19: Diagnostyka systemu przy problemach

**Scenariusz:** Maszyna zachowuje się nietypowo — chcesz sprawdzić stan systemu.

**Kroki:**

1. **Ekran POST (restart):**
   - Zrestartuj urządzenie (USB off/on)
   - Ekran POST pokaże status każdego modułu (OK/FAIL)
   - GPS FAIL przy starcie jest normalne (cold start trwa 30-60 s)

2. **Monitor szeregowy:**
   - Podłącz komputer USB i otwórz `pio device monitor` (115200 baud)
   - Co 30 s wyświetlany jest log diagnostyczny:
     ```
     [DIAG] Heap: 185000/327680 B (min: 165000)  Frag: 12%  WWW-stack: 2048  Core: 1
     ```
   - Sprawdź: Heap > 100 KB, min > 80 KB, Frag < 30%

3. **Panel WWW — diagnostyka:**
   - Sekcja "System": wersja firmware, wolna RAM, uptime
   - Sekcja "Statystyki": dystans per pistolet, licznik strzałów
   - Anomalia pistoletów: pulsujący banner ostrzegawczy

4. **Logi zdarzeń na karcie SD:**
   - Pliki `/logs/RRRRMMDD.log` zawierają szczegółowe zdarzenia systemowe
   - Format: `HH:MM:SS [CATEGORY] message`
   - Kategorie: SYSTEM, ENGINE, ANOMALY, NVS, GPS

---

### Przykład 20: Pomiar dystansu niezależny od malowania

**Scenariusz:** Trzeba odmierzyć odległość na drodze bez malowania (np. do wyznaczenia początku oznakowania).

**Kroki:**

1. Wejdź w menu serwisowe: STOP (1 s) na ekranie HOME
2. Nawiguj do "Pomiar dystansu" (pozycja 2): SELEKTOR × 1
3. Przytrzymaj SELEKTOR (1 s) aby wejść
4. Naciśnij **START** — rozpocznij pomiar
5. Jedź maszyną po odcinku do zmierzenia
6. Na ekranie wyświetlany jest dystans w metrach (lub km powyżej 1000 m)
7. Naciśnij **START** aby wstrzymać pomiar
8. Naciśnij **STOP** (krótko) aby wyzerować licznik
9. STOP (1 s) — powrót do menu serwisowego

---

## 35. Rozwiązywanie problemów (rozszerzone)

### 35.1 Tabela problemów

| Problem | Możliwa przyczyna | Rozwiązanie |
|---------|-------------------|-------------|
| Wyświetlacz nie świeci | Brak podświetlenia | Sprawdź pin GPIO 21 (PWM LEDC) |
| Biały/czarny ekran | Źle skonfigurowane SPI | Sprawdź piny 10, 9, 14, 11, 12, 13 |
| Brak daty i czasu | DS1307 niedostępny | Sprawdź I2C (SDA=17, SCL=18) i baterię CR2032 |
| Nie można połączyć WiFi | Poza zasięgiem AP | Zbliż się do maszyny. SSID: TrassarV3, hasło: 12345678 |
| Enkoder nie rejestruje obrotu | Uszkodzony enkoder lub okablowanie | Sprawdź piny CLK=5, DT=6, sprawdź pull-upy |
| Przyciski nie działają | Złe podłączenie | Sprawdź GPIO 38/39/40/7 do GND, wewnętrzne pull-upy |
| ESP32 restartuje się w pętli | Piny PSRAM użyte | GPIO 26–37 zajęte przez PSRAM — nie podłączać! |
| Pistolety nie włączają się | Prędkość < 3 km/h | Przyspiesz powyżej 3 km/h lub użyj trybu czyszczenia dysz |
| Pistolety nie włączają się na postoju | Zabezpieczenie prędkości | To normalne zachowanie. Użyj Menu → Czyszczenie dysz |
| Złe odczyty dystansu | Brak kalibracji | Menu serwisowe → Kalibracja enkodera (10 m) |
| Złe długości kresek/przerw | Błędna kalibracja | Powtórz kalibrację na dokładnie odmierzonym odcinku 10 m |
| SELEKTOR nie zmienia wzorca | Tak zaprojektowane | Wzorce zmienia się przez panel WWW lub przyciski MCP23017 |
| SELEKTOR nie odwraca | Nieodpowiedni wzorzec | Odwracanie działa tylko dla P-3a i P-3b |
| Karta SD nie działa | Błąd formatu lub podłączenia | Format FAT32, pin CS=GPIO 16, sprawdź poprawne włożenie |
| Brak raportów na karcie | Raport nie został zapisany | Raporty zapisują się po naciśnięciu STOP (zakończenie malowania) |
| "Start od przerwy" nie działa | Wzorzec ciągły | Dla P-2a/P-2b/P-4/P-7b/P-7d start od przerwy = normalny start |
| Panel WWW nie odpowiada | Serwer przeciążony | Max 4 klientów. Zamknij zbędne połączenia |
| Buzzer nie działa | Brak buzzera lub zły pin | Sprawdź podłączenie buzzera pasywnego do GPIO 8 |
| ESP32 restartuje się co 3 s | Watchdog timeout | Pętla główna się zawiesza — sprawdź monitor szeregowy |
| Pistolety wyłączają się co chwilę | Gun keepalive | Silnik malowania nie nadąża — sprawdź obciążenie procesora |
| Alarm prędkości miga ciągle | Próg za niski | Panel WWW → Alarm prędkości → zwiększ próg suwakiem |
| Prędkość miga na żółto | Niska prędkość | Przyspiesz powyżej 3 km/h — to ostrzeżenie, nie błąd |
| Anomalia pistoletu (buzzer 800 Hz) | Pistolet nie strzela | Sprawdź przekaźnik, przewód GPIO, dysze, zbiornik farby |
| Przyciski wzorców MCP nie reagują | MCP23017 niedostępny | Sprawdź I2C (SDA=17, SCL=18), adres 0x20, zasilanie 3.3V |
| Auto-pauza nie działa | Tryb MANUAL | Auto-pauza nie działa w trybie ręcznym |
| GPS nie ma fix po 5 min | Słaby sygnał | Przenieś antenę na zewnątrz, z widocznością nieba |
| Brak plików tras GPS | GPS nie miał fix | Trasa wymaga min. 1 punktu GPS z fix |
| Raport HTML nie generuje się | Brak karty SD | Sprawdź kartę SD i format FAT32 |
| Backup NVS nie działa | Karta SD pełna | Zwolnij miejsce na karcie SD |

### 35.2 Kody dźwiękowe buzzera — szybka diagnostyka

| Dźwięk | Częstotliwość | Znaczenie |
|--------|---------------|-----------|
| 1× krótki wysoki beep | 2 kHz, 100 ms | Start malowania / wznowienie |
| 2× krótki beep | 2 kHz, 80 ms | Pauza / stop malowania |
| 2× puls | 1.5 kHz, 150 ms | Niska prędkość (< 3 km/h) |
| 3× szybki alarm | 3 kHz, 60 ms | Przekroczenie prędkości |
| Niski-wysoki-niski | 800→1200→800 Hz | Anomalia pistoletu |
| Opadający ton | 1000→800→600 Hz | Błąd sprzętowy (SD/RTC) |
| 1× krótki | 1 kHz, 50 ms | Semi: kreska gotowa |
| 1× krótki | 1.5 kHz, 80 ms | Potwierdzenie (tryb, wzorzec, semi) |
| Specjalny wzorzec | — | Auto-pauza |

---

---

## 36. Bezpieczeństwo i BHP

### 36.1 Ostrzeżenia ogólne

> **UWAGA! Urządzenie steruje zaworami pistoletów natryskowych farby drogowej pod ciśnieniem. Nieprzestrzeganie zasad bezpieczeństwa grozi obrażeniami ciała, uszkodzeniem mienia i zagrożeniem dla uczestników ruchu drogowego.**

### 36.2 Wymagania bezpieczeństwa przed rozpoczęciem pracy

| # | Czynność kontrolna | Kryterium |
|---|---------------------|-----------|
| 1 | Sprawdzenie stanu zaworów i węży | Brak przecieków, pęknięć, luźnych połączeń |
| 2 | Weryfikacja ciśnienia w układzie | Zgodne ze specyfikacją farby (typowo 2–6 bar) |
| 3 | Test pistoletów (czyszczenie dysz) | Każdy pistolet reaguje na przycisk START w trybie czyszczenia |
| 4 | Sprawdzenie enkodera i koła pomiarowego | Swobodny obrót, brak poślizgu, poprawna kalibracja |
| 5 | Sprawdzenie karty SD | Karta zamontowana, format FAT32, wolne miejsce |
| 6 | Sprawdzenie GPS (jeśli wymagany) | Fix: TAK, satelity ≥ 4 |
| 7 | Sprawdzenie poziomu farby w zbiorniku | Minimum 10% pojemności |
| 8 | Sprawdzenie zasilania | Zasilacz USB-C min. 1.5A, stabilne napięcie |

### 36.3 Zasady bezpieczeństwa podczas malowania

1. **NIGDY** nie kieruj pistoletów natryskowych w stronę ludzi, pojazdów ani zwierząt
2. **NIGDY** nie otwieraj układu ciśnieniowego farby podczas pracy
3. **ZAWSZE** noś odzież ochronną: okulary, rękawice, obuwie ochronne
4. **ZAWSZE** stosuj oznakowanie tymczasowe strefy robót zgodnie z przepisami
5. Praca przy temperaturze otoczenia **5–35°C** (poza tym zakresem farba może nie schnąć prawidłowo)
6. Przy prędkości wiatru > 20 km/h wstrzymaj malowanie (rozprysk farby)
7. Nie maluj na mokrej nawierzchni (adherencja farby)
8. Zabezpiecz teren prac wg Rozporządzenia MI z dnia 3 lipca 2003 r.

### 36.4 Bezpieczeństwo elektryczne

| Zagrożenie | Środek zaradczy |
|------------|-----------------|
| Porażenie prądem | System zasilany napięciem bezpiecznym 5V DC (SELV) — brak zagrożenia |
| Zwarcie | Moduł przekaźnikowy ma opto-izolację; ESP32 zabezpieczone wewnętrznie |
| Przepięcie | Diody zabezpieczające (flyback) wbudowane w moduł przekaźnikowy |
| Wilgoć | Elektronika w obudowie IP54 lub wyższej; nie zanurzać w wodzie |
| Wyładowania ESD | GPIO 26–37 nie podłączać (PSRAM); zachować ostrożność przy łączeniu modułów |

### 36.5 Procedura awaryjna

**Gdy system zachowuje się nieprawidłowo (pistolety nie wyłączają się, dziwne zachowanie):**

1. **Natychmiast odłącz zasilanie USB-C** — wszystkie GPIO przechodzą w stan LOW, pistolety się zamykają
2. Zamknij ręczny zawór główny farby (jeśli istnieje)
3. Sprawdź monitor szeregowy (jeśli dostępny) — logi pomogą zidentyfikować przyczynę
4. Po analizie: podłącz ponownie USB-C, system uruchomi się z zapisanymi ustawieniami

**Wbudowane zabezpieczenia automatyczne:**
- Watchdog (3 s) → automatyczny restart
- Gun keepalive (300 ms) → awaryjne wyłączenie pistoletów
- Minimalna prędkość (3 km/h) → pistolety OFF na postoju
- Auto-pauza (1.5 s) → pistolety OFF przy zatrzymaniu

### 36.6 Przeciwwskazania do użytkowania

- Na drogach z ruchem bez odpowiedniego zabezpieczenia i oznakowania
- Podczas opadów deszczu lub śniegu
- Przy temperaturze nawierzchni poniżej 5°C (farba nie przywrze)
- Na mokrej, oblodzonej lub zanieczyszczonej nawierzchni
- Bez ważnej kalibracji enkodera (błędne długości kresek i przerw)

---

## 37. Konserwacja i przeglądy

### 37.1 Przegląd codzienny (przed pracą)

| # | Czynność | Narzędzia | Czas |
|---|----------|-----------|------|
| 1 | Oględziny zewnętrzne przewodów i złączy | Wzrok | 2 min |
| 2 | Test pistoletów (czyszczenie dysz) | Menu → Czyszczenie dysz | 3 min |
| 3 | Sprawdzenie poziomu farby | Wzrokowy kontrola zbiornika | 1 min |
| 4 | Sprawdzenie koła pomiarowego | Dotyk — swobodny obrót | 1 min |
| 5 | Weryfikacja daty i czasu na ekranie | Ekran HOME lub panel WWW | 30 s |
| 6 | Sprawdzenie karty SD (wolne miejsce) | Panel WWW → System | 30 s |
| 7 | Test GPS (fix) | Panel WWW → GPS | 1 min |

### 37.2 Przegląd tygodniowy

| # | Czynność | Szczegóły |
|---|----------|-----------|
| 1 | Pobranie raportów z karty SD | Skopiuj pliki CSV na komputer, archiwizuj |
| 2 | Sprawdzenie statystyk pistoletów | Panel WWW → Statystyki → dystans per pistolet |
| 3 | Czyszczenie dysz pistoletów | Menu → Czyszczenie dysz → 10 s trzymaj START na każdym wzorcu |
| 4 | Sprawdzenie baterii CR2032 | Jeśli data resetuje się po wyłączeniu — wymień baterię |
| 5 | Sprawdzenie złączy Dupont | Dociśnij luźne złącza, zwłaszcza SPI i I2C |
| 6 | Czyszczenie wyświetlacza TFT | Miękka ściereczka, bez rozpuszczalników |

### 37.3 Przegląd miesięczny

| # | Czynność | Szczegóły |
|---|----------|-----------|
| 1 | Ponowna kalibracja enkodera | Menu → Kalibracja → 10 m pomiar |
| 2 | Sprawdzenie licznika strzałów pistoletów | Planowanie wymiany dysz na podstawie licznika |
| 3 | Archiwizacja raportów | Skopiuj zawartość `/reports/` i `/tracks/` na dysk |
| 4 | Backup karty SD | Skopiuj całą kartę na komputer |
| 5 | Sprawdzenie motogodzin | Panel WWW → Statystyki → MTH |
| 6 | Sprawdzenie anteny GPS | Poprawność fix, liczba satelitów |

### 37.4 Przegląd sezonowy (co 6 miesięcy lub 500 MTH)

| # | Czynność | Szczegóły |
|---|----------|-----------|
| 1 | Wymiana dysz pistoletów | Wg zaleceń producenta dysz |
| 2 | Sprawdzenie przekaźników | Test wszystkich 6 kanałów — kliknięcie powinno być słyszalne |
| 3 | Sprawdzenie enkodera | Obrót powinien być płynny, bez zacięć |
| 4 | Sprawdzenie węży ciśnieniowych | Brak pęknięć, wzdęć, przecieków |
| 5 | Aktualizacja firmware | Jeśli dostępna nowa wersja |
| 6 | Wymiana baterii CR2032 | Prewencyjnie co 12 miesięcy |

### 37.5 Tabela żywotności komponentów

| Komponent | Szacunkowa żywotność | Sygnał wymiany |
|-----------|---------------------|----------------|
| Dysze pistoletów | 200–500 MTH | Anomalia pistoletu, nierówny natrysk |
| Przekaźniki | 100 000 cykli (~2–5 lat) | Brak kliknięcia, pistolet nie reaguje |
| Enkoder obrotowy | 50 000–200 000 obrotów | Skoki dystansu, brak rejestracji |
| Bateria CR2032 | 3–5 lat | Data resetuje się po wyłączeniu |
| Karta MicroSD | 100 000 cykli zapisu (~5 lat) | Błędy zapisu, utrata plików |
| Koło pomiarowe | 1–2 sezony | Zużycie bieżnika, poślizg |
| Przewody Dupont | 2–3 lata | Utrata kontaktu, korozja |

### 37.6 Czyszczenie i przechowywanie

**Po zakończeniu dnia pracy:**
1. Wyczyść dysze pistoletów (Menu → Czyszczenie dysz, 10 s na pistolet)
2. Zamknij zawór główny farby
3. Odłącz zasilanie USB-C
4. Zabezpiecz elektronikę przed wilgocią (okrycie, obudowa)

**Przechowywanie długoterminowe (>1 miesiąc):**
1. Wyczyść układ farby rozpuszczalnikiem
2. Wyjmij kartę SD i zarchiwizuj dane
3. Odłącz baterię CR2032 (zapobieganie wyładowaniu)
4. Przechowuj w suchym pomieszczeniu, temp. 0–40°C
5. Zabezpiecz wyświetlacz TFT przed zarysowaniem

---

## 38. Aktualizacja firmware

### 38.1 Wymagania

- Komputer z zainstalowanym **PlatformIO** (VS Code + rozszerzenie PlatformIO IDE)
- Kabel USB-C do USB-A/C
- Plik projektu TrassarV3 (repozytorium git)

### 38.2 Procedura aktualizacji

```
1. Podłącz ESP32-S3 kablem USB-C do komputera
2. Otwórz terminal w katalogu projektu TrassarV3
3. Skompiluj i wgraj firmware:
   $ pio run --target upload
4. Po zakończeniu — ESP32 automatycznie się zrestartuje
5. Sprawdź wersję firmware w panelu WWW → System → Wersja
```

### 38.3 Weryfikacja po aktualizacji

| Krok | Czynność | Oczekiwany wynik |
|------|----------|------------------|
| 1 | Ekran POST | Wszystkie moduły OK |
| 2 | Panel WWW | Nowa wersja firmware w sekcji System |
| 3 | Kalibracja | Zachowana z NVS (bez utraty) |
| 4 | Wzorce własne | Zachowane z NVS |
| 5 | Statystyki lifetime | Zachowane z NVS |
| 6 | Raporty na SD | Nienaruszone |

### 38.4 Rollback (przywracanie starej wersji)

Jeśli nowa wersja nie działa poprawnie:
1. Przywróć starszą wersję kodu z repozytorium git
2. `pio run --target upload`
3. Ustawienia NVS pozostaną niezmienione (chyba że zmienił się format `NVS_DATA_VERSION`)

---

## 39. Specyfikacja zgodności z normami drogowymi

### 39.1 Normy polskie — oznakowanie poziome dróg

System TrassarV3 implementuje wzorce zgodne z:
- **Rozporządzenie Ministra Infrastruktury z dnia 3 lipca 2003 r.** w sprawie szczegółowych warunków technicznych dla znaków i sygnałów drogowych oraz urządzeń bezpieczeństwa ruchu drogowego (Dz.U. 2003 Nr 220)
- **Załącznik nr 2** — Szczegółowe warunki techniczne dla znaków drogowych poziomych

### 39.2 Tabela zgodności wzorców z normą

| Wzorzec | Oznaczenie normowe | Szerokość [cm] | Kreska [m] | Przerwa [m] | Stosunek | Zastosowanie wg normy |
|---------|-------------------|----------------|------------|-------------|----------|----------------------|
| **P-1a** | Linia przerywana | 12 | 4.0 | 8.0 | 1:2 | Oddzielanie pasów ruchu na prostej |
| **P-1b** | Linia przerywana krótka | 12 | 2.0 | 4.0 | 1:2 | Oddzielanie pasów ruchu na skrzyżowaniu |
| **P-1c** | Linia wydzielająca | 12 | 2.0 | 2.0 | 1:1 | Wydzielanie pasów włączania/wyłączania |
| **P-1d** | Linia prowadząca wąska | 12 | 1.0 | 1.0 | 1:1 | Prowadzenie ruchu na skrzyżowaniach |
| **P-1e** | Linia prowadząca szeroka | 24 | 1.0 | 1.0 | 1:1 | Prowadzenie ruchu (pas większy) |
| **P-2a** | Linia ciągła wąska | 12 | — | — | — | Zakaz przekraczania, oddzielanie pasów |
| **P-2b** | Linia ciągła szeroka | 24 | — | — | — | Oddzielanie jezdni od pobocza |
| **P-3a** | Linia jednostronnie przekraczalna | 12+12 | 4.0/2.0 | — | — | Strefa zakazu wyprzedzania (od strony ciągłej) |
| **P-3b** | Linia jednostronnie przekraczalna | 12+12 | 1.0/1.0 | — | — | Strefa zakazu wyprzedzania (krótki cykl) |
| **P-4** | Linia podwójna ciągła | 12+12 | — | — | — | Zakaz przekraczania w obu kierunkach |
| **P-6** | Linia ostrzegawcza | 12 | 4.0 | 2.0 | 2:1 | Zbliżanie się do linii ciągłej |
| **P-7a** | Linia krawędziowa przerywana szer. | 24 | 1.0 | 1.0 | 1:1 | Krawędź jezdni (zjazdy, przystanki) |
| **P-7b** | Linia krawędziowa ciągła szer. | 24 | — | — | — | Krawędź jezdni (zakaz zjazdu) |
| **P-7c** | Linia krawędziowa przerywana wąska | 12 | 1.0 | 1.0 | 1:1 | Krawędź jezdni (drogi niższe klasy) |
| **P-7d** | Linia krawędziowa ciągła wąska | 12 | — | — | — | Krawędź jezdni (drogi niższe klasy) |

### 39.3 Tolerancje wg normy

| Parametr | Tolerancja normy | Tolerancja systemu TrassarV3 |
|----------|------------------|------------------------------|
| Szerokość linii | ±10% | Zależy od dysz pistoletów (montaż mechaniczny) |
| Długość kreski | ±5% | Zależy od kalibracji enkodera (typowo ±2%) |
| Długość przerwy | ±5% | Zależy od kalibracji enkodera (typowo ±2%) |
| Prostoliniowość | ±5 cm/10 m | Zależy od operatora i maszyny |

### 39.4 Uwaga dotycząca odpowiedzialności

Operator jest odpowiedzialny za:
- Poprawny dobór wzorca do rodzaju oznakowania
- Prawidłową kalibrację enkodera (wpływa na dokładność kresek/przerw)
- Właściwy montaż pistoletów (wpływa na szerokość linii)
- Przestrzeganie przepisów o ruchu drogowym podczas prac

---

## 40. Słownik pojęć

| Termin | Opis |
|--------|------|
| **ADC** | Analog-to-Digital Converter — przetwornik analogowo-cyfrowy (joystick) |
| **AP** | Access Point — tryb punktu dostępu WiFi (ESP32 tworzy własną sieć) |
| **Auto-pauza** | Automatyczne wstrzymanie malowania przy zatrzymaniu maszyny |
| **BOM** | Bill of Materials — lista materiałów do montażu |
| **Cold start** | Pierwsze uruchomienie GPS po długim wyłączeniu (~30–60 s do fix) |
| **Core 0 / Core 1** | Dwa rdzenie procesora ESP32-S3; Core 1 = krytyczne zadania, Core 0 = WiFi |
| **CR2032** | Bateria litowa 3V podtrzymująca zegar RTC |
| **Debounce** | Eliminacja drgań styków przycisków (50 ms) |
| **Demo** | Tryb demonstracyjny — wizualizacja bez uruchamiania pistoletów |
| **Dystans sesji** | Odległość przejechana w bieżącym etapie malowania |
| **Enkoder** | Obrotowy przetwornik impulsów montowany na kole pomiarowym |
| **FAT32** | System plików karty SD |
| **Fix (GPS)** | Poprawna lokalizacja z satelitów (min. 3 satelity) |
| **fmod** | Funkcja reszty z dzielenia — oblicza pozycję w cyklu kreska/przerwa |
| **FreeRTOS** | System operacyjny czasu rzeczywistego (zarządzanie taskami) |
| **GAP** | Start od przerwy — rozpoczęcie malowania od fazy przerwy |
| **GeoJSON** | Format zapisu tras GPS kompatybilny z narzędziami GIS |
| **GPIO** | General Purpose Input/Output — piny mikrokontrolera |
| **GPX** | GPS Exchange Format — format zapisu tras kompatybilny z Google Earth |
| **Gun keepalive** | Mechanizm bezpieczeństwa wyłączający pistolety po 300 ms braku aktualizacji |
| **HDOP** | Horizontal Dilution of Precision — miara dokładności GPS (niższa = lepsza) |
| **HSPI** | Hardware SPI port (SPI3) — magistrala do wyświetlacza i karty SD |
| **I2C** | Magistrala komunikacyjna (RTC DS1307 + MCP23017) |
| **Imp/metr** | Impulsy enkodera na metr — wynik kalibracji |
| **Instant** | Tryb natychmiastowej zmiany wzorca (bez czekania na koniec cyklu) |
| **ISR** | Interrupt Service Routine — przerwanie sprzętowe (enkoder) |
| **LEDC** | LED Control — moduł PWM ESP32 (buzzer + podświetlenie TFT) |
| **Lifetime** | Statystyki łączne od początku pracy urządzenia |
| **MCP23017** | Ekspander I2C 16-bitowy (15 przycisków wzorców) |
| **MTH** | Motogodziny — czas pracy silnika malowania |
| **Mutex** | Mechanizm synchronizacji dostępu do współdzielonych zasobów |
| **NMEA** | Standard komunikacji GPS (protokół sentencji tekstowych) |
| **NVS** | Non-Volatile Storage — pamięć trwała ESP32 (ustawienia, kalibracja) |
| **Octal PSRAM** | 8 MB pamięci RAM na ESP32-S3 N16R8 (bufor GPS) |
| **ODW** | Znacznik odwrócenia wzorca (P-3a/P-3b) |
| **OTA** | Over-The-Air — aktualizacja firmware przez WiFi (planowane) |
| **POST** | Power-On Self-Test — diagnostyka przy uruchomieniu |
| **PSRAM** | Pseudo-Static RAM — dodatkowa pamięć na ESP32-S3 |
| **Pull-up** | Rezystor podciągający pin do stanu HIGH (wbudowany w ESP32) |
| **PWM** | Pulse Width Modulation — modulacja szerokości impulsu (buzzer, LED) |
| **Semi-auto** | Tryb półautomatyczny — kreska auto, przerwa ręczna |
| **Sesja** | Bieżący etap malowania (od START do STOP) |
| **Smart Switch** | Inteligentne przełączanie wzorców (czeka na koniec cyklu) |
| **SPI** | Serial Peripheral Interface — magistrala do TFT, SD |
| **Strap pin** | Pin konfiguracyjny ESP32 wpływający na tryb bootowania (GPIO 46) |
| **TFT** | Thin Film Transistor — technologia wyświetlacza kolorowego |
| **UART** | Universal Asynchronous Receiver-Transmitter — port szeregowy (GPS) |
| **Warm start** | Uruchomienie GPS po krótkim wyłączeniu (~1–5 s do fix) |
| **Watchdog (WDT)** | Timer bezpieczeństwa resetujący ESP32 po 3 s zawieszenia |
| **WebSocket** | Protokół dwukierunkowej komunikacji w czasie rzeczywistym (port 81) |

---

## 41. Karta gwarancyjna i dane kontaktowe

### 41.1 Warunki gwarancji

Firmware TrassarV3 jest dostarczany w stanie "AS IS". Gwarancja obejmuje poprawne działanie oprogramowania zgodnie z niniejszą dokumentacją przy prawidłowym montażu sprzętowym.

### 41.2 Wyłączenia gwarancji

- Uszkodzenia wynikające z nieprawidłowego montażu (podłączenie pinów PSRAM GPIO 26–37)
- Uszkodzenia spowodowane zasilaniem nieodpowiednim (>5.5V, <4.5V)
- Uszkodzenia mechaniczne komponentów
- Modyfikacje firmware bez autoryzacji producenta

### 41.3 Dane techniczne urządzenia

```
┌───────────────────────────────────────────────┐
│            KARTA IDENTYFIKACYJNA               │
├───────────────────────────────────────────────┤
│  Nazwa:        TrassarV3                       │
│  Firmware:     v2.23.0                         │
│  MCU:          ESP32-S3 N16R8                  │
│  Flash:        16 MB                           │
│  PSRAM:        8 MB                            │
│  Wyświetlacz:  ILI9341 2.8" TFT 320×240       │
│  GPS:          NEO-6M (GY-NEO6MV2)            │
│  Ekspander:    MCP23017 (0x20)                 │
│  RTC:          DS1307 + CR2032                 │
│  Pistoletów:   6 (P1–P6)                      │
│  Wzorców:      16 (15 + własny)                │
│  Trybów:       4 (AUTO/SEMI/MANUAL/DEMO)       │
│  WiFi:         AP "TrassarV3" / 12345678       │
│  Panel WWW:    http://192.168.4.1              │
│  WebSocket:    ws://192.168.4.1:81             │
│  Zasilanie:    USB-C 5V / min. 1.5A           │
│  Data kompil.: __DATE__                        │
│  Kod źródłowy: ~9750 linii, 52 pliki          │
└───────────────────────────────────────────────┘
```

---

*TrassarV3 — Komputer pokładowy malowarki pasów drogowych*
*Firmware v2.23.0 | ESP32-S3 N16R8 | GPS NEO-6M + GPX/GeoJSON | 6 pistoletów, 16 wzorców, 4 tryby pracy (AUTO/SEMI/MANUAL/DEMO), Smart/Instant, auto-pauza, backup NVS, motogodziny, predykcja farby, raporty HTML, tryb nocny, WebSocket*
*Dokumentacja aktualizowana: marzec 2026*
