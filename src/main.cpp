#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

const char* hostname = "{{HOSTNAME}}";

const char* ssid = "{{WIFI_SSID}}";
const char* password = "{{WIFI_PASSWD}}";

const char* mqtt_server = "{{MQTT_IP}}";
const char* mqtt_user = "{{MQTT_USER}}";
const char* mqtt_pass = "{{MQTT_PASSWD}}";
const char* mqtt_topic_cmd = "{{MQTT_TOPIC_CMD}}";
const char* mqtt_topic_state = "{{MQTT_TOPIC_BASE}}";


WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);

  // We start by connecting to a WiFi network
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.print("Length:");
  Serial.print(length);
}

// reconnect non-blocking
long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect(hostname, mqtt_user, mqtt_pass)) {
    // Once connected, publish an announcement...
    client.publish(mqtt_topic_state,"connected");
    // ... and resubscribe
    client.subscribe(mqtt_topic_cmd);
  }
  return client.connected();
}

void setup() {
  Serial.begin(115200);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  lastReconnectAttempt = 0;
}

// loop non-blocking
void loop()
{
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }

}