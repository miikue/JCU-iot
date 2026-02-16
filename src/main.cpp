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

// Prahové změny pro odesílaní dat
#define TEMP_CHANGE_THRESHOLD 0.5
#define HUM_CHANGE_THRESHOLD 1.0
#define PRESS_CHANGE_THRESHOLD 1.0
#define AIR_QUALITY_CHANGE_THRESHOLD 10.0

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


// Kontrola prahových hodnot a alarmu
void checkThresholds() {
  bool currentOk = temp < tempThreshold && hum < humThreshold && press < pressThreshold && airQuality < airQualityThreshold;
  
  if (currentOk != ledOk || !alarmTriggered) { 
    ledOk = currentOk;
    ledError = !ledOk;
    
    if (!ledOk && !alarmTriggered) {
      alarmTriggered = true;
      Serial.println("\n[!!! ALARM !!!] Hodnoty prekrocily prah!");
      Blynk.virtualWrite(V10, "ALARM");
      Blynk.logEvent("alarm_event", "Alarm spusten!");
    } else if (ledOk && alarmTriggered) {
      alarmTriggered = false;
      Serial.println("\n[OK] Hodnoty se vratily pod prah. Alarm zrusen.");
      Blynk.virtualWrite(V10, "OK");
      Blynk.logEvent("ok_event", "Hodnoty v norme");
    }
  }
}

// Čtení hodnot ze senzorů
void readSensors() {
  temp = bme.readTemperature();
  hum = bme.readHumidity();
  press = bme.readPressure() / 100.0F;
  airQuality = gasSensor.getCorrectedPPM(temp, hum);
  printSensorData();
}

// Nastavení LED indikátorů
void updateLEDs() {
  digitalWrite(LED_OK_PIN, ledOk ? HIGH : LOW);
  digitalWrite(LED_ERROR_PIN, ledError ? HIGH : LOW);
  digitalWrite(LED_Status_PIN, ledStatus ? HIGH : LOW);
}



// Odesílání hodnot senzorů do Blynk pouze při významné změně nebo vynuceně
unsigned long lastForcedSend = 0;
const unsigned long FORCE_SEND_INTERVAL = 15UL * 60UL * 1000UL; // 15 minut v ms

void sendSensorDataBlynk(bool force = false) {
  bool sendTemp = force || fabs(temp - lastTemp) >= TEMP_CHANGE_THRESHOLD;
  bool sendHum = force || fabs(hum - lastHum) >= HUM_CHANGE_THRESHOLD;
  bool sendPress = force || fabs(press - lastPress) >= PRESS_CHANGE_THRESHOLD;
  bool sendAirQuality = force || fabs(airQuality - lastAirQuality) >= AIR_QUALITY_CHANGE_THRESHOLD;

  if (sendTemp) {
    Blynk.virtualWrite(V0, temp);
    lastTemp = temp;
  }
  if (sendHum) {
    Blynk.virtualWrite(V1, hum);
    lastHum = hum;
  }
  if (sendPress) {
    Blynk.virtualWrite(V2, press);
    lastPress = press;
  }
  if (sendAirQuality) {
    Blynk.virtualWrite(V3, airQuality);
    lastAirQuality = airQuality;
  }
  if (force) {
    lastForcedSend = millis();
  }
}



// Blynk: změna prahových hodnot a zapnutí/vypnutí regulace
BLYNK_WRITE(V5) {
  tempThreshold = param.asFloat();
  Serial.println("\n=========================================");
  Serial.printf(">>> ZMĚNA PRAHU: TEPLOTA = %.2f °C <<<\n", tempThreshold);
  Serial.println("=========================================\n");
  saveSettings();
}

BLYNK_WRITE(V6) {
  humThreshold = param.asFloat();
  Serial.println("\n=========================================");
  Serial.printf(">>> ZMĚNA PRAHU: VLHKOST = %.2f %% <<<\n", humThreshold);
  Serial.println("=========================================\n");
  saveSettings();
}

BLYNK_WRITE(V7) {
  pressThreshold = param.asFloat();
  Serial.println("\n=========================================");
  Serial.printf(">>> ZMĚNA PRAHU: TLAK = %.2f hPa <<<\n", pressThreshold);
  Serial.println("=========================================\n");
  saveSettings();
}

BLYNK_WRITE(V8) {
  airQualityThreshold = param.asFloat();
  Serial.println("\n=========================================");
  Serial.printf(">>> ZMĚNA PRAHU: VZDUCH = %.2f ppm <<<\n", airQualityThreshold);
  Serial.println("=========================================\n");
  saveSettings();
}

BLYNK_WRITE(V9) {
  onOff = param.asInt() == 1;
  Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  Serial.printf("!!! REGULACE JE NYNÍ %s !!!\n", onOff ? "ZAPNUTÁ (ON)" : "VYPNUTÁ (OFF)");
  Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
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

// Funkce pro periodickou kontrolu senzorů a aktualizaci stavu LED a odesílání dat do Blynk
void oneSecondTask() {
  readSensors();
  ledStatus = !ledStatus;

  if (onOff) {
    checkThresholds();
  } else {
    ledOk = false;
    ledError = false;
  }

  updateLEDs();
  sendSensorDataBlynk(false);
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
  Serial.println("Heating MQ135 for 10 seconds...");
  for(int i = 10; i > 0; i--) {
    Serial.print("Heating... ");
    Serial.println(i);
    ledOk = !ledOk;
    ledError = !ledError;
    ledStatus = !ledStatus;
    updateLEDs();
    delay(1000);
  }
  Serial.println("Everything is ready!");
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

  // První načtení hodnot a inicializace last* proměnných
  readSensors();
  lastTemp = temp;
  lastHum = hum;
  lastPress = press;
  lastAirQuality = airQuality;

  // Vynucené odeslání dat při startu
  sendSensorDataBlynk(true);
  lastForcedSend = millis();

  // Časovač pro kontrolu změn každých 10 minut (pouze změny)
  timer.setInterval(1000L, oneSecondTask);
  timer.setInterval(900000L, []() { sendSensorDataBlynk(true); });
}





// -----------------------------
// Hlavní smyčka programu
// -----------------------------
void loop() {
  Blynk.run();
  timer.run();
}
