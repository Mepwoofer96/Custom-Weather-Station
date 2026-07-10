

const int sensor = 5;
#define BLUE_LED 21
#define RED_LED 4

void setup() {
  pinMode(sensor, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
}

void loop() {
  int val = digitalRead(sensor);


  if (val == LOW) {  // South pole of magnet near sensor
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  }
}