#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

extern float tempThreshold;
extern float humThreshold;
extern float pressThreshold;
extern float airQualityThreshold;
extern bool onOff;

void loadSettings();
void saveSettings();

#endif