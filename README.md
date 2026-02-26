# JCU-iot

IoT monitorovací systém kotelny postavený na **ESP32-S3**, senzorech **BME280** a **MQ135** a cloudové platformě **Blynk**.

Projekt měří:
- teplotu,
- vlhkost,
- tlak,
- kvalitu vzduchu (ppm),

a podle nastavených prahů vyhodnocuje alarmový stav, ovládá LED signalizaci a odesílá data do Blynk aplikace.

## Funkce

- Pravidelné čtení dat ze senzorů (interval 1 s).
- Alarm při překročení prahových hodnot.
- LED signalizace stavu (OK / ERROR / STATUS).
- Odesílání dat do Blynk pouze při významné změně hodnot.
- Vynucená synchronizace dat každých 15 minut.
- Uložení nastavení prahů a zapnutí/vypnutí do `LittleFS` (`data/settings.json`).
- Změna prahů i zapnutí/vypnutí vzdáleně přes Blynk.

## Použitý hardware

- ESP32-S3-DevKitC-1
- BME280 (I2C)
- MQ135 (analog)
- 3x LED (OK / ERROR / STATUS)
- Breadboard + rezistory + propojovací vodiče

## Pinout v kódu

### Senzory
- BME280 (I2C):
	- SDA: GPIO `42`
	- SCL: GPIO `2`
- MQ135 (analog vstup): GPIO `6`

### LED
- `LED_OK_PIN` = GPIO `35`
- `LED_ERROR_PIN` = GPIO `36`
- `LED_Status_PIN` = GPIO `37`

## Blynk virtuální piny

Detailní mapování virtuálních pinů a Blynk eventů je v dokumentaci:
- `dokumentace/blynk_pins.md`

## Struktura projektu

```text
.
├─ data/
│  └─ settings.json
├─ src/
│  ├─ main.cpp
│  ├─ settings_manager.cpp
│  └─ settings_manager.h
├─ dokumentace/
│  ├─ main.tex
│  └─ ...
└─ platformio.ini
```

## Nastavení projektu

### 1) Požadavky
- VS Code
- Rozšíření PlatformIO IDE

### 2) Konfigurační soubor `secrets.h`
V repozitáři je přiložen anonymizovaný soubor `src/secrets.h`.
Před nahráním firmwaru v něm nahraď zástupné hodnoty za své skutečné údaje.

> Pokud používáš Blynk 2.0 šablony, hodnoty `BLYNK_TEMPLATE_ID` a `BLYNK_TEMPLATE_NAME` jsou povinné.

### 3) První nahrání
1. Otevři projekt v PlatformIO.
2. Zvol environment `esp32-s3-devkitc-1`.
3. Nahraj data do LittleFS (`Upload Filesystem Image`) pro inicializaci `data/settings.json`.
4. Nahraj firmware (`Upload`).
5. Otevři Serial Monitor (`115200`).
