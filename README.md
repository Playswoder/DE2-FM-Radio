# 🎶 FM Radio – AVR Project

Tento projekt implementuje jednoduché **FM rádio** na platformě AVR mikrokontrolérů.  
Vyvinuto jako součást laboratorních cvičení z předmětu *Digital Electronics*.

---

## 📌 Popis projektu
Cílem projektu je ukázat praktické použití mikrokontroléru AVR pro:
- příjem FM signálu pomocí externího modulu (např. TEA5767)
- manipulace rádia skrze I2C sběrnici
- zobrazení informací (frekvence, hlasitost, RDS) na LCD displeji
- ovládání za pomocí tlačítek
- přepínáná mezi frekvencemi static
- (možná implementace přesného ladění frekvence)

Projekt kombinuje znalosti z oblasti:
- práce s periferiemi (I2C, UART, GPIO,...)
- programování v jazyce C pro AVR
- návrhu jednoduchého uživatelského rozhraní

---

## ⚙️ Hardware
- **Mikrokontrolér:** ATmega16/ATmega328 (lze upravit dle potřeby)
- **FM tuner modul:** TEA5767 (I²C)
- **LCD displej:** 2×16 znaků (HD44780 kompatibilní)
- **Ovládací prvky:** tlačítka pro ladění a hlasitost
- **Napájení:** 5 V

---

## 🛠️ Software
- Jazyk: **C**
- Kompilátor: **AVR-GCC**
- Nahrávání: **AVRDUDE**
- Struktura projektu:
  - `main.c` – hlavní program
  - `radio.c/h` – ovládání FM modulu
  - `lcd.c/h` – ovládání LCD displeje
  - `i2c.c/h` – implementace I²C komunikace
  - `uart.c/h` – ladicí výstup přes sériovou linku
  - `twi.c/h`
  - `oled.c/h`
  - `oled_rds.c/h`
  - `freq_selector.c/h`
  - `timer.c/h`

---

## 🚀 Spuštění projektu
1. Naklonujte repozitář:
   ```bash
   git clone https://github.com/Playswoder/de2-fm-radio.git
