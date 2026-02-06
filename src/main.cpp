#include <Arduino.h>
#include <DHT.h>

// ---- Pin Definitions ----
#define DHT_PIN       27
#define DHT_TYPE      DHT22
#define SOIL_PIN      34
#define LDR_PIN       35
#define RELAY_PIN     26
#define BUZZER_PIN    25
#define GREEN_LED_PIN 32
#define RED_LED_PIN   33

// ---- Relay & Buzzer Logic ----
#define RELAY_ON   LOW    // relay is active LOW
#define RELAY_OFF  HIGH

#define BUZZER_ON  HIGH
#define BUZZER_OFF LOW

#define LED_ON  HIGH
#define LED_OFF LOW

// ---- Thresholds (temporary) ----
#define SOIL_DRY_THRESHOLD 3500

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);   // safe default

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);  // safe default

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Safe default: assume soil OK at boot
  digitalWrite(GREEN_LED_PIN, LED_ON);
  digitalWrite(RED_LED_PIN, LED_OFF);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  dht.begin();

  Serial.println("Greenhouse Controller - Integrated Test (NO PUMP)");
}

void loop() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int soil   = analogRead(SOIL_PIN);
  int light  = analogRead(LDR_PIN);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT22 read failed!");
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

  // ---- Control Logic ----
  if (soil > SOIL_DRY_THRESHOLD) {
  Serial.println("Soil DRY → Pump ON → Buzzer ON → RED LED ON");

  digitalWrite(RELAY_PIN, RELAY_ON);
  digitalWrite(BUZZER_PIN, BUZZER_ON);

  digitalWrite(RED_LED_PIN, LED_ON);
  digitalWrite(GREEN_LED_PIN, LED_OFF);

  } else {
  Serial.println("Soil OK → Pump OFF → Buzzer OFF → GREEN LED ON");

  digitalWrite(RELAY_PIN, RELAY_OFF);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);

  digitalWrite(RED_LED_PIN, LED_OFF);
  digitalWrite(GREEN_LED_PIN, LED_ON);
  }

  Serial.println("-------------------------\n");
  delay(3000);
}
