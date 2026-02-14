#include "secrets.h"
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_BME280.h>
#include <MQ135.h>
#include <WiFi.h>
#include <SPI.h>
#include <BlynkSimpleEsp32.h>
#include "settings_manager.h" // Nový include pro správu nastavení


// Messurement variables
float temp;
float lastTemp;
#define TEMP_THRESHOLD 0.5
float hum;
float lastHum;
#define HUM_THRESHOLD 1.0
float press;
float lastPress;
#define PRESS_THRESHOLD 1.0
float airQuality;
float lastAirQuality;
#define AIR_QUALITY_THRESHOLD 10.0

// settings for control from Blynk
float tempThreshold = 30.0; // Prahové hodnoty, které lze měnit přes Blynk a ukládat do settings.json
float humThreshold = 60.0;
float pressThreshold = 1013.0;
float airQualityThreshold = 150.0;
bool onOff = false;
bool alarmTriggered = false;
// LED status
bool ledOk = false;
bool ledError = false;
bool ledStatus = false;
#define LED_OK_PIN 35
#define LED_ERROR_PIN 36
#define LED_Status_PIN 37

// Helpers
#define CORRECT_TEMP (0.40)

// Init sensors
MQ135 gasSensor = MQ135(6); // GPIO 6
Adafruit_BME280 bme;  // I2C




// Moving average buffer for resistance/PPM
const int MA_SAMPLES = 10;
float resBuffer[MA_SAMPLES];
float ppmBuffer[MA_SAMPLES];
int maIndex = 0;
bool maFilled = false;

// Blynk časovač
BlynkTimer timer;

void printSensorData() {
  Serial.println("--- Sensor Readings ---");
  Serial.printf("Temperature : %.2f °C\n", temp);
  Serial.printf("Humidity    : %.2f %%\n", hum);
  Serial.printf("Pressure    : %.2f hPa\n", press);
  Serial.printf("Air Quality : %.2f ppm\n", airQuality);
  Serial.println("-----------------------");
}

void readSensors() {
  temp = bme.readTemperature() - CORRECT_TEMP;
  hum = bme.readHumidity();
  press = bme.readPressure() / 100.0F;
  airQuality = gasSensor.getCorrectedPPM(temp, hum);

  printSensorData();
}

void updateLastValues() {
  lastTemp = temp;
  lastHum = hum;
  lastPress = press;
  lastAirQuality = airQuality;
}



void updateLEDs() {
  digitalWrite(LED_OK_PIN, ledOk ? HIGH : LOW);
  digitalWrite(LED_ERROR_PIN, ledError ? HIGH : LOW);
  digitalWrite(LED_Status_PIN, ledStatus ? HIGH : LOW);
}

// Funkce pro odesílání dat do Blynk
void sendSensorDataBlynk() {
  Blynk.virtualWrite(V0, temp);         // Virtual pin V1: Teplota
  Blynk.virtualWrite(V1, hum);          // Virtual pin V2: Vlhkost
  Blynk.virtualWrite(V2, press);        // Virtual pin V3: Tlak
  Blynk.virtualWrite(V3, airQuality);   // Virtual pin V4: Kvalita vzduchu
}


// Funkce pro nastavení prahových hodnot z Blynk
BLYNK_WRITE(V5) {
  tempThreshold = param.asFloat();
  Serial.printf("Nastavená prahová teplota: %.2f °C\n", tempThreshold);
  saveSettings();
}

BLYNK_WRITE(V6) {
  humThreshold = param.asFloat();
  Serial.printf("Nastavená prahová vlhkost: %.2f %%\n", humThreshold);
  saveSettings();
}

BLYNK_WRITE(V7) {
  pressThreshold = param.asFloat();
  Serial.printf("Nastavený prahový tlak: %.2f hPa\n", pressThreshold);
  saveSettings();
}

BLYNK_WRITE(V8) {
  airQualityThreshold = param.asFloat();
  Serial.printf("Nastavená prahová kvalita vzduchu: %.2f ppm\n", airQualityThreshold);
  saveSettings();
}
BLYNK_WRITE(V9) {
  onOff = param.asInt() == 1;
  Serial.printf("Regulace je  %s\n", onOff ? "zapnutá" : "vypnutá");
  ledOk = onOff ? true : false;
  updateLEDs();
  saveSettings();
}

void sendSettingsToBlynk() {
  Blynk.virtualWrite(V5, tempThreshold);
  Blynk.virtualWrite(V6, humThreshold);
  Blynk.virtualWrite(V7, pressThreshold);
  Blynk.virtualWrite(V8, airQualityThreshold);
  Blynk.virtualWrite(V9, onOff);
}

void setup() {
  // Inits
  Serial.begin(115200);

  loadSettings(); // Načteme uložené nastavení z LittleFS
  pinMode(35, OUTPUT);     // Set LED pin as output
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(47, OUTPUT);      // Set EN pin for second stabilisator as output
  digitalWrite(47, HIGH);   // Turn on the second stabilisator
  delay(1000);
  Wire.begin(42, 2);        // Set dedicated I2C pins 42 - SDA, 2 - SCL for ESP32-S3-DEVKit
  analogSetAttenuation(ADC_11db);  // Full scale = 3.3V
  analogReadResolution(10);

  
  // Init LEDs
  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ERROR_PIN, OUTPUT);
  pinMode(LED_Status_PIN, OUTPUT);

  bme.begin(0x77);  
  // Calibrate MQ135 in clean air
  Serial.println("Heating MQ135 for 20 seconds...");
  for(int i = 10; i > 0; i--) {
    Serial.print("Heating... ");
    Serial.println(i);
    ledOk = !ledOk;
    ledError = !ledError;
    ledStatus = !ledStatus;     
    updateLEDs();
    delay(1000);
  }  
  Serial.println("Evrything is ready!");
  Serial.println("------------------------");
  ledOk = false;
  ledError = false;
  ledStatus = false;
  updateLEDs();

  // // Připojení k Wi-Fi
   WiFi.mode(WIFI_STA);
   WiFi.setMinSecurity(WIFI_AUTH_WEP); //WIFI_AUTH_WPA_PSK
   Serial.println("Připojování k Wi-Fi...");
   Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Připojeno k Wi-Fi!");

  // Odešleme aktuální nastavení do Blynku po připojení
  sendSettingsToBlynk();

  // Nastavení časovače pro odesílání dat každých 10 minut   
  timer.setInterval(600000L, sendSensorDataBlynk);
}



void checkThresholds() {
  ledOk = temp < tempThreshold && hum < humThreshold && press < pressThreshold && airQuality < airQualityThreshold;
  ledError = !ledOk;
  if (!ledOk && !alarmTriggered) {
    Serial.println("Alarm! Hodnota překročila nastavený práh!");
    alarmTriggered = true;
  } else if (ledOk && alarmTriggered) {
    Serial.println("Hodnota se vrátila pod práh, alarm zrušen.");
    alarmTriggered = false;
  }

  Blynk.virtualWrite(V10, ledOk ? "OK" : "ALARM"); // Virtual pin V10: Stav alarmu
}

void loop() {
  readSensors();
  ledStatus = !ledStatus;

  if (onOff) {
    checkThresholds();
    ledOk = true;
  } else {
    ledOk = false;
    Serial.println("Regulace je vypnutá, všechny LED jsou OFF");
  }

  updateLEDs();
  // // Zpracování Blynk
  Blynk.run();
  timer.run();
    delay(1000); 
}
