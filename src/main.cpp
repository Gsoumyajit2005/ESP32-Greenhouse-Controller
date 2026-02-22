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
#define RELAY_ON    LOW
#define RELAY_OFF   HIGH

#define BUZZER_ON   HIGH
#define BUZZER_OFF  LOW

// ---------------- DEMAND THRESHOLD ----------------
#define DEMAND_THRESHOLD 0.5

// ---------------- HYSTERESIS ----------------
// To prevent rapid toggling of the pump around the threshold, we can implement a simple hysteresis mechanism.
// For example, we can define a lower threshold for turning the pump off once it's on.
#define DEMAND_ON_THRESHOLD 0.55
#define DEMAND_OFF_THRESHOLD 0.45

// ---------------- MINIMUM RUN TIME ----------------
// To prevent short cycling of the pump, we can enforce a minimum run time once the pump is turned on.
#define MIN_PUMP_ON_TIME 10000  // Minimum time (in milliseconds) the pump should run once turned on

DHT dht(DHT_PIN, DHT_TYPE);

// ---------------- STATE VARIABLES ----------------
bool pumpState = false;          // Current state of the pump (ON/OFF)
unsigned long pumpStartTime = 0;   // Timestamp when the pump was turned on

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);
  
  pinMode(GREEN_LED_PIN, OUTPUT);
pinMode(RED_LED_PIN, OUTPUT);

  // Default state: assume pump OFF
  digitalWrite(GREEN_LED_PIN, LED_ON);
  digitalWrite(RED_LED_PIN, LED_OFF);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  dht.begin();

  Serial.println("Greenhouse Controller - Evaporation-Based Model");
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

  // Soil: 3000 (wet) → 0, 4000 (dry) → 1
  float soilFactor = (float)(soil - 3000) / 1000.0;
  soilFactor = constrain(soilFactor, 0.0, 1.0);

  // Temperature: 20°C → 0, 40°C → 1
  float tempFactor = (temp - 20.0) / 20.0;
  tempFactor = constrain(tempFactor, 0.0, 1.0);

  // Humidity: 0–100% → 0–1
  float humidityFactor = hum / 100.0;

  // Light: 0–4095 → 0–1
  float lightFactor = (float)light / 4095.0;

  // ---------------- EVAPORATION MODEL ----------------
  // Evaporation ∝ Temperature × (1 − Humidity)

  float evaporationFactor = tempFactor * (1.0 - humidityFactor);
  evaporationFactor = constrain(evaporationFactor, 0.0, 1.0);

  // ---------------- WEIGHTS ----------------
  float Ws = 0.6;   // Soil (dominant)
  float We = 0.3;   // Evaporation
  float Wl = 0.1;   // Light contribution

  // ---------------- WATER DEMAND ----------------
  float waterDemand =
      (soilFactor * Ws)
    + (evaporationFactor * We)
    + (lightFactor * Wl);

  waterDemand = constrain(waterDemand, 0.0, 1.0);

  Serial.print("Soil Factor: ");
  Serial.println(soilFactor);

  Serial.print("Evaporation Factor: ");
  Serial.println(evaporationFactor);

  Serial.print("Light Factor: ");
  Serial.println(lightFactor);

  Serial.print("Water Demand Score: ");
  Serial.println(waterDemand);

  // ---------------- HYSTERESIS + MINIMUM ON TIME ----------------
unsigned long currentTime = millis();

// Hysteresis logic
if (!pumpState && waterDemand > DEMAND_ON_THRESHOLD) {
    pumpState = true;
    pumpStartTime = currentTime;
}

if (pumpState && waterDemand < DEMAND_OFF_THRESHOLD) {
    // Only allow turning OFF if minimum runtime satisfied
    if (currentTime - pumpStartTime >= MIN_PUMP_ON_TIME) {
        pumpState = false;
    }
}

// Apply output based on pumpState
if (pumpState) {
    Serial.println("Pump ON (Robust Mode)");

    digitalWrite(RELAY_PIN, RELAY_OFF);
    digitalWrite(BUZZER_PIN, BUZZER_ON);

    digitalWrite(RED_LED_PIN, LED_ON);
    digitalWrite(GREEN_LED_PIN, LED_OFF);

} else {
    Serial.println("Pump OFF (Robust Mode)");

    digitalWrite(RELAY_PIN, RELAY_ON);
    digitalWrite(BUZZER_PIN, BUZZER_OFF);

    digitalWrite(RED_LED_PIN, LED_OFF);
    digitalWrite(GREEN_LED_PIN, LED_ON);
}

  Serial.println("-------------------------\n");

  delay(3000);
}



