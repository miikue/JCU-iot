#include "secrets.h"
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_BME280.h>
#include <MQ135.h>
#include <WiFi.h>
#include <SPI.h>
#include <BlynkSimpleEsp32.h>
#include "settings_manager.h"

// -----------------------------
// Globální proměnné a nastavení
// -----------------------------



// Měření
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

// Prahové hodnoty pro regulaci (lze měnit přes Blynk)
float tempThreshold = 30.0;
float humThreshold = 60.0;
float pressThreshold = 1013.0;
float airQualityThreshold = 150.0;
bool onOff = false;
bool alarmTriggered = false;

// LED indikátory
bool ledOk = false;
bool ledError = false;
bool ledStatus = false;
#define LED_OK_PIN 35
#define LED_ERROR_PIN 36
#define LED_Status_PIN 37

// Senzory
MQ135 gasSensor = MQ135(6); // GPIO 6
Adafruit_BME280 bme;





// Buffer pro klouzavý průměr (pro MQ135)
const int MA_SAMPLES = 10;
float resBuffer[MA_SAMPLES];
float ppmBuffer[MA_SAMPLES];
int maIndex = 0;
bool maFilled = false;

// Blynk timer
BlynkTimer timer;


// -----------------------------
// Pomocné funkce
// -----------------------------

// Výpis hodnot senzorů do seriové linky
void printSensorData() {
  Serial.println("--- Sensor Readings ---");
  Serial.printf("Temperature : %.2f °C\n", temp);
  Serial.printf("Humidity    : %.2f %%\n", hum);
  Serial.printf("Pressure    : %.2f hPa\n", press);
  Serial.printf("Air Quality : %.2f ppm\n", airQuality);
  Serial.println("-----------------------");
}


// Čtení hodnot ze senzorů
void readSensors() {
  temp = bme.readTemperature();
  hum = bme.readHumidity();
  press = bme.readPressure() / 100.0F;
  airQuality = gasSensor.getCorrectedPPM(temp, hum);
  printSensorData();
}


// Aktualizace posledních hodnot
void updateLastValues() {
  lastTemp = temp;
  lastHum = hum;
  lastPress = press;
  lastAirQuality = airQuality;
}




// Nastavení LED indikátorů
void updateLEDs() {
  digitalWrite(LED_OK_PIN, ledOk ? HIGH : LOW);
  digitalWrite(LED_ERROR_PIN, ledError ? HIGH : LOW);
  digitalWrite(LED_Status_PIN, ledStatus ? HIGH : LOW);
}


// Odesílání hodnot senzorů do Blynk
void sendSensorDataBlynk() {
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, press);
  Blynk.virtualWrite(V3, airQuality);
}



// Blynk: změna prahových hodnot a zapnutí/vypnutí regulace
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


// Odeslání aktuálních nastavení do Blynk
void sendSettingsToBlynk() {
  Blynk.virtualWrite(V5, tempThreshold);
  Blynk.virtualWrite(V6, humThreshold);
  Blynk.virtualWrite(V7, pressThreshold);
  Blynk.virtualWrite(V8, airQualityThreshold);
  Blynk.virtualWrite(V9, onOff);
}


// -----------------------------
// Setup: inicializace systému
// -----------------------------
void setup() {
  Serial.begin(115200);
  loadSettings();

  // Inicializace pinů
  pinMode(35, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(47, OUTPUT);
  digitalWrite(47, HIGH);
  delay(1000);

  // I2C a ADC
  Wire.begin(42, 2);
  analogSetAttenuation(ADC_11db);
  analogReadResolution(10);

  // LED indikátory
  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ERROR_PIN, OUTPUT);
  pinMode(LED_Status_PIN, OUTPUT);

  // Senzor BME280
  bme.begin(0x77);

  // Kalibrace MQ135 v čistém vzduchu
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

  // Připojení k Wi-Fi a Blynk
  WiFi.mode(WIFI_STA);
  WiFi.setMinSecurity(WIFI_AUTH_WEP);
  Serial.println("Připojování k Wi-Fi...");
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Připojeno k Wi-Fi!");

  // Odeslání aktuálních nastavení do Blynk
  sendSettingsToBlynk();

  // Časovač pro odesílání dat každých 10 minut
  timer.setInterval(600000L, sendSensorDataBlynk);
}




// Kontrola prahových hodnot a alarmu
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
  Blynk.virtualWrite(V10, ledOk ? "OK" : "ALARM");
}


// -----------------------------
// Hlavní smyčka programu
// -----------------------------
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
  Blynk.run();
  timer.run();
  delay(1000);
}
