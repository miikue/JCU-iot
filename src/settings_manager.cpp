#include "settings_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Cesta k souboru s nastavením
const char* SETTINGS_FILE = "/settings.json";

// Globální proměnné jsou definovány v main.cpp a deklarovány jako extern v settings_manager.h
// Zde se k nim přistupuje přes deklaraci v hlavičkovém souboru.


void loadSettings() {
    Serial.println("Inicializuji LittleFS...");
    if (!LittleFS.begin()) {
        Serial.println("Chyba při inicializaci LittleFS.");
        return;
    }
    Serial.println("LittleFS inicializováno.");

    if (!LittleFS.exists(SETTINGS_FILE)) {
        Serial.println("Soubor settings.json nenalezen. Používám výchozí nastavení.");
        saveSettings(); // Uloží výchozí nastavení do souboru
        return;
    }

    File settingsFile = LittleFS.open(SETTINGS_FILE, "r");
    if (!settingsFile) {
        Serial.println("Chyba při otevírání settings.json pro čtení.");
        return;
    }

    StaticJsonDocument<512> doc; // Kapacita pro JSON dokument
    DeserializationError error = deserializeJson(doc, settingsFile);
    settingsFile.close();

    if (error) {
        Serial.print(F("Chyba při parsování settings.json: "));
        Serial.println(error.f_str());
        Serial.println("Používám výchozí nastavení.");
        return;
    }

    tempThreshold = doc["tempThreshold"] | 30.0;
    humThreshold = doc["humThreshold"] | 60.0;
    pressThreshold = doc["pressThreshold"] | 1013.0;
    airQualityThreshold = doc["airQualityThreshold"] | 150.0;
    onOff = doc["onOff"] | false;

    Serial.println("Nastavení úspěšně načteno:");
    Serial.printf("  Teplota: %.2f °C\n", tempThreshold);
    Serial.printf("  Vlhkost: %.2f %%\n", humThreshold);
    Serial.printf("  Tlak: %.2f hPa\n", pressThreshold);
    Serial.printf("  Kvalita vzduchu: %.2f ppm\n", airQualityThreshold);
    Serial.printf("  Regulace: %s\n", onOff ? "Zapnuta" : "Vypnuta");
}

void saveSettings() {
    if (!LittleFS.begin()) { // Zajištění inicializace, pokud nebyla dříve
        Serial.println("Chyba při inicializaci LittleFS pro ukládání.");
        return;
    }

    File settingsFile = LittleFS.open(SETTINGS_FILE, "w");
    if (!settingsFile) {
        Serial.println("Chyba při otevírání settings.json pro zápis.");
        return;
    }

    StaticJsonDocument<512> doc;
    doc["tempThreshold"] = tempThreshold;
    doc["humThreshold"] = humThreshold;
    doc["pressThreshold"] = pressThreshold;
    doc["airQualityThreshold"] = airQualityThreshold;
    doc["onOff"] = onOff;

    if (serializeJson(doc, settingsFile) == 0) {
        Serial.println(F("Chyba při zápisu do settings.json."));
    } else {
        Serial.println("Nastavení úspěšně uloženo.");
    }
    settingsFile.close();
}
