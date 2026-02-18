#include <Arduino.h>
#include <DHT.h>

// ---------------- PIN DEFINITIONS ----------------
#define DHT_PIN     27
#define DHT_TYPE    DHT22
#define SOIL_PIN    34
#define LDR_PIN     35
#define RELAY_PIN   26
#define BUZZER_PIN  25

// ---------------- DEVICE POLARITY ----------------
// Relay is ACTIVE LOW
#define RELAY_ON    LOW
#define RELAY_OFF   HIGH

// Buzzer is ACTIVE HIGH
#define BUZZER_ON   HIGH
#define BUZZER_OFF  LOW

// ---------------- DEMAND THRESHOLD ----------------
#define DEMAND_THRESHOLD 0.5

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);   // Safe default

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  dht.begin();

  Serial.println("Greenhouse Controller - Multi-Sensor Weighted Model");
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

  // ---------------- PRINT RAW VALUES ----------------
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
  // Soil: 3000 (wet) → 0, 4000+ (very dry) → 1
  float soilFactor = (float)(soil - 3000) / 1000.0;
  soilFactor = constrain(soilFactor, 0.0, 1.0);

  // Temperature: 20°C → 0, 40°C → 1
  float tempFactor = (temp - 20.0) / 20.0;
  tempFactor = constrain(tempFactor, 0.0, 1.0);

  // Light: scale 0–4095 to 0–1
  float lightFactor = (float)light / 4095.0;

  // Humidity: 0–100% to 0–1
  float humidityFactor = hum / 100.0;

  // ---------------- WEIGHTS ----------------
  float Ws = 0.5;   // Soil weight (primary)
  float Wt = 0.2;   // Temperature weight
  float Wl = 0.2;   // Light weight
  float Wh = 0.3;   // Humidity reduction weight

  // ---------------- WATER DEMAND CALCULATION ----------------
  float waterDemand =
      (soilFactor * Ws)
    + (tempFactor * Wt)
    + (lightFactor * Wl)
    - (humidityFactor * Wh);

  waterDemand = constrain(waterDemand, 0.0, 1.0);

  Serial.print("Water Demand Score: ");
  Serial.println(waterDemand);

  // ---------------- DECISION ----------------
  bool pumpShouldRun = (waterDemand > DEMAND_THRESHOLD);

  if (pumpShouldRun) {
    Serial.println("High Water Demand → Pump ON");
    digitalWrite(RELAY_PIN, RELAY_ON);
    digitalWrite(BUZZER_PIN, BUZZER_ON);
  } else {
    Serial.println("Low Water Demand → Pump OFF");
    digitalWrite(RELAY_PIN, RELAY_OFF);
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
  }

  Serial.println("-------------------------\n");

  delay(3000);
}