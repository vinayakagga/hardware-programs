#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <BH1750.h>

// ---------- WIFI ----------
const char* ssid = "GUEST-N";
const char* password = "hello@2026";
const char* mqtt_server = "192.168.160.37";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- MOTOR ----------
int IN1 = 25, IN2 = 26, IN3 = 27, IN4 = 14;
int ENA = 32, ENB = 33;

// ---------- DHT ----------
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------- SENSORS ----------
int gasPin = 34;
int soilPin = 35;
BH1750 lightMeter;

// ---------- MQTT ----------
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT connected");
    } else {
      delay(2000);
    }
  }
}

// ---------- MOTOR ----------
void forward() {
  digitalWrite(IN1, 0); digitalWrite(IN2, 1);
  digitalWrite(IN3, 0); digitalWrite(IN4, 1);
  ledcWrite(ENA, 200);
  ledcWrite(ENB, 200);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  pinMode(5,OUTPUT);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  client.setServer(mqtt_server, 1883);

  // Motor
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 5000, 8);
  ledcAttach(ENB, 5000, 8);

  // Sensors
  dht.begin();
  Wire.begin(21, 22);
  lightMeter.begin();
}

// ---------- LOOP ----------
void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  Serial.println("===== NEW CYCLE =====");

  // 1️⃣ MOVE
  forward();
  delay(2000);

  // 2️⃣ STOP (VERY IMPORTANT FOR SERVO)
  stopMotors();
  delay(3000);

  // 3️⃣ SERVO ACTION
  Serial.println("Servo moving...");
  digitalWrite(5,1);
  delay(2000);
 digitalWrite(5,0);
  
  delay(2000);

  // 4️⃣ SENSOR READ
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gas = analogRead(gasPin);
  int soil = analogRead(soilPin);
  float light = lightMeter.readLightLevel();

  Serial.println("---- SENSOR DATA ----");

  if (isnan(temp) || isnan(humidity)) {
    Serial.println("DHT ERROR");
  } else {
    Serial.print("Temp: "); Serial.println(temp);
    Serial.print("Humidity: "); Serial.println(humidity);
  }

  Serial.print("Gas: "); Serial.println(gas);
  Serial.print("Soil: "); Serial.println(soil);
  Serial.print("Light: "); Serial.println(light);

  // 5️⃣ MQTT SEND
  String payload = "{";
  payload += "\"gas\":" + String(gas) + ",";
  payload += "\"light\":" + String(light) + ",";
  payload += "\"soil\":" + String(soil) + ",";
  payload += "\"temp\":" + String(temp) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"status\":\"SAFE\",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";

  client.publish("home/data", payload.c_str());

  Serial.println("Data sent:");
  Serial.println(payload);

  Serial.println("===== CYCLE COMPLETE =====\n");

  delay(3000);
}