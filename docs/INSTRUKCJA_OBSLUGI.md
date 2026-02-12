# TrassarV3 - Instrukcja obsługi

## 1. Opis ogólny

TrassarV3 to sterownik automatycznej maszyny malarskiej oparty na mikrokontrolerze ESP32-S3. Urządzenie posiada kolorowy wyświetlacz TFT 2.8", trzy przyciski funkcyjne, enkoder obrotowy oraz wbudowany serwer WWW dostępny przez WiFi.

## 2. Panel sterowania

### 2.1 Przyciski funkcyjne

| Przycisk | Krótkie naciśnięcie | Długie naciśnięcie (1s) |
|----------|---------------------|-------------------------|
| **START/PAUZA** | Start malowania / Pauza malowania | - |
| **STOP** | Zatrzymanie malowania | Wejście w menu / Powrót |
| **SELEKTOR** | Przeskok do następnej opcji | Wejście w podświetloną funkcję |

### 2.2 Enkoder obrotowy

| Akcja | Funkcja |
|-------|---------|
| **Obrót w prawo** | Przewijanie menu w dół / Zwiększenie wartości |
| **Obrót w lewo** | Przewijanie menu w górę / Zmniejszenie wartości |
| **Naciśnięcie** | Potwierdzenie wyboru (alternatywa dla Selektor długi) |

## 3. Ekrany

### 3.1 Ekran główny

Po uruchomieniu wyświetla się ekran główny z informacjami:
- Aktualny czas (duży format)
- Data
- Status maszyny (Gotowy / Malowanie / Pauza / Zatrzymany)
- Aktualna prędkość i liczba przejść

**Dostępne akcje:**
- **START** - rozpocznij malowanie
- **STOP (1s)** - wejdź do menu

### 3.2 Menu główne

Cztery opcje do wyboru:
1. **Ustawienia malowania** - prędkość i liczba przejść
2. **Czas i data** - edycja zegara RTC
3. **Informacje WiFi** - dane sieci i adres panelu
4. **Informacje systemowe** - wersja firmware, pamięć, uptime

**Nawigacja:**
- **SELEKTOR (krótko)** lub **Enkoder** - przesuwanie między opcjami
- **SELEKTOR (1s)** lub **Przycisk enkodera** - wejście w opcję
- **STOP (1s)** - powrót do ekranu głównego

### 3.3 Ustawienia malowania

Dwa parametry do regulacji:
- **Prędkość** - 0-100% (krok 5%)
- **Liczba przejść** - 1-99

**Sterowanie:**
- **SELEKTOR (krótko)** - przeskocz między polami
- **Enkoder** - zmiana wartości wybranego pola
- **STOP (1s)** - powrót do menu

### 3.4 Czas i data

Edycja poszczególnych składników czasu:
- Godzina, Minuta, Sekunda
- Dzień, Miesiąc, Rok

**Sterowanie:**
- **SELEKTOR (krótko)** - przeskocz do następnego pola
- **Enkoder** - zmiana wartości
- **STOP (1s)** - powrót do menu

### 3.5 Ekran malowania

Wyświetla się automatycznie po rozpoczęciu malowania:
- Status (Malowanie / Pauza)
- Czas trwania
- Prędkość z paskiem postępu
- Aktualny przejazd / łączna liczba przejść

**Sterowanie:**
- **START** - pauza (podczas malowania) / wznowienie (podczas pauzy)
- **STOP** - zatrzymanie malowania
- **Enkoder** - regulacja prędkości w trakcie pracy

## 4. Panel WWW (zdalny dostęp)

### 4.1 Połączenie

1. Na telefonie/komputerze wyszukaj sieć WiFi **TrassarV3**
2. Połącz się hasłem: **12345678**
3. Otwórz przeglądarkę i wejdź na: **http://192.168.4.1**

### 4.2 Funkcje panelu WWW

- Podgląd statusu maszyny w czasie rzeczywistym
- Przyciski START / PAUZA / STOP
- Suwak regulacji prędkości
- Zmiana liczby przejść
- Informacje systemowe (firmware, RAM, uptime)

Panel automatycznie odświeża dane co 1 sekundę.

## 5. Procedura malowania

1. Ustaw parametry w **Ustawienia malowania** (prędkość, przejazdy)
2. Na ekranie głównym naciśnij **START**
3. Maszyna rozpocznie malowanie - ekran przejdzie do widoku postępu
4. W trakcie malowania możesz:
   - Nacisnąć **START** aby zapauzować (pistolety się zatrzymają)
   - Ponownie **START** aby wznowić
   - Kręcić **enkoderem** aby zmieniać prędkość
   - Nacisnąć **STOP** aby całkowicie zatrzymać
5. Po zakończeniu system wróci do ekranu głównego

## 6. Wskaźniki statusu

| Kolor | Status | Opis |
|-------|--------|------|
| Szary | Gotowy | System bezczynny, gotowy do pracy |
| Zielony | Malowanie | Proces malowania w toku |
| Żółty | Pauza | Malowanie wstrzymane |
| Czerwony | Zatrzymany | Proces przerwany |

## 7. Rozwiązywanie problemów

| Problem | Rozwiązanie |
|---------|-------------|
| Wyświetlacz nie świeci | Sprawdź podłączenie pinu podświetlenia (GPIO 21) |
| Brak czasu/daty | Sprawdź podłączenie modułu DS1307 (SDA=17, SCL=18) |
| Nie można połączyć WiFi | Upewnij się, że jesteś w zasięgu. SSID: TrassarV3, hasło: 12345678 |
| Enkoder nie reaguje | Sprawdź piny CLK=5, DT=6, SW=7 |
| Przyciski nie działają | Sprawdź podłączenie do GND i odpowiednich GPIO |
