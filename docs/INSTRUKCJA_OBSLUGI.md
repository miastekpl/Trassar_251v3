# TrassarV3 - Instrukcja obsługi v2.1.0

## 1. Opis ogólny

TrassarV3 to komputer pokładowy malowarki pasów drogowych oparty na mikrokontrolerze ESP32-S3 N16R8. System steruje **6 pistoletami natryskowymi** (P1-P6) poprzez przekaźniki, obsługuje **15 wzorców malowania** zgodnych z polskimi normami oznakowania poziomego, mierzy dystans za pomocą enkodera obrotowego i oblicza powierzchnię malowaną w czasie rzeczywistym.

Urządzenie posiada:
- Kolorowy wyświetlacz TFT ILI9341 2.8" (240x320)
- Trzy przyciski funkcyjne BS-33B (START, STOP, SELEKTOR)
- Enkoder obrotowy (pomiar dystansu + nawigacja menu)
- Zegar RTC DS1307 z baterią podtrzymującą
- Czytnik kart SD do zapisu raportów
- Wbudowany serwer WWW dostępny przez WiFi (panel zdalnego sterowania)
- Zabezpieczenie prędkości minimalnej (3 km/h)

## 2. Panel sterowania

### 2.1 Przyciski funkcyjne

| Przycisk | Krótkie naciśnięcie | Długie naciśnięcie (1s) |
|----------|---------------------|-------------------------|
| **START** | Start malowania / Pauza / Wznowienie | - |
| **STOP** | Zatrzymanie malowania | Wejście w menu serwisowe / Powrót |
| **SELEKTOR** | Następna opcja / Następny wzorzec | Wejście w funkcję / Odwróć wzorzec |

### 2.2 Enkoder obrotowy

| Akcja | Funkcja |
|-------|---------|
| **Obrót w prawo (CW)** | Następny wzorzec / Przewijanie menu |
| **Obrót w lewo (CCW)** | Poprzedni wzorzec / Przewijanie menu |
| **Naciśnięcie (krótkie)** | **Start od przerwy** (na ekranie głównym) / Wejście w opcję (w menu) |

> **Uwaga:** Enkoder pełni podwójną rolę - mierzy dystans podczas jazdy oraz służy do nawigacji w menu i wyboru wzorców.

## 3. Pistolety natryskowe

### 3.1 Opis pistoletów

| Pistolet | Szerokość | Zastosowanie |
|----------|-----------|-------------|
| **P1** | 12 cm | Oś jezdni - lewy |
| **P2** | 12 cm | Oś jezdni - środek |
| **P3** | 12 cm | Oś jezdni - prawy |
| **P4** | 24 cm | Oś jezdni - szeroki |
| **P5** | 12 cm | Krawędź - wąska |
| **P6** | 24 cm | Krawędź - szeroka |

### 3.2 Przypisanie pistoletów do wzorców

- **P-1a, P-1b, P-1c, P-1d** → P2 (przerywane, 12cm)
- **P-1e** → P4 (przerywana szeroka, 24cm)
- **P-2a** → P2 (ciągła wąska, 12cm)
- **P-2b** → P4 (ciągła szeroka, 24cm)
- **P-3a, P-3b** → P1 (ciągła) + P3 (przerywana) - ODWRACALNE
- **P-4** → P1 + P3 (podwójna ciągła)
- **P-6** → P5 (ostrzegawcza)
- **P-7a, P-7b** → P6 (krawędziowe szerokie, 24cm)
- **P-7c, P-7d** → P5 (krawędziowe wąskie, 12cm)

### 3.3 Kolory wskaźników pistoletów

| Kolor | Znaczenie |
|-------|-----------|
| **Żółty** | Pistolet wybrany we wzorcu (gotowy) |
| **Zielony** | Pistolet aktualnie maluje |
| **Żółty migający** | Pauza (pistolet we wzorcu, ale wstrzymany) |
| **Szary** | Pistolet nieużywany w tym wzorcu |

## 4. Wzorce malowania

### 4.1 Tabela wzorców

| Kod | Nazwa | Typ | Kreska | Przerwa | Szer. |
|-----|-------|-----|--------|---------|-------|
| P-1a | Przerywana długa | DASHED | 6.0 m | 6.0 m | 12 cm |
| P-1b | Przerywana krótka | DASHED | 3.0 m | 3.0 m | 12 cm |
| P-1c | Wydzielająca | DASHED | 3.0 m | 1.5 m | 12 cm |
| P-1d | Prowadząca wąska | DASHED | 1.0 m | 1.0 m | 12 cm |
| P-1e | Prowadząca szeroka | DASHED | 1.0 m | 1.0 m | 24 cm |
| P-2a | Ciągła wąska | CONTINUOUS | - | - | 12 cm |
| P-2b | Ciągła szeroka | CONTINUOUS | - | - | 24 cm |
| P-3a | Przekraczalna długa | MIXED | P1: ciągła, P3: 6m/6m | - | 12 cm |
| P-3b | Przekraczalna krótka | MIXED | P1: ciągła, P3: 3m/3m | - | 12 cm |
| P-4 | Podwójna ciągła | CONTINUOUS | P1+P3: ciągła | - | 12 cm |
| P-6 | Ostrzegawcza | DASHED | 1.0 m | 1.0 m | 12 cm |
| P-7a | Krawędziowa przeryw. szer. | DASHED | 1.0 m | 2.0 m | 24 cm |
| P-7b | Krawędziowa ciągła szer. | CONTINUOUS | - | - | 24 cm |
| P-7c | Krawędziowa przeryw. wąska | DASHED | 1.0 m | 2.0 m | 12 cm |
| P-7d | Krawędziowa ciągła wąska | CONTINUOUS | - | - | 12 cm |

### 4.2 Odwracanie wzorców (P-3a, P-3b)

Wzorce P-3a i P-3b obsługują odwracanie - zamianę roli pistoletów P1 i P3:

| Tryb | P1 | P3 |
|------|----|----|
| **Normalny** | Ciągła | Przerywana |
| **Odwrócony** | Przerywana | Ciągła |

Odwracanie aktywuje się:
- **Na urządzeniu:** Selektor długi (1s) na ekranie głównym lub malowania
- **W panelu WWW:** Przycisk "Odwróć"

## 5. Ekrany interfejsu

### 5.1 Ekran główny (HOME)

Wyświetla się po uruchomieniu:
- Aktualny wzorzec (kod + nazwa)
- Znacznik [ODW] (jeśli wzorzec odwrócony)
- Prędkość [km/h]
- Dystans [m / km]
- Status kalibracji (OK / BRAK)
- Data i czas z modułu RTC

**Dostępne akcje:**

| Przycisk | Akcja |
|----------|-------|
| **START** | Rozpocznij malowanie (od początku wzorca) |
| **Przycisk enkodera** | **Start od przerwy** - rozpocznij od przerwy we wzorcu |
| **SELEKTOR / Enkoder CW/CCW** | Zmień wzorzec |
| **SELEKTOR (1s)** | Odwróć wzorzec (P-3a/P-3b) |
| **STOP (1s)** | Wejdź do menu serwisowego |

### 5.2 Ekran malowania (PAINTING)

Automatycznie po rozpoczęciu malowania:
- Status: MALOWANIE / PAUZA
- Aktualny wzorzec (z opcjonalnym [ODW] i [PRZERWA])
- Prędkość [km/h] i czas trwania sesji
- Dystans [m/km] i powierzchnia [m²]
- 6 wskaźników pistoletów (kółka kolorowe ON/OFF)

**Sterowanie:**

| Przycisk | Akcja |
|----------|-------|
| **START** | Pauza / Wznowienie |
| **STOP** | Zatrzymanie (powrót do HOME, zapis raportu) |
| **SELEKTOR / Enkoder** | Zmiana wzorca w trakcie malowania |
| **SELEKTOR (1s)** | Odwróć wzorzec |

> **Bezpieczeństwo:** Pistolety włączają się automatycznie dopiero po osiągnięciu prędkości **3 km/h**. Poniżej tej prędkości pistolety są wyłączone, nawet jeśli malowanie trwa.

### 5.3 Menu serwisowe

Dostęp: **STOP (1s)** na ekranie głównym.

4 pozycje do wyboru:

| # | Pozycja | Opis |
|---|---------|------|
| 1 | **Kalibracja enkodera** | Procedura kalibracyjna 10m |
| 2 | **Pomiar dystansu** | Ręczny pomiar odległości |
| 3 | **Raporty** | Przeglądanie raportów z karty SD |
| 4 | **Czyszczenie dysz** | Ręczne uruchamianie pistoletów |

**Nawigacja:**

| Przycisk | Akcja |
|----------|-------|
| **SELEKTOR (krótko) / Enkoder CW** | Następna pozycja |
| **Enkoder CCW** | Poprzednia pozycja |
| **SELEKTOR (1s) / Przycisk enkodera** | Wejdź w wybraną opcję |
| **STOP (1s)** | Powrót do ekranu głównego |

### 5.4 Kalibracja enkodera

Procedura kalibracji pomiarowej:

1. Naciśnij **START** aby rozpocząć pomiar
2. Przejedź maszyną dokładnie **10 metrów** po prostej
3. Naciśnij **START** aby zakończyć pomiar
4. System obliczy i zapisze impulsy/metr do pamięci NVS

Na ekranie wyświetlane:
- Status: POMIAR... / GOTOWY
- Licznik impulsów (podczas pomiaru)
- Aktualne impulsy/metr
- Status kalibracji: Skalibrowany / Domyślny

Powrót: **STOP (1s)**

### 5.5 Pomiar dystansu

Ręczny pomiar odległości (niezależny od malowania):

| Przycisk | Akcja |
|----------|-------|
| **START** | Rozpocznij / Wstrzymaj pomiar |
| **STOP (krótko)** | Resetuj licznik do 0 |
| **STOP (1s)** | Powrót do menu serwisowego |

Na ekranie: dystans w metrach (duża czcionka) + status (POMIAR / PAUZA).

### 5.6 Raporty

Wyświetla informacje o raportach zapisanych na karcie SD:
- Status karty SD (Gotowa / Brak karty)
- Liczba plików raportów
- Ostatni zapisany raport (data, wzorzec, dystans, powierzchnia)

Raporty zapisywane automatycznie po zakończeniu każdej sesji malowania.
Format: CSV w katalogu `/reports/RRRRMMDD.csv`.

Powrót: **STOP (1s)**

### 5.7 Czyszczenie dysz

Tryb ręcznego testowania i czyszczenia pistoletów:

1. Wybierz wzorzec enkoderem lub selektorem (określa które pistolety będą aktywne)
2. **Trzymaj przycisk START** - pistolety włączą się na czas trzymania
3. Puść START - pistolety natychmiast się wyłączą

Na ekranie: nazwa wzorca, 6 prostokątów pistoletów (żółty = w wzorcu, zielony = aktualnie strzela, szary = nieużywany).

> **Ważne:** W trybie czyszczenia dysz zabezpieczenie prędkości minimalnej jest **wyłączone** - pistolety działają na postoju.

Powrót: **STOP (1s)**

## 6. Funkcja "Start od przerwy"

### 6.1 Opis

Funkcja "Start od przerwy" pozwala rozpocząć malowanie nie od kreski, ale od przerwy we wzorcu. Jest to przydatne gdy maszyna musi dojechać do miejsca, gdzie linia przerywana powinna mieć przerwę (np. kontynuacja istniejącego oznakowania).

### 6.2 Jak działa

Normalny start (przycisk START):
```
Kreska → Przerwa → Kreska → Przerwa → ...
```

Start od przerwy (przycisk enkodera):
```
Przerwa → Kreska → Przerwa → Kreska → ...
```

System przesuwa punkt startowy wzorca o długość kreski, dzięki czemu cykl zaczyna się od przerwy.

### 6.3 Aktywacja

- **Na urządzeniu:** Naciśnij **przycisk enkodera** na ekranie głównym
- **W panelu WWW:** Przycisk **START OD PRZERWY** (żółty)

Na ekranie malowania pojawi się znacznik **[PRZERWA]** informujący, że użyto startu od przerwy.

### 6.4 Kiedy używać

- Kontynuacja istniejącej linii przerywanej (np. po przerwie w pracy)
- Malowanie od punktu, gdzie powinna być przerwa
- Synchronizacja z istniejącym oznakowaniem na jezdni

> **Uwaga:** Dla wzorców ciągłych (P-2a, P-2b, P-4, P-7b, P-7d) start od przerwy działa tak samo jak normalny start, ponieważ nie mają one przerw.

## 7. Panel WWW (zdalny dostęp)

### 7.1 Połączenie

1. Na telefonie/komputerze wyszukaj sieć WiFi **TrassarV3**
2. Połącz się hasłem: **12345678**
3. Otwórz przeglądarkę i wejdź na: **http://192.168.4.1**

### 7.2 Funkcje panelu WWW

- **Status** - stan maszyny z kolorowym wskaźnikiem (animowany puls)
- **Informacje** - wzorzec, prędkość, dystans, powierzchnia, czas, kalibracja
- **Sterowanie** - przyciski START / PAUZA / STOP / **START OD PRZERWY**
- **15 przycisków wzorców** - pogrupowane: P-1x, P-2x, P-3x, P-4/P-6, P-7x
- **Przycisk odwracania** - aktywny tylko dla P-3a/P-3b
- **Wskaźniki pistoletów** - 6 kółek P1-P6 (zielone = ON, szare = OFF)
- **Kalibracja** - przycisk rozpoczęcia/zakończenia, licznik impulsów
- **Info systemowe** - firmware, RAM, uptime, klienci WiFi

Panel automatycznie odświeża dane co 1 sekundę.

## 8. Zabezpieczenia

### 8.1 Minimalna prędkość malowania

System wymaga prędkości minimum **3 km/h** do włączenia pistoletów. Poniżej tej prędkości:
- Pistolety pozostają wyłączone (nawet w stanie MALOWANIE)
- Na ekranie nadal widoczny jest status malowania
- Po przyspieszeniu powyżej 3 km/h pistolety włączają się automatycznie

**Wyjątek:** Tryb czyszczenia dysz omija zabezpieczenie prędkości.

### 8.2 Automatyczne wyłączanie pistoletów

Pistolety wyłączają się automatycznie przy:
- Zatrzymaniu malowania (STOP)
- Pauzie malowania
- Spadku prędkości poniżej 3 km/h
- Wyjściu z trybu czyszczenia dysz

## 9. Przykłady zastosowania

### Przykład 1: Malowanie linii przerywanej P-1a na nowej drodze

**Scenariusz:** Malowanie osi jezdni na nowo wybudowanej drodze linią przerywaną długą (6m kreska, 6m przerwa, 12cm szerokości).

**Kroki:**

1. **Przygotowanie:**
   - Włącz urządzenie - pojawi się ekran główny
   - Obróć enkoder aby wybrać wzorzec **P-1a** (Przerywana długa)
   - Sprawdź status kalibracji (powinno być "OK")

2. **Kalibracja (jeśli pierwszy raz):**
   - Przytrzymaj STOP (1s) → menu serwisowe
   - Wybierz "Kalibracja enkodera"
   - Naciśnij START, przejedź dokładnie 10m, naciśnij START
   - STOP (1s) → powrót

3. **Malowanie:**
   - Na ekranie głównym naciśnij **START**
   - Ruszaj maszyną - po osiągnięciu 3 km/h pistolet P2 zacznie malować
   - Wzorzec: 6m farba → 6m przerwa → 6m farba → ...
   - Obserwuj ekran: prędkość, dystans, powierzchnię

4. **Zakończenie:**
   - Naciśnij **STOP** - malowanie się zakończy
   - Raport zostanie automatycznie zapisany na kartę SD
   - Na ekranie pojawi się podsumowanie sesji

**Wskaźniki na ekranie malowania:**
- P2: zielony (maluje) / szary (przerwa)
- P1, P3, P4, P5, P6: szare (nieużywane w P-1a)

---

### Przykład 2: Malowanie linii podwójnej P-3a z odwracaniem

**Scenariusz:** Malowanie linii przekraczalnej (ciągła + przerywana) na drodze dwukierunkowej. Kierunek malowania wymaga, aby linia ciągła była po prawej stronie.

**Kroki:**

1. **Wybór wzorca:**
   - Na ekranie głównym wybierz **P-3a** (Przekraczalna długa)
   - Domyślnie: P1 = ciągła (lewa), P3 = przerywana (prawa)

2. **Sprawdzenie orientacji:**
   - Jeśli linia ciągła powinna być po prawej stronie, odwróć wzorzec
   - Przytrzymaj **SELEKTOR (1s)** → pojawi się znacznik [ODW]
   - Teraz: P1 = przerywana, P3 = ciągła

3. **Malowanie:**
   - Naciśnij **START**
   - Pistolety P1 i P3 będą pracować jednocześnie
   - P1: przerywana (6m/6m), P3: ciągła
   - Obserwuj wskaźniki - oba pistolety powinny świecić na zielono

4. **Zmiana kierunku (na powrotnej drodze):**
   - Naciśnij **STOP** aby zakończyć
   - Na ekranie głównym przytrzymaj **SELEKTOR (1s)** → zdejmie [ODW]
   - Teraz P1 = ciągła, P3 = przerywana (odwrotna orientacja)
   - Naciśnij **START** dla nowego odcinka

**Wskaźniki:**
- P1: zielony (maluje ciągłą lub przerywaną)
- P3: zielony (maluje drugą linię)
- P2, P4, P5, P6: szare (nieużywane)

---

### Przykład 3: Kontynuacja malowania z użyciem "Start od przerwy"

**Scenariusz:** Na drodze istnieje już linia przerywana P-1b (3m/3m). Maszyna stoi w miejscu, gdzie kończy się kreska i zaczyna przerwa. Trzeba kontynuować malowanie od przerwy.

**Kroki:**

1. **Ustawienie wzorca:**
   - Wybierz **P-1b** (Przerywana krótka: 3m kreska, 3m przerwa)

2. **Start od przerwy:**
   - Naciśnij **przycisk enkodera** (nie START!)
   - Na ekranie malowania pojawi się znacznik **[PRZERWA]**
   - System przesunął punkt startowy o 3m (długość kreski)

3. **Malowanie:**
   - Ruszaj maszyną
   - Przez pierwsze 3m pistolet NIE maluje (przerwa)
   - Po 3m pistolet zaczyna malować kreskę (3m)
   - Dalej normalny cykl: 3m przerwa → 3m kreska → ...

4. **Alternatywa - panel WWW:**
   - Zamiast przycisku enkodera, na telefonie naciśnij żółty przycisk **START OD PRZERWY**
   - Efekt identyczny

**Porównanie:**
```
Normalny START:   ███░░░███░░░███░░░  (zaczyna od kreski)
START OD PRZERWY: ░░░███░░░███░░░███  (zaczyna od przerwy)
```
(███ = malowanie, ░░░ = przerwa)

---

### Przykład 4: Czyszczenie dysz i testowanie pistoletów

**Scenariusz:** Przed rozpoczęciem pracy trzeba sprawdzić czy wszystkie pistolety działają prawidłowo i oczyścić dysze po nocnym postoju.

**Kroki:**

1. **Wejście w tryb czyszczenia:**
   - Na ekranie głównym przytrzymaj **STOP (1s)** → menu serwisowe
   - Enkoderem lub selektorem przejdź do pozycji 4: **Czyszczenie dysz**
   - Naciśnij przycisk enkodera lub przytrzymaj SELEKTOR (1s)

2. **Wybór wzorca do testu:**
   - Domyślnie wybrany jest aktualny wzorzec
   - Obróć enkoder aby wybrać wzorzec, który chcesz przetestować
   - Np. **P-4** (podwójna ciągła) aktywuje P1 i P3 jednocześnie
   - Np. **P-1a** aktywuje tylko P2

3. **Testowanie pistoletów:**
   - **Trzymaj przycisk START** - pistolety przypisane do wzorca włączą się
   - Na ekranie prostokąty odpowiednich pistoletów zmienią kolor na zielony
   - **Puść START** - pistolety natychmiast się wyłączą
   - Powtórz dla różnych wzorców aby przetestować wszystkie pistolety

4. **Test wszystkich pistoletów po kolei:**
   - Wybierz P-1a → trzymaj START → sprawdź P2
   - Wybierz P-2b → trzymaj START → sprawdź P4
   - Wybierz P-4 → trzymaj START → sprawdź P1 i P3
   - Wybierz P-7a → trzymaj START → sprawdź P6
   - Wybierz P-7c → trzymaj START → sprawdź P5

5. **Powrót:**
   - Przytrzymaj **STOP (1s)** → powrót do menu serwisowego
   - Przytrzymaj **STOP (1s)** ponownie → powrót na ekran główny

> **Uwaga:** W trybie czyszczenia dysz zabezpieczenie prędkości minimalnej jest wyłączone - pistolety działają nawet na postoju. Pistolety działają TYLKO gdy trzymasz przycisk START.

## 10. Raporty na karcie SD

### 10.1 Format raportów

Raporty zapisywane są w formacie CSV na karcie SD (FAT32):
- Lokalizacja: `/reports/RRRRMMDD.csv` (np. `/reports/20250612.csv`)
- Nagłówek: `data,godzina,wzorzec,dystans_m,powierzchnia_m2`
- Jeden wiersz na każdą sesję malowania

Przykład zawartości pliku:
```
data,godzina,wzorzec,dystans_m,powierzchnia_m2
2025-06-12,08:30:15,P-1a,1250.5,150.06
2025-06-12,10:45:22,P-3a,875.3,210.07
2025-06-12,14:10:08,P-2b,430.0,103.20
```

### 10.2 Przeglądanie raportów

Menu serwisowe → Raporty wyświetla:
- Status karty SD
- Liczbę plików raportów
- Ostatni zapisany raport

Szczegółowe raporty dostępne po wyjęciu karty SD i otwarciu plików CSV na komputerze.

## 11. Wskaźniki statusu

| Status | Kolor na ekranie | Opis |
|--------|-------------------|------|
| Gotowy (IDLE) | Zielony | System bezczynny, gotowy do malowania |
| Malowanie | Zielony pulsujący | Aktywne malowanie |
| Pauza | Żółty | Malowanie wstrzymane |
| Zatrzymany | Czerwony | Malowanie przerwane |

## 12. Rozwiązywanie problemów

| Problem | Rozwiązanie |
|---------|-------------|
| Wyświetlacz nie świeci | Sprawdź pin podświetlenia GPIO 21 |
| Brak czasu/daty | Sprawdź DS1307 (SDA=17, SCL=18) i baterię CR2032 |
| Nie można połączyć WiFi | Upewnij się, że jesteś w zasięgu. SSID: TrassarV3, hasło: 12345678 |
| Enkoder nie reaguje | Sprawdź piny CLK=5, DT=6, SW=7 |
| Przyciski nie działają | Sprawdź podłączenie do GND i GPIO 38/39/40 |
| Restart w pętli (crash) | GPIO 33-37 zajęte przez PSRAM! Nie podłączać! |
| Pistolety nie włączają się | 1) Sprawdź prędkość >= 3 km/h. 2) Sprawdź przekaźniki na GPIO 41,42,1,2,3,4 |
| Pistolety nie włączają się na postoju | Normalnie - zabezpieczenie prędkości. Użyj trybu czyszczenia dysz |
| Złe odczyty dystansu | Wykonaj kalibrację enkodera (Menu → Kalibracja) |
| Wzorzec P-3a/P-3b maluje odwrotnie | Użyj funkcji odwracania (Selektor 1s lub przycisk w panelu WWW) |
| Karta SD nie działa | Sprawdź format FAT32, pin CS=GPIO 16, poprawne włożenie karty |
| Brak raportów na karcie | Sprawdź status SD w Menu → Raporty. Raporty zapisują się po STOP |
| "Start od przerwy" nie działa | Działa tylko dla wzorców przerywanych (P-1x, P-3x, P-6, P-7a, P-7c) |
