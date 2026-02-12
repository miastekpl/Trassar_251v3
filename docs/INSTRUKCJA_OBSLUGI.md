# TrassarV3 - Instrukcja obsługi v2.0.0

## 1. Opis ogólny

TrassarV3 to komputer pokładowy malowarki pasów drogowych oparty na mikrokontrolerze ESP32-S3. System steruje **6 pistoletami natryskowanymi** (P1-P6) poprzez przekaźniki, obsługuje **15 wzorców malowania** zgodnych z polskimi normami, mierzy dystans za pomocą enkodera i oblicza powierzchnię malowaną.

Urządzenie posiada kolorowy wyświetlacz TFT 2.8", trzy przyciski funkcyjne, enkoder obrotowy oraz wbudowany serwer WWW dostępny przez WiFi.

## 2. Panel sterowania

### 2.1 Przyciski funkcyjne

| Przycisk | Krótkie naciśnięcie | Długie naciśnięcie (1s) |
|----------|---------------------|-------------------------|
| **START/PAUZA** | Start malowania / Pauza / Wznowienie | - |
| **STOP** | Zatrzymanie malowania | Wejście w menu / Powrót |
| **SELEKTOR** | Następna opcja / Następny wzorzec | Wejście w funkcję / Odwróć wzorzec |

### 2.2 Enkoder obrotowy

| Akcja | Funkcja |
|-------|---------|
| **Obrót w prawo (CW)** | Następny wzorzec / Przewijanie menu / Zwiększenie wartości |
| **Obrót w lewo (CCW)** | Poprzedni wzorzec / Przewijanie menu / Zmniejszenie wartości |
| **Naciśnięcie** | Potwierdzenie wyboru (alternatywa dla Selektor długi) |

> **Uwaga:** Enkoder pełni podwójną rolę - mierzy dystans podczas malowania oraz służy do nawigacji w menu.

## 3. Pistolety natryskowe

### 3.1 Opis pistoletów

| Pistolet | Szerokość | Zastosowanie | Pin GPIO |
|----------|-----------|-------------|----------|
| **P1** | 12 cm | Oś jezdni - lewy | 41 |
| **P2** | 12 cm | Oś jezdni - środek | 42 |
| **P3** | 12 cm | Oś jezdni - prawy | 1 |
| **P4** | 24 cm | Oś jezdni - szeroki | 2 |
| **P5** | 12 cm | Krawędź - wąska | 3 |
| **P6** | 24 cm | Krawędź - szeroka | 4 |

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

Wyświetla po uruchomieniu:
- Aktualny wzorzec (kod + nazwa)
- Znacznik [ODWRÓCONY] (jeśli aktywny)
- Prędkość [km/h]
- Dystans [m / km]
- Status kalibracji (OK / BRAK)
- Data i czas

**Dostępne akcje:**
- **START** - rozpocznij malowanie
- **SELEKTOR / Enkoder** - zmień wzorzec
- **SELEKTOR (1s)** - odwróć wzorzec (P-3a/P-3b)
- **STOP (1s)** - wejdź do menu

### 5.2 Ekran malowania (PAINTING)

Automatycznie po rozpoczęciu malowania:
- Status: MALOWANIE / PAUZA
- Aktualny wzorzec (z opcjonalnym [ODW])
- Prędkość [km/h] i czas trwania
- Dystans [m/km] i powierzchnia [m²]
- Wskaźniki 6 pistoletów (kółka ON/OFF)

**Sterowanie:**
- **START** - pauza / wznowienie
- **STOP** - zatrzymanie (powrót do HOME)
- **SELEKTOR / Enkoder** - zmiana wzorca w trakcie malowania
- **SELEKTOR (1s)** - odwróć wzorzec

### 5.3 Menu główne

6 opcji do wyboru:
1. **Wybór wzorca** - lista 15 wzorców
2. **Kalibracja enkodera** - procedura 10m
3. **Statystyki** - sesja + łączne
4. **Czas i data** - edycja zegara RTC
5. **Informacje WiFi** - dane sieci
6. **Info systemowe** - firmware, RAM, uptime

**Nawigacja:**
- **SELEKTOR (krótko) / Enkoder** - przesuwanie
- **SELEKTOR (1s) / Przycisk enkodera** - wejście
- **STOP (1s)** - powrót do ekranu głównego

### 5.4 Wybór wzorca

Lista 15 wzorców z paskiem przewijania. 8 pozycji widocznych jednocześnie.

- **Enkoder** - przewijanie listy
- **SELEKTOR (1s) / Przycisk enkodera** - wybór wzorca
- **STOP (1s)** - powrót do menu

### 5.5 Kalibracja enkodera

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

- **STOP (1s)** - anuluj i powrót do menu

### 5.6 Statystyki

Dwa bloki informacji:

**Bieżąca sesja:**
- Dystans [m / km]
- Powierzchnia [m²]
- Czas pracy

**Łączne (całkowite):**
- Dystans [m / km]
- Powierzchnia [m²]
- Czas pracy

Łączne statystyki zapisywane do NVS i utrzymywane po restarcie.

- **STOP (1s)** - powrót do menu

### 5.7 Czas i data

Edycja 6 pól: Godzina, Minuta, Sekunda, Dzień, Miesiąc, Rok.

- **SELEKTOR** - przeskocz do następnego pola
- **Enkoder** - zmiana wartości
- **STOP (1s)** - powrót do menu

### 5.8 Informacje WiFi

Wyświetla dane sieci WiFi:
- Tryb: Access Point
- SSID i hasło
- Adres IP
- Liczba połączonych klientów

### 5.9 Informacje systemowe

- Wersja firmware
- Data kompilacji
- Wolna pamięć RAM
- Uptime
- Platforma: ESP32-S3 N16R8
- Wyświetlacz: ILI9341 240x320

## 6. Panel WWW (zdalny dostęp)

### 6.1 Połączenie

1. Na telefonie/komputerze wyszukaj sieć WiFi **TrassarV3**
2. Połącz się hasłem: **12345678**
3. Otwórz przeglądarkę i wejdź na: **http://192.168.4.1**

### 6.2 Funkcje panelu WWW

- **Status** - stan maszyny z kolorowym wskaźnikiem (animowany)
- **Informacje** - wzorzec, prędkość, dystans, powierzchnia, czas, kalibracja
- **Sterowanie** - przyciski START / PAUZA / STOP
- **15 przycisków wzorców** - pogrupowane: P-1x, P-2x, P-3x, P-4/P-6, P-7x
- **Przycisk odwracania** - aktywny tylko dla P-3a/P-3b
- **Wskaźniki pistoletów** - 6 kółek P1-P6 (zielone = ON, szare = OFF)
- **Kalibracja** - przycisk rozpoczęcia/zakończenia, licznik impulsów
- **Info systemowe** - firmware, RAM, uptime, klienci WiFi

Panel automatycznie odświeża dane co 1 sekundę.

## 7. Procedura malowania

1. **Przed startem:**
   - Wykonaj kalibrację enkodera (Menu → Kalibracja → START → 10m → START)
   - Wybierz wzorzec (na ekranie głównym enkoderem lub w Menu → Wybór wzorca)
   - Dla P-3a/P-3b ustaw kierunek (Selektor 1s = odwróć)

2. **Malowanie:**
   - Na ekranie głównym naciśnij **START**
   - System przejdzie do ekranu malowania
   - Pistolety włączają się automatycznie na podstawie wzorca i dystansu
   - W trakcie malowania możesz:
     - **START** - pauza (pistolety się wyłączą)
     - **START** (ponownie) - wznowienie
     - **Enkoder** - zmiana wzorca on-the-fly
     - **Selektor (1s)** - odwrócenie (P-3a/P-3b)
     - **STOP** - zakończenie malowania

3. **Po zakończeniu:**
   - System wróci do ekranu głównego
   - Statystyki sesji są dostępne w Menu → Statystyki
   - Łączne statystyki zapisują się automatycznie do pamięci trwałej

## 8. Wskaźniki statusu

| Kolor | Status | Opis |
|-------|--------|------|
| Zielony | Gotowy / Malowanie | System bezczynny lub maluje |
| Żółty | Pauza | Malowanie wstrzymane |
| Czerwony | Zatrzymany | Malowanie przerwane |

## 9. Rozwiązywanie problemów

| Problem | Rozwiązanie |
|---------|-------------|
| Wyświetlacz nie świeci | Sprawdź pin podświetlenia GPIO 21 |
| Brak czasu/daty | Sprawdź DS1307 (SDA=17, SCL=18) i baterię CR2032 |
| Nie można połączyć WiFi | Upewnij się, że jesteś w zasięgu. SSID: TrassarV3, hasło: 12345678 |
| Enkoder nie reaguje | Sprawdź piny CLK=5, DT=6, SW=7 |
| Przyciski nie działają | Sprawdź podłączenie do GND i GPIO 38/39/40 |
| Restart w pętli (crash) | GPIO 33-37 zajęte przez PSRAM! Nie podłączać! |
| Pistolety nie włączają się | Sprawdź przekaźniki na GPIO 41,42,1,2,3,4. Przekaźniki aktywne HIGH |
| Złe odczyty dystansu | Wykonaj kalibrację enkodera (Menu → Kalibracja) |
| Wzorzec P-3a/P-3b maluje odwrotnie | Użyj funkcji odwracania (Selektor 1s lub przycisk w panelu WWW) |
