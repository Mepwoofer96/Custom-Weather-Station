
#include <Adafruit_MAX31865.h>

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(27, 14, 32, 33);  // CS, DI, DO, CLK




// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31865 thermo = Adafruit_MAX31865(10);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF 430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL 100.0

#define LED_BUILTIN 2
#define RED_LED 4
#define BLUE_LED 21

// Fault check 
unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 10000; // every 30 seconds

// Temp Check
unsigned long lastAutoRead = 0;
const unsigned long AUTO_READ_INTERVAL = 600000; // 10 minutes, in milliseconds (10 * 60 * 1000)


void checkSensorHealth() {
  bool health = false;
  while(health == false){
  uint16_t rtd = thermo.readRTD();
  uint8_t fault = thermo.readFault();
  float ratio = rtd;
  ratio /= 32768;
  float resistance = RREF * ratio;

  if (resistance >= 195) {
    Serial.println("Reset Fuse");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    health = false;
  } else if (resistance == 0) {
    Serial.println("Not Plugged in");
    digitalWrite(RED_LED, HIGH);
    health = false;
  } else if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH)  Serial.println("RTD High Threshold");
    if (fault & MAX31865_FAULT_LOWTHRESH)   Serial.println("RTD Low Threshold");
    if (fault & MAX31865_FAULT_REFINLOW)    Serial.println("REFIN- > 0.85 x Bias");
    if (fault & MAX31865_FAULT_REFINHIGH)   Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
    if (fault & MAX31865_FAULT_RTDINLOW)    Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
    if (fault & MAX31865_FAULT_OVUV)        Serial.println("Under/Over voltage");
    thermo.clearFault();
    digitalWrite(RED_LED, HIGH);
    health = false;
  } else {
    digitalWrite(RED_LED, LOW);
    health = true;
  }
  }
}

void readTemp(){
    int idx = 0;
    float tempF;
    float lasttempF = 0;
    for (int i = 0; i < 6; ++i) {

      uint16_t rtd = thermo.readRTD();
      float ratio = rtd;
      ratio /= 32768;
      float resistance = RREF * ratio;
      tempF = ((thermo.temperature(RNOMINAL, RREF) * (9.0 / 5.0)) + 32);
      Serial.print("RTD value: ");
      Serial.println(rtd);
      Serial.print("Ratio = ");
      Serial.println(ratio, 8);
      Serial.print("Resistance = ");
      Serial.println(RREF * ratio, 8);
      Serial.print("Temperature C = ");
      Serial.println(thermo.temperature(RNOMINAL, RREF));
      Serial.print("Temperature F = ");
      Serial.println(((thermo.temperature(RNOMINAL, RREF) * (9.0 / 5.0)) + 32));
      delay(1000);
      lasttempF += tempF; 
      ++idx;
    }
    Serial.println("Average Temp over " + String(idx) + " Seconds" );
    Serial.println(String(lasttempF/idx)+ " F");
  }



void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Adafruit MAX31865 PT100 Sensor Test!");
  delay(500);                    // let power rail fully settle before touching MAX31865
  thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  thermo.clearFault();
  delay(100);
  thermo.readRTD();
  delay(50);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(22, INPUT_PULLUP);
  Serial.println("PRINT ME");
}


void loop() {


  int buttonState = digitalRead(22);
  unsigned long now = millis();
  bool timeToAutoRead = (now - lastAutoRead >= AUTO_READ_INTERVAL);
  
  if (now - lastCheck >= CHECK_INTERVAL) {
    lastCheck = now;
    checkSensorHealth();
  }

  if (buttonState == LOW || timeToAutoRead) {
    digitalWrite(BLUE_LED, HIGH);

    if (buttonState == LOW) {
      Serial.println("BUTTON PRESSED - reading temp");
    } else {
      Serial.println("AUTO 10-MIN READ - reading temp");
      lastAutoRead = now; // only reset the timer on an auto-triggered read
    }

    readTemp();
    digitalWrite(BLUE_LED, LOW);
  }

}
