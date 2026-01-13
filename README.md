# 🌐 Language / Język

**[🇬🇧 English](#english)** | **[🇵🇱 Polski](#polish)**

---

<a name="english"></a>
## 🇬🇧 ENGLISH VERSION

The English description will be created at a later stage.

### 📖 Description

**Diesel Pilot** 

### 🛠️ Required Hardware

| Component | Model | Notes |
|-----------|-------|-------|
| Microcontroller | ESP32 |
| RF Transceiver | CC1101 | 433 MHz |

**CC1101 Wiring:**
```
ESP32    CC1101
-----    ------
GPIO4  - GDO2
GPIO18 - SCK
GPIO19 - MISO
GPIO23 - MOSI
GPIO5  - CSn
3.3V   - VCC
GND    - GND
```

⚠️ **IMPORTANT:** 
Use at your own risk!!!!

<a name="polish"></a>
## 🇵🇱 WERSJA POLSKA

⚠️ **WAŻNE:** 
To jest wersja 0 wygenerowana przez AI.
Nie mam jeszcze nawet hardwaru żeby to sprawdzić. 

### 📖 Opis

Jak pisze autor oryginalnego projektu: 
**Diesel Pilot** to pełnoprawny kontroler ESP32 dla chińskich ogrzewaczy diesla komunikujących się przez RF 433 MHz. Projekt umożliwia pełną kontrolę ogrzewacza przez przeglądarkę internetową, MQTT oraz integrację z Home Assistant.

Ja stwierdziłem że nie chcę by był to zewnętrznie zarządzane urządzenie z osobnym kompilatorem gdyż można to zrobić za pomocą ESP Home i ten projekt jest próbą adaptacji tego pomysłu.

### 🔧 Kompatybilność:
Nie będę wypowiadał się na temat czegoś czego nie sprawdzałem. 
Odsyłam do autora oryginalnego projektu: https://github.com/PPTG/DieselPilot?tab=readme-ov-file#-kompatybilno%C5%9B%C4%87

### ✨ Funkcje
- Web GUI - oryginalne Web GUI zostało skasowane (a w zasadzie nie podjąłem póby jego odtworzenia) bo uważam że jest niepotrzebne przy wykożystaniu ESP Home. ESP Home ma swoje Web GUI w którym powinno dać się zrobić podstawowe funkcje (doprecyzuję po próbie uruchomienia).  
- Wyświetlacz - nie planuję wyciągać urządzenia na pierwszy widok, tam mam oryginalny panel ogrzewacza, a wszelkie parametry są dostępne z poziomu HA nie widzę więc potrzeby zastosowania wyświetlacza ale w bardzo łatwy sposób można go dodać do EPS Home. Jeśli nie zapomnę to może dodam go w zakomentowanej formie do kodu na późniejszym etapie.
- WiFi - domyślnie ESP32 z ESP Home łączy się po WiFi i tak też tu to działa. W przypadku nie wykrycia sieci jest stawiany awaryny AP.
- MQTT - nie zostało zaimplementowane w tej adaptacji gdyż ESP Home łączy się po API i w ten sposób jest zapewniana pełna integracja z Home Assistantem.
- Parowanie - na chwilę obecną go nie ma bo mi go AI chyba nie dodało. Nie mam jeszcze sprzętu żeby to testować. Docelowo chiał bym dodać do kodu jakiś rodzaj parowania.

### ⚙️ Changes

Wydanie V0
- to wersja na czysto wygenerowana przez AI na podstawie plików oryginalnego projetu
- nie zostało to jeszcze nawet wrzucone do kompilatora żeby sprawdzić błędy a na ile znam generowane kody będzie tego sporo

Pliki:
- **DieselPilot.yaml** - Stwórz nowy projekt w ESP Home i zastąp jego zawartość zawartością tego plilku. Dostosuj do swoich potrzeb wszystkie wymagane elementy takie jak dane do sieci, adresy IP itp.
- **diesel_pilot_component.h** - Wrzuć ten plik obok YAML ESPHome.

### 🛠️ Wymagany sprzęt

| Komponent | Model | Uwagi |
|-----------|-------|-------|
| Mikrokontroler | ESP32 
| Transceiver RF | CC1101 | 433 MHz |

**Podłączenie CC1101:**
```
ESP32    CC1101
-----    ------
GPIO4  - GDO2
GPIO18 - SCK
GPIO19 - MISO
GPIO23 - MOSI
GPIO5  - CSn
3.3V   - VCC
GND    - GND
```

⚠️ **WAŻNE:** 
Używasz na własne ryzyko !!!!

Based on PPTG/DieselPilot: https://github.com/PPTG/DieselPilot
