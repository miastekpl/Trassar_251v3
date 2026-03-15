# TrassarV3 — Propozycje rozbudowy funkcjonalności

## Stan obecny (v2.52.0)

System jest dojrzały i funkcjonalny. Poniżej propozycje ulepszeń pogrupowane w trzy kategorie priorytetowe.

---

## Priorytet 1 — Wysoki (bezpośrednia wartość operacyjna)

### 1.1 OTA — aktualizacja firmware przez WiFi

**Problem:** Aktualizacja firmware wymaga kabla USB i komputera z PlatformIO.

**Propozycja:**
- Endpoint `POST /api/firmware` przyjmujący plik `.bin`
- Przycisk "Aktualizuj firmware" w panelu WWW z upload pliku
- Wykorzystanie `ESP32 OTA` (biblioteka `Update.h`)
- Walidacja rozmiaru i CRC przed flashowaniem
- Automatyczny restart po pomyślnej aktualizacji

**Pracochłonność:** 2-3 dni

---

### 1.2 Bluetooth — połączenie z drukarką etykiet

**Problem:** Dokumentacja papierowa z raportami wymaga osobnego komputera.

**Propozycja:**
- BLE (Bluetooth Low Energy) serial — ESP32-S3 ma wbudowany BT
- Wysyłanie podsumowania sesji (wzorzec, dystans, powierzchnia) do drukarki termicznej (np. Cat Printer, POS printer)
- Automatyczny druk po STOP lub na żądanie z panelu WWW
- Obsługa protokołu ESC/POS (standardowy dla drukarek termicznych)

**Pracochłonność:** 3-5 dni

---

### 1.3 Dwukierunkowy enkoder — detekcja jazdy do tyłu

**Problem:** Jazda do tyłu nalicza dystans w przód, co zaburza wzorzec.

**Propozycja:**
- Odczyt pinu DT (GPIO 6) w ISR dla detekcji kierunku
- Pauza naliczania dystansu przy jeździe wstecz
- Alarm buzzerem przy cofaniu z włączonymi pistoletami
- Opcja: automatyczne wyłączenie pistoletów przy cofaniu

**Pracochłonność:** 1 dzień (ISR już odczytuje CLK, wystarczy dodać logikę kierunku)

---

### 1.4 Profil operatora — zapisywanie presetów

**Problem:** Różni operatorzy mają różne preferencje (tryb, progi, wzorzec domyślny).

**Propozycja:**
- 3-5 slotów profili operatora w NVS
- Każdy profil zawiera: tryb pracy, progi prędkości, domyślny wzorzec, tryb przełączania, auto-resume
- Wybór profilu z ekranu SETUP lub panelu WWW
- Nazwy profilów definiowane z panelu WWW

**Pracochłonność:** 2-3 dni

---

### 1.5 Eksport raportów z wieloma sesjami (zlecenie)

**Problem:** Raporty CSV zapisują pojedyncze sesje. Brak grupowania w zlecenia/kontrakty.

**Propozycja:**
- Koncept "zlecenia" — grupowanie sesji malowania pod wspólną etykietą
- Wybór/tworzenie zlecenia z panelu WWW przed rozpoczęciem pracy
- Raport zbiorczy per zlecenie: łączny dystans, powierzchnia, czas, wzorce użyte, trasa GPS
- Eksport zlecenia jako jeden plik PDF/HTML

**Pracochłonność:** 4-5 dni

---

## Priorytet 2 — Średni (usprawnienia UX i niezawodności)

### 2.1 Ekran dotykowy — nawigacja palcem

**Problem:** Nawigacja po menu wymaga fizycznych przycisków/joysticka.

**Propozycja:**
- Moduł ILI9341 ma wbudowany Touch (piny T_CS, T_CLK, T_DIN, T_DO) — już podłączony do SPI
- Biblioteka `TFT_eSPI` wspiera touch natywnie
- Touch do: wyboru wzorca na ekranie, przycisków START/STOP, nawigacji menu
- Nadal zachować przyciski fizyczne jako backup

**Pracochłonność:** 3-4 dni (kalibracja touch + obsługa zdarzeń per ekran)

---

### 2.2 Mapa trasy w panelu WWW (Leaflet.js)

**Problem:** Trasa GPS dostępna tylko jako plik do pobrania — brak podglądu na żywo.

**Propozycja:**
- Osadzenie mapy Leaflet.js w panelu WWW (OpenStreetMap tiles z cache)
- Rysowanie trasy na żywo z danych GPS (WebSocket)
- Kolorowanie trasy: zielony = malowanie, szary = pauza
- Eksport widoku mapy jako obraz PNG

**Ograniczenie:** Wymaga dostępu do Internetu po stronie przeglądarki (tiles OSM). Alternatywa: offline tiles na karcie SD.

**Pracochłonność:** 3-5 dni

---

### 2.3 Alarm braku farby — czujnik poziomu zbiornika

**Problem:** Predykcja zużycia farby jest szacunkowa — rzeczywisty poziom może się różnić.

**Propozycja:**
- Czujnik ultradźwiękowy HC-SR04 lub analogowy pływakowy w zbiorniku farby
- Odczyt poziomu farby co 5 s
- Alarm buzzerem przy niskim poziomie (<10%)
- Automatyczne wyłączenie pistoletów przy krytycznym poziomie (<5%)
- Kalibracja: pełny/pusty zbiornik z panelu WWW

**Pracochłonność:** 2-3 dni (+ montaż mechaniczny)

---

### 2.4 Tryb nocny — automatyczne przełączanie

**Problem:** Tryb nocny wymaga ręcznego przełączenia.

**Propozycja:**
- Fotorezystor (LDR) lub czujnik światła BH1750 (I2C) mierzący oświetlenie
- Automatyczne przełączanie trybu dziennego/nocnego przy progowym oświetleniu
- Histereza zapobiegająca ciągłemu przełączaniu
- Regulacja jasności podświetlenia TFT proporcjonalnie do oświetlenia

**Pracochłonność:** 1-2 dni

---

### 2.5 Serwer NTP — automatyczna synchronizacja czasu

**Problem:** DS1307 RTC dryfuje ~2 ppm (~1 min/rok). Wymaga ręcznej korekty.

**Propozycja:**
- Synchronizacja czasu RTC z GPS (NMEA zawiera dokładny czas UTC)
- Automatyczna korekcja przy starcie systemu (jeśli GPS ma fix)
- Opcja: NTP przez WiFi gdy ESP32 podłączony do routera (tryb STA+AP)

**Pracochłonność:** 1 dzień (GPS→RTC sync)

---

### 2.6 Wykrywanie końca farby per pistolet

**Problem:** System nie rozróżnia między końcem farby a awarią pistoletu.

**Propozycja:**
- Czujnik przepływu (Hall-effect flow meter) na linii farby do każdego pistoletu
- Pomiar rzeczywistego przepływu vs oczekiwanego
- Alarm "brak farby" zamiast ogólnej "anomalii pistoletu"
- Statystyka zużycia farby per pistolet (dokładniejsza niż predykcja z powierzchni)

**Pracochłonność:** 3-5 dni (+ 6 czujników przepływu, montaż hydrauliczny)

---

## Priorytet 3 — Niski (przyszłościowe i eksperymentalne)

### 3.1 Tryb STA — połączenie z Internetem

**Problem:** ESP32-S3 pracuje tylko jako AP — brak połączenia z Internetem.

**Propozycja:**
- Tryb STA+AP: ESP32 jednocześnie hostuje AP i łączy się z routerem
- Automatyczny upload raportów na serwer (FTP/HTTP POST/MQTT)
- Zdalne monitorowanie stanu maszyny przez Internet
- Powiadomienia push (np. Telegram bot) o anomaliach i statusie

**Pracochłonność:** 3-5 dni

---

### 3.2 MQTT — integracja z systemem zarządzania flotą

**Problem:** Firmy z wieloma malowarkami potrzebują centralnego monitoringu.

**Propozycja:**
- Klient MQTT (np. PubSubClient) na ESP32
- Publish statusu maszyny co 5 s na topic `trassar/{device_id}/status`
- Subscribe na topic `trassar/{device_id}/command` dla zdalnego sterowania
- Dashboard (Grafana, Node-RED) z mapą floty

**Pracochłonność:** 3-4 dni

---

### 3.3 Przyspieszomierz — kontrola jakości malowania

**Problem:** Drgania maszyny wpływają na jakość linii, ale nie są mierzone.

**Propozycja:**
- Czujnik IMU MPU-6050 (I2C, 0x68 — kolizja z DS1307, użyć 0x69 z AD0=HIGH)
- Pomiar wibracji i przechylenia maszyny
- Alarm przy nadmiernych drganiach (mogą powodować nierówne linie)
- Kompensacja nachylenia jezdni w obliczaniu prędkości

**Pracochłonność:** 3-4 dni

---

### 3.4 Zaawansowane wzorce — edytor graficzny

**Problem:** Wzorzec własny ograniczony do prostych cykli kreska/przerwa.

**Propozycja:**
- Edytor graficzny wzorców w panelu WWW (canvas)
- Wieloetapowe wzorce: sekwencja różnych cykli (np. 2× krótka kreska, 1× długa)
- Import/eksport wzorców jako JSON
- Biblioteka wzorców z różnych krajów (DIN, MUTCD, PN)

**Pracochłonność:** 5-7 dni

---

### 3.5 Kamera — dokumentacja fotograficzna

**Problem:** Raporty tekstowe nie zawierają wizualnego potwierdzenia jakości.

**Propozycja:**
- Moduł kamery OV2640 (SPI, wspierany przez ESP32-S3)
- Automatyczne zdjęcie co X metrów podczas malowania
- Zdjęcia zapisywane na kartę SD obok raportów
- Miniaturki w raporcie HTML

**Ograniczenie:** SPI współdzielone z TFT — wymaga multipleksowania lub osobnego portu SPI.

**Pracochłonność:** 5-7 dni

---

### 3.6 Wyświetlacz HUD — projekcja na szybę

**Problem:** Operator musi spoglądać na ekran TFT w dół.

**Propozycja:**
- Mały wyświetlacz OLED 0.96" (SSD1306, I2C 0x3C) montowany na poziomie wzroku
- Wyświetlanie: prędkość, status malowania, wzorzec
- Minimalny interfejs — tylko najważniejsze informacje
- Wspólna magistrala I2C (trzeci adres obok DS1307 0x68 i MCP23017 0x20)

**Pracochłonność:** 2-3 dni

---

### 3.7 Sterowanie wieloma maszynami (master/slave)

**Problem:** Malowanie wielu linii jednocześnie wymaga synchronizacji.

**Propozycja:**
- ESP-NOW (protokół peer-to-peer ESP32): master wysyła sygnały start/stop/pattern do slave'ów
- Synchronizacja pozycji w cyklu wzorca między maszynami
- Jeden operator steruje wieloma malowarkami jednocześnie

**Pracochłonność:** 5-10 dni

---

## Tabela podsumowująca

| # | Propozycja | Priorytet | Pracochłonność | Złożoność HW | Wartość |
|---|-----------|-----------|----------------|---------------|---------|
| 1.1 | OTA firmware update | Wysoki | 2-3 dni | Brak | Wysoka |
| 1.2 | Bluetooth drukarka | Wysoki | 3-5 dni | Drukarka BT | Wysoka |
| 1.3 | Detekcja jazdy wstecz | Wysoki | 1 dzień | Brak | Wysoka |
| 1.4 | Profile operatora | Wysoki | 2-3 dni | Brak | Średnia |
| 1.5 | Zlecenia/kontrakty | Wysoki | 4-5 dni | Brak | Wysoka |
| 2.1 | Ekran dotykowy | Średni | 3-4 dni | Brak (już podł.) | Średnia |
| 2.2 | Mapa Leaflet.js | Średni | 3-5 dni | Brak | Średnia |
| 2.3 | Czujnik poziomu farby | Średni | 2-3 dni | HC-SR04 | Wysoka |
| 2.4 | Auto tryb nocny | Średni | 1-2 dni | LDR/BH1750 | Niska |
| 2.5 | GPS→RTC sync | Średni | 1 dzień | Brak | Średnia |
| 2.6 | Czujniki przepływu | Średni | 3-5 dni | 6× flow meter | Wysoka |
| 3.1 | Tryb STA + Internet | Niski | 3-5 dni | Router WiFi | Średnia |
| 3.2 | MQTT flota | Niski | 3-4 dni | Serwer MQTT | Średnia |
| 3.3 | Przyspieszomierz | Niski | 3-4 dni | MPU-6050 | Niska |
| 3.4 | Edytor wzorców | Niski | 5-7 dni | Brak | Średnia |
| 3.5 | Kamera OV2640 | Niski | 5-7 dni | Kamera | Niska |
| 3.6 | HUD OLED | Niski | 2-3 dni | OLED SSD1306 | Średnia |
| 3.7 | Master/slave | Niski | 5-10 dni | 2+ ESP32 | Niska |

---

## Rekomendacja

**Szybkie wygrane (Quick Wins):**
1. **Detekcja jazdy wstecz** (1.3) — 1 dzień, brak dodatkowego HW, duża wartość bezpieczeństwa
2. **GPS→RTC sync** (2.5) — 1 dzień, automatyczna korekta czasu
3. **OTA update** (1.1) — 2-3 dni, eliminuje potrzebę kabla USB przy aktualizacjach

**Największa wartość biznesowa:**
1. **Zlecenia/kontrakty** (1.5) — grupowanie raportów dla zleceniodawców
2. **Czujnik poziomu farby** (2.3) — eliminacja ryzyka malowania bez farby
3. **Bluetooth drukarka** (1.2) — natychmiastowa dokumentacja papierowa

---

*TrassarV3 — Roadmap v2.52.0*
*Propozycje rozbudowy oparte na analizie kodu i architektury systemu*
