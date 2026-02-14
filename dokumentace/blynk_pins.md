# Dokumentace Virtuálních Pinů pro Blynk

Tento dokument popisuje význam jednotlivých virtuálních pinů (Vx) používaných v projektu `iot1` pro snadnější nastavení mobilní aplikace Blynk.

---

## Data odesílaná z desky do aplikace (Výstupy)

Tyto piny slouží k zobrazování dat ze senzorů v aplikaci. Pro tyto piny nastavte v aplikaci frekvenci čtení na `PUSH`.

| Pin | Název              | Jednotka/Formát | Doporučený Widget v Blynku |
|:----|:-------------------|:----------------|:---------------------------|
| `V0`  | Teplota            | `°C` (float)    | `Gauge`, `Labeled Value`   |
| `V1`  | Vlhkost            | `%` (float)     | `Gauge`, `Labeled Value`   |
| `V2`  | Tlak               | `hPa` (float)   | `Gauge`, `Labeled Value`   |
| `V3`  | Kvalita vzduchu    | `ppm` (float)   | `Gauge`, `Labeled Value`   |

---

## Ovládací prvky z aplikace na desku (Vstupy)

Tyto piny slouží k ovládání zařízení a nastavování prahových hodnot z mobilní aplikace.

| Pin | Název                    | Jednotka/Formát | Doporučený Widget v Blynku |
|:----|:-------------------------|:----------------|:---------------------------|
| `V5`  | Prahová hodnota teploty  | `°C` (float)    | `Slider`, `Numeric Input`  |
| `V6`  | Prahová hodnota vlhkosti | `%` (float)     | `Slider`, `Numeric Input`  |
| `V7`  | Prahová hodnota tlaku    | `hPa` (float)   | `Slider`, `Numeric Input`  |
| `V8`  | Prahová hodnota kvality vzduchu | `ppm` (float)   | `Slider`, `Numeric Input`  |
| `V9`  | Zapnutí/Vypnutí regulace | `0` / `1` (int) | `Button`, `Switch`         |
