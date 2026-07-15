#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 


const char* ssid = "your_wifi";
const char* password = "your_password";
const char* mqtt_server = "192.168.1.37"; // server/broker IP
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastPublish = 0;
const long publishInterval = 10000; // 10 sec

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// This fires whenever a message arrives on a subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.printf("Message on [%s]: %s\n", topic, msg.c_str());

  if (String(topic) == "station/camera/set_position") {
    int angle = msg.toInt();
    moveCameraServo(angle); //  servo control function
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_WeatherStation")) {
      Serial.println("connected");
      client.subscribe("station/camera/set_position"); // listen for camera commands
    } else {
      Serial.printf("failed, rc=%d, retrying in 5s\n", client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void publishSensorData(float temp, float humidity, float windSpeed, int windDir) {
  StaticJsonDocument<200> doc;
…
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // MUST be called regularly — handles keepalive + incoming messages

  unsigned long now = millis();
  if (now - lastPublish > publishInterval) {
    lastPublish = now;
    float temp = readTemp();       // your sensor read functions
    float humidity = readHumidity();
    float windSpeed = readWindSpeed();
    int windDir = readWindDir();

    publishSensorData(temp, humidity, windSpeed, windDir);
  }
}