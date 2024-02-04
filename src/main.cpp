#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* hostname = "{{HOSTNAME}}";

const char* ssid = "{{WIFI_SSID}}";
const char* password = "{{WIFI_PASSWD}}";

const char* mqtt_server = "{{MQTT_IP}}";
const char* mqtt_user = "{{MQTT_USER}}";
const char* mqtt_pass = "{{MQTT_PASSWD}}";
const char* mqtt_topic_cmd = "{{MQTT_TOPIC_CMD}}";
const char* mqtt_topic_base = "{{MQTT_TOPIC_BASE}}";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// void callback() {}

void reconnect() {
  // Loop until we're reconnected to MQTT
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(hostname, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.setKeepAlive(30);
      client.subscribe(mqtt_topic_cmd);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  Serial.begin(115200);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  // client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
