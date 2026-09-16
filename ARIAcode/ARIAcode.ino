#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "GUEST-N";
const char* password = "hello@2026";
const char* mqtt_server = "192.168.160.37";  // your laptop IP

//fields
int gas = analogRead(34);
int light = 300;      // replace later
int motion = 0;
int soil = 0;
float temp = 28;
float humidity = 60;
//;;;;;;;;;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");

  client.setServer(mqtt_server, 1883);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT connected");
    } else {
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // FAKE DATA (for testing)
String payload = "{";
payload += "\"gas\":" + String(gas) + ",";
payload += "\"light\":" + String(light) + ",";
payload += "\"motion\":" + String(motion) + ",";
payload += "\"soil\":" + String(soil) + ",";
payload += "\"temp\":" + String(temp) + ",";
payload += "\"humidity\":" + String(humidity) + ",";
payload += "\"status\":\"SAFE\",";
payload += "\"timestamp\":" + String(millis());
payload += "}";
//;;;;;;;;;;;;;;

  client.publish("home/data", payload.c_str());

  Serial.println("Data sent:");
  Serial.println(payload);

  delay(2000);
}