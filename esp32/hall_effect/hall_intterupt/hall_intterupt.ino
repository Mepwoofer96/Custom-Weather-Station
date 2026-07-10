const int sensor = 5;
volatile int count = 0;
#define BLUE_LED 21
#define RED_LED 4

// Fault check
unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 5000;  // every 5 seconds

void onMagnet() {
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  count++;
  delay(300);
}

void setup() {
  Serial.begin(9600);
  pinMode(sensor, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(sensor), onMagnet, FALLING);
  delay(100);
  Serial.println("Intitalized");
}

void loop() {
  unsigned long now = millis();

  digitalWrite(RED_LED, HIGH);
  digitalWrite(BLUE_LED, LOW);

  if (now - lastCheck >= CHECK_INTERVAL) {
    lastCheck = now;
    Serial.println(count);
  }
}