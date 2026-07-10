// INFO ON SENSOR
// ------------------------------------
// Temperature Sensor
// Sensor Type: DHT11
// Driver Ver:  1
// Unique ID:   -1
// Max Value:   50.00°C
// Min Value:   0.00°C
// Resolution:  2.00°C
// ------------------------------------
// Humidity Sensor
// Sensor Type: DHT11
// Driver Ver:  1
// Unique ID:   -1
// Max Value:   80.00%
// Min Value:   20.00%
// Resolution:  5.00%
// ------------------------------------


#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// DHT11 defines
#define DHTPIN 18      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);

// Event for measurement
sensors_event_t event;

// LED PINS
#define LED_BUILTIN 2
#define RED_LED 4
#define BLUE_LED 21
#define BUTTON_PIN 22

// Fault check
unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 10000;  // every 30 seconds

// Sensor Check
unsigned long lastAutoRead = 0;
const unsigned long AUTO_READ_INTERVAL = 600000;  // 10 minutes, in milliseconds (10 * 60 * 1000)



void checkSensorHealth() {
  bool health = false;
  //check
  dht.humidity().getEvent(&event);
  dht.temperature().getEvent(&event);

  while (health == false) {
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
      digitalWrite(RED_LED, HIGH);
    } else if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
      digitalWrite(RED_LED, HIGH);
    } else {
      digitalWrite(RED_LED, LOW);
      health = true;
    }
  }
}

void readSensor() {

  int idx = 0;
  float tempC = 0;
  float humid = 0;
  float tempF = 0;
  float lasttempF = 0;
  float lasthumid = 0;
  for (int i = 0; i < 6; ++i) {

    // Get measurements
    dht.temperature().getEvent(&event);
    tempC = event.temperature;
    dht.humidity().getEvent(&event);
    humid = event.relative_humidity;
    tempF = tempC * (9.0 / 5.0) + 32;
    lasttempF += tempF;
    lasthumid += humid;

    // Temp
    Serial.print(F("Temperature: "));
    Serial.print(tempC);
    Serial.println(F("°C"));
    Serial.print(tempF);
    Serial.println(F("°F"));

    // Humid
    Serial.print(F("Humidity: "));
    Serial.print(humid);
    Serial.println(F("%"));

    ++idx;
    delay(1000);
  }
  Serial.println("Average Temp and humidity over " + String(idx) + " Seconds");
  Serial.println(String(lasttempF / idx) + " F");
  Serial.println(String(lasthumid / idx) + " %");
}

uint32_t delayMS;

void setup() {
  Serial.begin(9600);
  // Initialize device.
  dht.begin();
  delay(100);
  Serial.println(F("DHT11 Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  delay(100);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Set delay between sensor readings based on sensor details.
  // delayMS = sensor.min_delay;
  // 1000000 = delayMS
}

void loop() {

  int buttonState = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  bool timeToAutoRead = (now - lastAutoRead >= AUTO_READ_INTERVAL);

  if (now - lastCheck >= CHECK_INTERVAL) {
    lastCheck = now;
    checkSensorHealth();
  }

  if (buttonState == LOW || timeToAutoRead) {
    digitalWrite(BLUE_LED, HIGH);

    if (buttonState == LOW) {
      Serial.println("BUTTON PRESSED - reading sensor");
    } else {
      Serial.println("AUTO 10-MIN READ - reading sensor");
      lastAutoRead = now;  // only reset the timer on an auto-triggered read
    }

    readSensor();
    digitalWrite(BLUE_LED, LOW);
  }
}
