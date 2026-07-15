// Librarys

// RTD
#include <Adafruit_MAX31865.h>
// DHT11
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Defines
//RTD Max31865
Adafruit_MAX31865 thermo = Adafruit_MAX31865(27, 14, 32, 33);  // CS, DI, DO, CLK

#define RREF 430.0      // Resistance ref for PT100
#define RNOMINAL 100.0  // nominal degrees C resistance of sensor for PT100

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

// Hall Effect Sensors
#define NUM_HALL 3
const int hallPins[NUM_HALL] = {5, 13, 26};  // D5, D13, D12

volatile uint64_t lastEdgeTime[NUM_HALL] = {0, 0, 0};
volatile uint64_t lastPeriod[NUM_HALL] = {0, 0, 0};
volatile unsigned long pulseCount[NUM_HALL] = {0, 0, 0};  // optional, useful for debug/avg RPM later


enum SensorState { IDLE,
                   SAMPLING };
SensorState sensorState = IDLE;
int sensorSampleIdx = 0;
const int SAMPLES_PER_WINDOW = 30;                                    //1hz for 30 seconds
const int WINDOWS_PER_SEND = 5;                                        // 10 min /2 min
const int TOTAL_SAMPLE_COUNT = SAMPLES_PER_WINDOW * WINDOWS_PER_SEND;  // 150
const unsigned long SENSOR_SAMPLE_INTERVAL = 1000;
unsigned long lastSensorSample = 0;
int windowSampleCount = 0; 


float temp1Buffer[TOTAL_SAMPLE_COUNT];
float temp2Buffer[TOTAL_SAMPLE_COUNT];
float humidityBuffer[TOTAL_SAMPLE_COUNT];
unsigned long sensorTimestamps[TOTAL_SAMPLE_COUNT];

// 2 min timing
const unsigned long SENSOR_WINDOW_INTERVAL = 120000;  // 2 min
unsigned long lastWindowFire = 0;

void updateWindowScheduler() {
  unsigned long now = millis();
  if (now - lastWindowFire >= SENSOR_WINDOW_INTERVAL) {
    lastWindowFire = now;
    if (sensorState == IDLE && sensorSampleIdx < TOTAL_SAMPLE_COUNT) {  // don't clobber an in-progress window and check for a filled matrix
      startSensorWindow();
    }
  }
}

// 10 min timing
const unsigned long SERVER_SEND_INTERVAL = 600000;  // 10 min
unsigned long lastServerFire = 0;

void serverSend() {
  if (sensorSampleIdx >= TOTAL_SAMPLE_COUNT) {
    Serial.println("Buffer");
    Serial.println("idx\ttimestamp\trtd_C\tdht_C\thumidity_%");
    for (int i = 0; i < TOTAL_SAMPLE_COUNT; i++) {
      Serial.print(i);
      Serial.print("\t");
      Serial.print(sensorTimestamps[i]);
      Serial.print("\t");
      Serial.print(temp1Buffer[i], 2);
      Serial.print("\t");
      Serial.print(temp2Buffer[i], 2);
      Serial.print("\t");
      Serial.println(humidityBuffer[i], 2);


      
    }
      // Hall effects
     uint64_t periods[NUM_HALL];
    unsigned long counts[NUM_HALL];
    getHallSnapshot(periods, counts);
    Serial.println("hall\tperiod_us\tpulses\trpm");
    for (int i = 0; i < NUM_HALL; i++) {
      float rpm = (periods[i] > 0) ? (60000000.0 / periods[i]) : 0;
      Serial.print(i); Serial.print("\t");
      Serial.print((unsigned long)periods[i]); Serial.print("\t");
      Serial.print(counts[i]); Serial.print("\t");
      Serial.println(rpm, 2);
    }
    sensorSampleIdx = 0;
  }
}


void startSensorWindow() {
  sensorState = SAMPLING;
  lastSensorSample = millis();
  windowSampleCount = 0;
}

void IRAM_ATTR hallISR0() {
  uint64_t now = esp_timer_get_time();
  lastPeriod[0] = now - lastEdgeTime[0];
  lastEdgeTime[0] = now;
  pulseCount[0]++;
}

void IRAM_ATTR hallISR1() {
  uint64_t now = esp_timer_get_time();
  lastPeriod[1] = now - lastEdgeTime[1];
  lastEdgeTime[1] = now;
  pulseCount[1]++;
}

void IRAM_ATTR hallISR2() {
  uint64_t now = esp_timer_get_time();
  lastPeriod[2] = now - lastEdgeTime[2];
  lastEdgeTime[2] = now;
  pulseCount[2]++;
}


void getHallSnapshot(uint64_t periodOut[NUM_HALL], unsigned long countOut[NUM_HALL]) {
  noInterrupts();
  for (int i = 0; i < NUM_HALL; i++) {
    periodOut[i] = lastPeriod[i];
    countOut[i] = pulseCount[i];
  }
  interrupts();
}

void updateSensor() {

  if (sensorState != SAMPLING) return;
  unsigned long now = millis();

  if (now - lastSensorSample >= SENSOR_SAMPLE_INTERVAL) {
    lastSensorSample = now;

    digitalWrite(BLUE_LED, HIGH);

    // RTD
    float tempC1 = thermo.temperature(RNOMINAL, RREF);
    temp1Buffer[sensorSampleIdx] = tempC1;

    // DHT11 Temp
    dht.temperature().getEvent(&event);
    float tempC2 = event.temperature;
    temp2Buffer[sensorSampleIdx] = tempC2;

    // DHT11 Humidity
    dht.humidity().getEvent(&event);
    float humidity = event.relative_humidity;
    humidityBuffer[sensorSampleIdx] = humidity;

    // Timing
    sensorTimestamps[sensorSampleIdx] = now;
    sensorSampleIdx++; //overall 
    windowSampleCount++; // per run

    if (windowSampleCount >= SAMPLES_PER_WINDOW) {
      sensorState = IDLE;  // window done, buffer ready to be sent
      digitalWrite(BLUE_LED, LOW);
    }
  }
}

// Health check
bool sensorHealthy = true;
unsigned long lastFaultCheck = 0;
const unsigned long FAULT_CHECK_INTERVAL = 10000;

void updateFaultCheck() {
  unsigned long now = millis();
  if (now - lastFaultCheck < FAULT_CHECK_INTERVAL) return;
  lastFaultCheck = now;

  uint8_t fault = thermo.readFault();
  uint16_t rtd = thermo.readRTD();

  float resistance = RREF * ((float)rtd / 32768);
  dht.temperature().getEvent(&event);
  if (resistance == 0) {
    sensorHealthy = false;  // not plugged in
  } else if (resistance >= 195) {
    sensorHealthy = false;  // fuse/overrange
  } else if (fault) {
    thermo.clearFault();
    sensorHealthy = false;
  } else if (isnan(event.temperature)) {
    sensorHealthy = false;
  } else {
    sensorHealthy = true;
  }

  if (!sensorHealthy) {
    digitalWrite(RED_LED, HIGH);
    Serial.println("FAULT");
  } else {
    digitalWrite(RED_LED, LOW);
  }
}

void setup() {
  Serial.begin(9600);

  // DHT11 Setup
  dht.begin();
  delay(100);
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  // RTD setup
  thermo.begin(MAX31865_3WIRE);
  thermo.clearFault();
  thermo.readRTD();

  // LED setup
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

// Hall sensor setup
  pinMode(hallPins[0], INPUT_PULLUP);
  pinMode(hallPins[1], INPUT_PULLUP);
  pinMode(hallPins[2], INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallPins[0]), hallISR0, FALLING);
  attachInterrupt(digitalPinToInterrupt(hallPins[1]), hallISR1, FALLING);
  attachInterrupt(digitalPinToInterrupt(hallPins[2]), hallISR2, FALLING);


  delay(2000);
  Serial.println("Serial De box er");
  delay(200);
  Serial.println("Serial De box er");
}

void loop() {

  updateWindowScheduler();
  updateFaultCheck();
  updateSensor();
  serverSend();
  // updateNetworkSend(); // checks if buffers are full / interval elapsed
  // hall sensors need no update() call — ISRs run independently
}