#include "settings_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Cesta k souboru s nastavenim
const char* SETTINGS_FILE = "/settings.json";

void loadSettings() {
    Serial.println("Inicializuji LittleFS...");
    if (!LittleFS.begin()) {
        Serial.println("Chyba pri inicializaci LittleFS.");
        return;
    }
    Serial.println("LittleFS inicializovano.");

    if (!LittleFS.exists(SETTINGS_FILE)) {
        Serial.println("Soubor settings.json nenalezen. Pouzivam vychozi nastaveni.");
        saveSettings(); // Ulozi vychozi nastaveni do souboru
        return;
    }

    File settingsFile = LittleFS.open(SETTINGS_FILE, "r");
    if (!settingsFile) {
        Serial.println("Chyba pri otevirani settings.json pro cteni.");
        return;
    }

    StaticJsonDocument<512> doc; // Kapacita pro JSON dokument
    DeserializationError error = deserializeJson(doc, settingsFile);
    settingsFile.close();

    if (error) {
        Serial.print(F("Chyba pri parsovani settings.json: "));
        Serial.println(error.f_str());
        Serial.println("Pouzivam vychozi nastaveni.");
        return;
    }

    tempThreshold = doc["tempThreshold"] | 30.0;
    humThreshold = doc["humThreshold"] | 60.0;
    pressThreshold = doc["pressThreshold"] | 1013.0;
    airQualityThreshold = doc["airQualityThreshold"] | 150.0;
    onOff = doc["onOff"] | false;

    Serial.println("Nastaveni uspesne nacteno:");
    Serial.printf("  Teplota: %.2f C\n", tempThreshold);
    Serial.printf("  Vlhkost: %.2f %%\n", humThreshold);
    Serial.printf("  Tlak: %.2f hPa\n", pressThreshold);
    Serial.printf("  Kvalita vzduchu: %.2f ppm\n", airQualityThreshold);
    Serial.printf("  Regulace: %s\n", onOff ? "Zapnuta" : "Vypnuta");
}

void saveSettings() {
    if (!LittleFS.begin()) { // Zajisteni inicializace, pokud nebyla drive
        Serial.println("Chyba pri inicializaci LittleFS pro ukladani.");
        return;
    }

    File settingsFile = LittleFS.open(SETTINGS_FILE, "w");
    if (!settingsFile) {
        Serial.println("Chyba pri otevirani settings.json pro zapis.");
        return;
    }

    StaticJsonDocument<512> doc;
    doc["tempThreshold"] = tempThreshold;
    doc["humThreshold"] = humThreshold;
    doc["pressThreshold"] = pressThreshold;
    doc["airQualityThreshold"] = airQualityThreshold;
    doc["onOff"] = onOff;

    if (serializeJson(doc, settingsFile) == 0) {
        Serial.println(F("Chyba pri zapisu do settings.json."));
    } else {
        Serial.println("Nastaveni uspesne ulozeno.");
    }
    settingsFile.close();
}