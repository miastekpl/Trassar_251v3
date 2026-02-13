# TrassarV3 - Instrukcja obsługi v2.3.0

## Spis treści

1. [Opis ogólny](#1-opis-ogólny)
2. [Dane techniczne](#2-dane-techniczne)
3. [Panel sterowania](#3-panel-sterowania)
4. [Pistolety natryskowe](#4-pistolety-natryskowe)
5. [Wzorce malowania](#5-wzorce-malowania)
6. [Ekrany interfejsu](#6-ekrany-interfejsu)
7. [Funkcja "Start od przerwy"](#7-funkcja-start-od-przerwy)
8. [Panel WWW (zdalny dostęp)](#8-panel-www-zdalny-dostęp)
9. [Kalibracja enkodera](#9-kalibracja-enkodera)
10. [Raporty na karcie SD](#10-raporty-na-karcie-sd)
11. [Zabezpieczenia](#11-zabezpieczenia)
12. [Przykłady zastosowania](#12-przykłady-zastosowania)
13. [Rozwiązywanie problemów](#13-rozwiązywanie-problemów)

---

## 1. Opis ogólny

**TrassarV3** to komputer pokładowy malowarki pasów drogowych oparty na mikrokontrolerze ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM). System steruje **6 pistoletami natryskowymi** (P1–P6) poprzez moduły przekaźnikowe, obsługuje **15 wzorców malowania** zgodnych z polskimi normami oznakowania poziomego dróg i mierzy dystans oraz prędkość za pomocą enkodera obrotowego montowanego na kole pomiarowym.

System zapewnia:
- Automatyczne sterowanie pistoletami w czasie malowania (ciągłe, przerywane, mieszane)
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
| Firmware | v2.3.0 |
| Wyświetlacz | ILI9341 2.8" TFT, 320×240 px, tryb landscape |
| Interfejs SPI | HSPI (SPI3), 27 MHz |
| Zegar RTC | DS1307 z baterią CR2032 |
| Enkoder | Obrotowy, ISR na pinie CLK (CHANGE) |
| Przyciski | 4 szt. monostabilne (START, STOP, SELEKTOR, GAP) |
| Przekaźniki | 6 szt. (pistolety P1–P6), logika HIGH = ON |
| Karta SD | Slot zintegrowany w module wyświetlacza, FAT32 |
| WiFi | Access Point, SSID: TrassarV3, hasło: 12345678 |
| Serwer WWW | HTTP port 80, max 4 klientów, auto-refresh 1 s |
| Prędkość min. | 3 km/h (zabezpieczenie pistoletów) |
| Kalibracja | Odcinek 10 m, zapis do NVS |
| Zasilanie | USB-C 5V (ESP32-S3 DevKit) |

---

## 3. Panel sterowania

### 3.1 Przyciski funkcyjne

System posiada **4 przyciski fizyczne**. Każdy przycisk obsługuje krótkie naciśnięcie (klik) oraz — w wybranych przypadkach — długie naciśnięcie (przytrzymanie 1 s).

| Przycisk | GPIO | Krótkie naciśnięcie | Długie naciśnięcie (1 s) |
|----------|------|---------------------|--------------------------|
| **START** | 38 | Start malowania / Pauza / Wznowienie | — |
| **STOP** | 39 | Zatrzymanie malowania / Cofnij w menu | Wejście w menu serwisowe / Powrót |
| **SELEKTOR** | 40 | *Zależy od ekranu (patrz niżej)* | Wejdź w opcję menu |
| **GAP (od przerwy)** | 7 | Start od przerwy (na ekranie HOME) | — |

### 3.2 Funkcja selektora w zależności od ekranu

Przycisk **SELEKTOR** pełni różne funkcje w zależności od aktualnie wyświetlanego ekranu:

| Ekran | Krótkie naciśnięcie | Długie naciśnięcie (1 s) |
|-------|---------------------|--------------------------|
| **Ekran główny (HOME)** | Odwróć wzorzec (tylko P-3a / P-3b)* | — |
| **Ekran malowania** | Odwróć wzorzec (tylko P-3a / P-3b)* | — |
| **Menu serwisowe** | **Następna pozycja w menu** | **Wejdź w wybraną opcję** |
| **Czyszczenie dysz** | Następny wzorzec | Poprzedni wzorzec |

> \* Na ekranie głównym i ekranie malowania selektor służy **wyłącznie** do odwracania wzorców P-3a i P-3b. Dla pozostałych wzorców krótkie naciśnięcie jest ignorowane. Zmiana wzorca odbywa się wyłącznie przez panel WWW.

### 3.3 Enkoder obrotowy

Enkoder obrotowy (piny CLK = GPIO 5, DT = GPIO 6) służy **wyłącznie** do pomiaru dystansu i prędkości. **Nie jest używany do nawigacji ani sterowania interfejsem.** Obrót enkodera jest rejestrowany przez przerwanie sprzętowe (ISR) na pinie CLK.

Wbudowany przycisk enkodera (pin SW = GPIO 7) pełni funkcję dedykowanego przycisku **"Start od przerwy"**.

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

System obsługuje 15 wzorców zgodnych z polskimi normami oznakowania poziomego:

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

### 5.2 Odwracanie wzorców (P-3a, P-3b)

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

## 6. Ekrany interfejsu

### 6.1 Ekran główny (HOME)

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
| **START** | Rozpocznij malowanie od początku wzorca |
| **GAP** (GPIO 7) | Start od przerwy — rozpocznij od przerwy we wzorcu |
| **SELEKTOR** | Odwróć wzorzec (tylko P-3a / P-3b) |
| **STOP (1 s)** | Wejdź do menu serwisowego |

### 6.2 Ekran malowania (PAINTING)

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

**Sterowanie na ekranie malowania:**

| Przycisk | Akcja |
|----------|-------|
| **START** | Pauza (gdy maluje) / Wznowienie (gdy pauza) |
| **STOP** | Zatrzymanie malowania, zapis raportu, powrót do HOME |
| **SELEKTOR** | Odwróć wzorzec (tylko P-3a / P-3b) |

> **Zabezpieczenie:** Pistolety włączają się automatycznie dopiero po osiągnięciu prędkości **3 km/h**. Poniżej tej prędkości pistolety pozostają wyłączone nawet w stanie "Malowanie".

### 6.3 Menu serwisowe

Dostęp: **STOP (1 s)** na ekranie głównym.

Ekran wyświetla 4 pozycje z nagłówkiem "SERWIS":

| # | Pozycja | Opis |
|---|---------|------|
| 1 | **Kalibracja enkodera** | Procedura kalibracyjna na odcinku 10 m |
| 2 | **Pomiar dystansu** | Ręczny pomiar odległości (niezależny od malowania) |
| 3 | **Raporty** | Przeglądanie raportów z karty SD |
| 4 | **Czyszczenie dysz** | Ręczne uruchamianie pistoletów |

**Nawigacja w menu serwisowym:**

| Przycisk | Akcja |
|----------|-------|
| **SELEKTOR (krótko)** | Następna pozycja (w dół) |
| **STOP (krótko)** | Poprzednia pozycja (w górę) |
| **SELEKTOR (1 s)** | Wejdź w wybraną opcję |
| **STOP (1 s)** | Powrót do ekranu głównego |

### 6.4 Kalibracja enkodera

Ekran procedury kalibracyjnej (szczegóły → [sekcja 9](#9-kalibracja-enkodera)):

- **Status:** POMIAR... (żółty, duża czcionka) lub GOTOWY
- **Licznik impulsów** (widoczny podczas pomiaru)
- **Impulsy/metr** (aktualna wartość)
- **Status kalibracji:** OK / Domyślny
- **Podpowiedzi:** START=rozpocznij/zakończ pomiar, STOP(1s)=powrót

### 6.5 Pomiar dystansu

Ręczny pomiar odległości (niezależny od malowania):

- **Status:** POMIAR... lub GOTOWY
- **Wynik:** Duża czcionka — w metrach (do 1000 m) lub kilometrach (powyżej)
- **Dodatkowa informacja:** Wartość w centymetrach

| Przycisk | Akcja |
|----------|-------|
| **START** | Rozpocznij / Wstrzymaj pomiar |
| **STOP (krótko)** | Resetuj licznik do 0 |
| **STOP (1 s)** | Powrót do menu serwisowego |

### 6.6 Raporty

Wyświetla informacje o raportach zapisanych na karcie SD:

- **Status karty SD:** OK (zielony) / BRAK (czerwony)
- **Liczba plików raportów**
- **Ostatni wpis:** Data, godzina, wzorzec, dystans, powierzchnia

Powrót: **STOP (1 s)**

### 6.7 Czyszczenie dysz

Tryb ręcznego testowania i czyszczenia pistoletów:

1. Wybierz wzorzec przyciskiem **SELEKTOR** (krótko = następny, długo = poprzedni)
2. **Trzymaj przycisk START** — pistolety przypisane do wybranego wzorca włączą się
3. **Puść START** — pistolety natychmiast się wyłączą

Na ekranie: Nagłówek "CZYSZCZENIE DYSZ", kod i nazwa wzorca, legenda kolorów, 6 prostokątów pistoletów.

> **Ważne:** W trybie czyszczenia dysz zabezpieczenie prędkości minimalnej jest **wyłączone** — pistolety działają nawet na postoju. Działają TYLKO gdy trzymasz przycisk START.

Powrót: **STOP (1 s)**

---

## 7. Funkcja "Start od przerwy"

### 7.1 Opis

Funkcja "Start od przerwy" pozwala rozpocząć malowanie nie od kreski, ale od przerwy we wzorcu. Jest to niezbędne przy kontynuacji istniejącego oznakowania drogowego — maszyna może dojechać do miejsca, gdzie linia przerywana powinna mieć przerwę, i rozpocząć pracę z właściwą fazą wzorca.

### 7.2 Jak działa

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

### 7.3 Aktywacja

| Sposób | Jak |
|--------|-----|
| **Na urządzeniu** | Naciśnij dedykowany przycisk **GAP** (GPIO 7) na ekranie HOME |
| **W panelu WWW** | Naciśnij żółty przycisk **START OD PRZERWY** |

Na ekranie malowania pojawi się znacznik **[GAP]** informujący o aktywnym trybie.

### 7.4 Kiedy używać

- Kontynuacja istniejącej linii przerywanej (np. po przerwie w pracy)
- Malowanie od punktu, gdzie powinna być przerwa a nie kreska
- Synchronizacja z istniejącym oznakowaniem na jezdni
- Rozpoczynanie pracy na środku istniejącego odcinka oznakowania

> **Uwaga:** Dla wzorców ciągłych (P-2a, P-2b, P-4, P-7b, P-7d) start od przerwy działa identycznie jak normalny start, ponieważ te wzorce nie posiadają przerw.

---

## 8. Panel WWW (zdalny dostęp)

### 8.1 Połączenie

1. Na telefonie lub komputerze wyszukaj sieć WiFi **TrassarV3**
2. Połącz się hasłem: **12345678**
3. Otwórz przeglądarkę i wejdź na: **http://192.168.4.1**

Panel automatycznie odświeża dane co 1 sekundę bez przeładowania strony.

### 8.2 Funkcje panelu WWW

Panel sterowania w przeglądarce oferuje pełną kontrolę nad maszyną:

| Sekcja | Opis |
|--------|------|
| **Status** | Stan maszyny z kolorowym pulsującym wskaźnikiem (Gotowy / Malowanie / Pauza / Zatrzymany) |
| **Informacje** | Wzorzec, nazwa, prędkość, dystans, powierzchnia, czas sesji, odwrócenie, kalibracja |
| **Sterowanie** | Przyciski START / PAUZA / STOP / START OD PRZERWY |
| **Wybór wzorca** | 15 przycisków pogrupowanych: P-1x, P-2x, P-3x, P-4/P-6, P-7x |
| **Odwracanie** | Przycisk "Odwróć" — aktywny tylko dla P-3a / P-3b |
| **Pistolety** | 6 kółek P1–P6 (zielone = ON, szare = OFF) |
| **Kalibracja** | Przycisk rozpoczęcia/zakończenia, licznik impulsów, impulsy/metr |
| **System** | Wersja firmware, wolna RAM, uptime, liczba klientów WiFi |

### 8.3 Zmiana wzorca przez panel WWW

W panelu WWW dostępne jest **15 przycisków wzorców** — kliknięcie zmienia wzorzec natychmiast. Aktywny wzorzec jest podświetlony na zielono. Jest to **jedyny sposób zmiany wzorca** — na fizycznym panelu sterowania (ekran HOME i malowania) przycisk SELEKTOR służy wyłącznie do odwracania P-3a/P-3b.

---

## 9. Kalibracja enkodera

### 9.1 Dlaczego kalibracja jest ważna

Enkoder obrotowy mierzy obroty koła pomiarowego, ale fabryczna wartość impulsów/metr (100.0) może nie odpowiadać rzeczywistemu obwodowi koła. Prawidłowa kalibracja zapewnia dokładne pomiary dystansu, prędkości i precyzyjne odwzorowanie długości kresek i przerw we wzorcach.

### 9.2 Procedura kalibracji

1. Wejdź w **Menu serwisowe** → **Kalibracja enkodera**
2. Odmierz na podłożu dokładnie **10 metrów** (np. taśmą mierniczą)
3. Ustaw maszynę na początku odcinka
4. Naciśnij **START** — na ekranie pojawi się "POMIAR..." i licznik impulsów
5. Przejedź maszyną **dokładnie 10 m** po prostej
6. Naciśnij **START** — system obliczy impulsy/metr i zapisze w pamięci NVS
7. Sprawdź wynik: "Imp/metr" i "Status: OK"

Wynik kalibracji jest zapisywany trwale w pamięci NVS (Non-Volatile Storage) i przetrwa restart urządzenia.

### 9.3 Anulowanie kalibracji

Naciśnij **STOP (1 s)** w trakcie pomiaru — kalibracja zostanie anulowana, poprzednia wartość pozostanie bez zmian.

---

## 10. Raporty na karcie SD

### 10.1 Format raportów

Raporty zapisywane są automatycznie po każdym zatrzymaniu malowania (STOP) na kartę SD w formacie CSV:

- **Lokalizacja:** `/reports/RRRRMMDD.csv` (np. `/reports/20250612.csv`)
- **Nagłówek:** `data,godzina,wzorzec,dystans_m,powierzchnia_m2`
- **Jeden wiersz** na każdą sesję malowania

**Przykład zawartości pliku `/reports/20250612.csv`:**
```csv
data,godzina,wzorzec,dystans_m,powierzchnia_m2
2025-06-12,08:30:15,P-1a,1250.5,150.06
2025-06-12,10:45:22,P-3a,875.3,210.07
2025-06-12,14:10:08,P-2b,430.0,103.20
```

### 10.2 Przeglądanie raportów

- **Na urządzeniu:** Menu serwisowe → Raporty — status SD, liczba plików, ostatni wpis
- **Na komputerze:** Wyjmij kartę SD i otwórz pliki CSV w dowolnym arkuszu kalkulacyjnym

### 10.3 Wymagania karty SD

- Format: **FAT32**
- Slot: Zintegrowany w module wyświetlacza ILI9341
- Współdzieli magistralę SPI z wyświetlaczem (osobne linie CS)

---

## 11. Zabezpieczenia

### 11.1 Minimalna prędkość malowania

System wymaga prędkości minimum **3 km/h** do włączenia pistoletów. Poniżej tej prędkości:
- Pistolety pozostają wyłączone (nawet w stanie "Malowanie")
- Na ekranie nadal widoczny jest status "Malowanie"
- Po przyspieszeniu powyżej 3 km/h pistolety włączają się automatycznie
- Pozycja w cyklu wzorca (kreska/przerwa) jest obliczana na bieżąco z dystansu

**Wyjątek:** Tryb czyszczenia dysz omija zabezpieczenie prędkości — pistolety działają na postoju.

### 11.2 Automatyczne wyłączanie pistoletów

Pistolety wyłączają się natychmiast przy:
- Zatrzymaniu malowania (STOP)
- Pauzie malowania (START podczas malowania)
- Spadku prędkości poniżej 3 km/h
- Wyjściu z trybu czyszczenia dysz
- Puszczeniu przycisku START w trybie czyszczenia dysz

### 11.3 Odszumianie enkodera

Podczas inicjalizacji systemu enkoder może rejestrować drobne drgania. Po zakończeniu inicjalizacji system automatycznie zeruje licznik dystansu (`resetDistance()`), eliminując szum nazbierany podczas startu.

---

## 12. Przykłady zastosowania

### Przykład 1: Malowanie linii przerywanej P-1a na nowej drodze

**Scenariusz:** Nowo wybudowana droga wymaga namalowania osi jezdni linią przerywaną długą (4 m kreska, 8 m przerwa, 12 cm szerokości). Maszyna jest przygotowana i ustawiona na początku odcinka.

**Kroki:**

1. **Przygotowanie:**
   - Włącz urządzenie — pojawi się ekran powitalny "TrassarV3 v2.3.0", a po chwili ekran główny
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

## 13. Rozwiązywanie problemów

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

---

*TrassarV3 — Komputer pokładowy malowarki pasów drogowych*
*Firmware v2.3.0 | ESP32-S3 N16R8 | 6 pistoletów, 15 wzorców*
