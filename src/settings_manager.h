#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h> // Pro práci s JSON
#include <LittleFS.h>    // Pro práci se souborovým systémem ESP32-S3

// Externí deklarace globálních proměnných, které budou spravovány
// Důležité: Tyto proměnné musí být definovány v main.cpp
extern float tempThreshold;
extern float humThreshold;
extern float pressThreshold;
extern float airQualityThreshold;
extern bool onOff;

/**
 * @brief Inicializuje souborový systém (LittleFS) a načte nastavení ze souboru.
 * Pokud soubor neexistuje nebo je poškozen, použije výchozí hodnoty a uloží je.
 */
void loadSettings();

/**
 * @brief Uloží aktuální hodnoty nastavení do souboru settings.json.
 */
void saveSettings();

#endif // SETTINGS_MANAGER_H