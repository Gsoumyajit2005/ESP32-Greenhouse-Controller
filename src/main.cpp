#include <Arduino.h>
#include <DHT.h>

// ---------------- PIN DEFINITIONS ----------------
#define DHT_PIN     27
#define DHT_TYPE    DHT22
#define SOIL_PIN    34
#define LDR_PIN     35
#define RELAY_PIN   26
#define BUZZER_PIN  25
#define GREEN_LED_PIN 32
#define RED_LED_PIN   33

#define LED_ON  HIGH
#define LED_OFF LOW

// ---------------- DEVICE POLARITY ----------------
#define RELAY_ON    HIGH
#define RELAY_OFF   LOW

#define BUZZER_ON   HIGH
#define BUZZER_OFF  LOW

// ---------------- DAY/NIGHT CONTROL ----------------
#define LIGHT_DAY_THRESHOLD 0.2

DHT dht(DHT_PIN, DHT_TYPE);

// ---------------- STATE VARIABLES ----------------
bool pumpState = false;
bool burstActive = false;
unsigned long burstTimer = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LED_ON);
  digitalWrite(RED_LED_PIN, LED_OFF);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  dht.begin();

  Serial.println("Greenhouse Controller - Climate Aware Burst Mode");
}

void loop() {

  // ---------------- READ SENSORS ----------------
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int soil   = analogRead(SOIL_PIN);
  int light  = analogRead(LDR_PIN);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT22 read failed! Pump forced OFF.");
    digitalWrite(RELAY_PIN, RELAY_OFF);
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    delay(2000);
    return;
  }

  Serial.println("---- Sensor Readings ----");

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  Serial.print("Soil ADC: ");
  Serial.println(soil);

  Serial.print("Light ADC: ");
  Serial.println(light);

  // ---------------- NORMALIZATION ----------------

  float soilFactor = constrain((soil - 3000) / 1000.0, 0.0, 1.0);
  float tempFactor = constrain((temp - 20.0) / 20.0, 0.0, 1.0);
  float humidityFactor = hum / 100.0;
  float lightFactor = (float)light / 4095.0;

  float evaporationFactor = constrain(tempFactor * (1.0 - humidityFactor), 0.0, 1.0);

  Serial.print("Soil Factor: ");
  Serial.println(soilFactor);

  Serial.print("Evaporation Factor: ");
  Serial.println(evaporationFactor);

  Serial.print("Light Factor: ");
  Serial.println(lightFactor);

  // ---------------- IRRIGATION LOGIC ----------------

  unsigned long currentTime = millis();

  bool isDaytime = (lightFactor > LIGHT_DAY_THRESHOLD);
  bool soilDryEnough = (soilFactor > 0.3);

  Serial.print("Daytime: ");
  Serial.println(isDaytime ? "YES" : "NO");

  Serial.print("Soil Dry Enough: ");
  Serial.println(soilDryEnough ? "YES" : "NO");

  // Dynamic burst timing
  unsigned long dynamicOffTime = 20000 - (evaporationFactor * 15000); // 5–20 sec
  unsigned long dynamicOnTime  = 5000;

  Serial.print("Dynamic OFF Time (ms): ");
  Serial.println(dynamicOffTime);

  bool irrigationAllowed = soilDryEnough && isDaytime;

  if (irrigationAllowed) {

      if (!burstActive) {
          burstActive = true;
          pumpState = true;
          burstTimer = currentTime;
      }

      if (pumpState && (currentTime - burstTimer >= dynamicOnTime)) {
          pumpState = false;
          burstTimer = currentTime;
      }

      if (!pumpState && (currentTime - burstTimer >= dynamicOffTime)) {
          pumpState = true;
          burstTimer = currentTime;
      }

  } else {
      pumpState = false;
      burstActive = false;
  }

  Serial.print("Pump State: ");
  Serial.println(pumpState ? "ON" : "OFF");

  // ---------------- APPLY OUTPUT ----------------

  if (pumpState) {
      Serial.println("Burst Mode: Pump ON");

      digitalWrite(RELAY_PIN, RELAY_ON);
      digitalWrite(BUZZER_PIN, BUZZER_ON);
      digitalWrite(RED_LED_PIN, LED_ON);
      digitalWrite(GREEN_LED_PIN, LED_OFF);
  } else {
      Serial.println("Burst Mode: Pump OFF");

      digitalWrite(RELAY_PIN, RELAY_OFF);
      digitalWrite(BUZZER_PIN, BUZZER_OFF);
      digitalWrite(RED_LED_PIN, LED_OFF);
      digitalWrite(GREEN_LED_PIN, LED_ON);
  }

  Serial.println("-------------------------\n");

  delay(3000);
}
