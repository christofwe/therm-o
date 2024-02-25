#include <string>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define MYHOME
#include "secrets.h"

LiquidCrystal_I2C lcd(0x27,20,4);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
JsonDocument doc;

String mqtt_main_topic = "therm-o/";
const char* mqtt_topic_cmd = "therm-o/cmd";
String mqtt_discovery_topic_sensor = "homeassistant/sensor/therm-o/";
String mqtt_discovery_topic_select = "homeassistant/select/therm-o/";
String mqtt_discovery_topic_number = "homeassistant/number/therm-o/";

char mode[] = "automatic";
char priority[] = "normal";

float Temp_POOL_VL = 0;
float Temp_POOL_RL = 0;
float Temp_POOL_IST = 0;
float Temp_WWA_IST = 0;
float Temp_SK_VL = 0;
float Temp_SK_RL = 0;
float Temp_SK_IST = 0;
float Temp_WWH_VL = 0;
float Temp_WWH_RL = 0;
float Temp_WWH_IST = 0;

float diff_wwh;
float diff_pool;
float diff_wwa;

int wwh_limit;
int wwh_min = 50;
int wwh_max = 58;
int wwh_relais_state;
int last_wwh_relais_state;
unsigned long uptime;

String get_config(){
  JsonDocument doc;
  doc["free_mem"] = ESP.getFreeHeap();
  doc["mode"] = mode;
  doc["priority"] = priority;
  doc["uptime"] = uptime;
  doc["wwh_min"] = wwh_min;
  doc["wwh_max"] = wwh_max;
  return doc.as<String>();
}

String get_temp(){
  JsonDocument doc;
  doc["pool_vl"] = Temp_POOL_VL;
  doc["pool_rl"] = Temp_POOL_RL;
  doc["pool_ist"] = Temp_POOL_IST;
  doc["wwa_ist"] = Temp_WWA_IST;
  doc["sk_vl"] = Temp_SK_VL;
  doc["sk_rl"] = Temp_SK_RL;
  doc["sk_ist"] = Temp_SK_IST;
  doc["wwh_vl"] = Temp_WWH_VL;
  doc["wwh_rl"] = Temp_WWH_RL;
  doc["wwh_ist"] = Temp_WWH_IST;
  return doc.as<String>();
}

String set_ha_discovery_sensor(char* item){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_") + std::string(item);
  doc["object_id"] = CONTROLLER_NAME + std::string("_") + std::string(item);
  doc["name"] = std::string(item) + " Temperature";
  doc["icon"] = "mdi:thermometer";
  doc["unit_of_meas"] = "°C";
  doc["device_class"] = "temperature";
  doc["state_class"] = "measurement";
  doc["state_topic"] = mqtt_main_topic + "sensor";
  doc["value_template"] = "{{ value_json." + std::string(item) +" }}";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

String set_ha_discovery_free_mem(){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_free_mem");
  doc["object_id"] = CONTROLLER_NAME + std::string("_free_mem");
  doc["name"] = "Free Memory";
  doc["icon"] = "mdi:memory";
  doc["unit_of_meas"] = "B";
  doc["state_class"] = "measurement";
  doc["entity_category"] = "diagnostic";
  doc["state_topic"] = mqtt_main_topic + "config";
  doc["value_template"] = "{{ value_json.free_mem }}";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

String set_ha_discovery_uptime(){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_uptime");
  doc["object_id"] = CONTROLLER_NAME + std::string("_uptime");
  doc["name"] = "Uptime";
  doc["icon"] = "mdi:clock-time-eight-outline";
  doc["unit_of_meas"] = "s";
  doc["state_class"] = "measurement";
  doc["entity_category"] = "diagnostic";
  doc["state_topic"] = mqtt_main_topic + "config";
  doc["value_template"] = "{{ value_json.uptime }}";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

// not working yet
String set_ha_discovery_mode(){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_mode");
  doc["object_id"] = CONTROLLER_NAME + std::string("_mode");
  doc["name"] = "Mode";
  doc["icon"] = "mdi:refresh-auto";
  doc["state_topic"] = mqtt_main_topic + "config";
  doc["value_template"] = "{{ value_json.mode }}";
  doc["command_topic"] = mqtt_main_topic + "cmd";
  doc["command_template"] = "{\"mode\":\"{{ value }}\"}";
  doc["options"][0] = "off";
  doc["options"][1] = "automatic";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

String set_ha_discovery_priority(){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_priority");
  doc["object_id"] = CONTROLLER_NAME + std::string("_priority");
  doc["name"] = "Priority";
  doc["icon"] = "mdi:priority-high";
  doc["state_topic"] = mqtt_main_topic + "config";
  doc["value_template"] = "{{ value_json.priority }}";
  doc["command_topic"] = mqtt_main_topic + "cmd";
  doc["command_template"] = "{\"priority\":\"{{ value }}\"}";
  doc["options"][0] = "normal";
  doc["options"][1] = "winter";
  doc["options"][2] = "pool";
  doc["options"][3] = "wwaoff";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

String set_ha_discovery_wwh_min(){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_wwh_min");
  doc["object_id"] = CONTROLLER_NAME + std::string("_wwh_min");
  doc["name"] = "WWH Min";
  doc["icon"] = "mdi:thermometer-low";
  doc["state_topic"] = mqtt_main_topic + "config";
  doc["value_template"] = "{{ value_json.wwh_min }}";
  doc["command_topic"] = mqtt_main_topic + "cmd";
  doc["command_template"] = "{\"wwh_min\": {{ value }}}";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

String set_ha_discovery_wwh_max(){
  JsonDocument doc;
  doc["~"] = CONTROLLER_NAME;
  doc["unique_id"] = CONTROLLER_NAME + std::string("_wwh_max");
  doc["object_id"] = CONTROLLER_NAME + std::string("_wwh_max");
  doc["name"] = "WWH Max";
  doc["icon"] = "mdi:thermometer-high";
  doc["state_topic"] = mqtt_main_topic + "config";
  doc["value_template"] = "{{ value_json.wwh_max }}";
  doc["command_topic"] = mqtt_main_topic + "cmd";
  doc["command_template"] = "{\"wwh_max\": {{ value }}}";
  doc["availability_topic"] = mqtt_main_topic + "state";
  doc["payload_available"] = "connected";
  doc["payload_not_available"] = "disconnected";
  doc["device"]["identifiers"] = CONTROLLER_NAME;
  doc["device"]["name"] = CONTROLLER_NAME;
  doc["device"]["model"] = CONTROLLER_MODEL;
  doc["device"]["manufacturer"] = CONTROLLER_MANUFACTURER;
  doc["device"]["sw_version"] = CONTROLLER_SW_VERSION;
  return doc.as<String>();
}

#define SENSOR_PIN1 D3 
#define SENSOR_PIN2 D4
#define RELAIS_1 D5
#define RELAIS_2 D6
#define RELAIS_3 D7
#define RELAIS_4 D8

OneWire oneWire1(SENSOR_PIN1);
DallasTemperature sensors1(&oneWire1);

OneWire oneWire2(SENSOR_PIN2);
DallasTemperature sensors2(&oneWire2);

DeviceAddress POOL_VL = { 40, 255, 202, 76, 193, 23, 2, 227 };
DeviceAddress POOL_RL = { 40, 255, 57, 184, 195, 23, 4, 230 };
DeviceAddress POOL_IST = { 40, 255, 70, 248, 193, 23, 1, 116 };

//DeviceAddress WWA_VL = { 40, 255, 164, 74, 193, 23, 2, 89 };
//DeviceAddress WWA_RL = { 40, 255, 189, 191, 193, 23, 1, 196 };
DeviceAddress WWA_IST = { 40, 255, 208, 80, 193, 23, 2, 8 };

DeviceAddress SK_VL = { 40, 255, 159, 189, 193, 23, 1, 184 };
DeviceAddress SK_RL = { 40, 255, 7, 187, 193, 23, 1, 56 };
// ALTERNATIVE DeviceAddress SK_IST = { 40, 97, 100, 17, 178, 75, 200, 16 };
DeviceAddress SK_IST = { 40, 255, 86, 187, 193, 23, 1, 96 };

DeviceAddress WWH_RL = { 40, 255, 175, 191, 193, 23, 1, 59 };
DeviceAddress WWH_VL = { 40, 255, 80, 192, 193, 23, 1, 192 };
// ALTERNATIVE DeviceAddress WWH_IST = { 40, 97, 100, 17, 188, 122, 112, 253 };
DeviceAddress WWH_IST = { 40, 255, 76, 16, 194, 23, 1, 120 };

void setup_wifi() {
  delay(10);

  // Connecting to WiFi
  WiFi.hostname(CONTROLLER_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());

  Serial.printf("\nIP address: %s\n", WiFi.localIP().toString().c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("mode")){
    strcpy(mode, doc["mode"]);
  }
  if (doc.containsKey("priority")){
    strcpy(priority, doc["priority"]);
  }
  if (doc.containsKey("wwh_min")){
    wwh_min = doc["wwh_min"];
  }
  if (doc.containsKey("wwh_max")){
    wwh_max = doc["wwh_max"];
  }
  
}

// reconnect non-blocking
long lastReconnectAttempt = 0;

boolean reconnect() {
  if (mqttClient.connect(CONTROLLER_NAME, MQTT_USER, MQTT_PASS)) {
    // Once connected, publish an announcement...
    mqttClient.publish((mqtt_main_topic + "state").c_str(), "connected");
    // ... and resubscribe
    mqttClient.subscribe(mqtt_topic_cmd);
  }
  return mqttClient.connected();
}

void setup() {
  Serial.begin(115200);

  setup_wifi();

  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(1024);

  lastReconnectAttempt = 0;

  pinMode(RELAIS_1, OUTPUT);
  pinMode(RELAIS_2, OUTPUT);
  pinMode(RELAIS_3, OUTPUT);
  pinMode(RELAIS_4, OUTPUT);

  sensors1.begin();
  sensors2.begin();

  digitalWrite(RELAIS_1, HIGH);
  digitalWrite(RELAIS_2, HIGH);
  digitalWrite(RELAIS_3, HIGH);
  digitalWrite(RELAIS_4, HIGH);
   
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void loop()
{
  // If not connected, reconnect
  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();

    delay(2000);
    uptime = (millis()/1000);

    // Publish config to MQTT
    mqttClient.publish((mqtt_main_topic + "state").c_str(), "connected");
    mqttClient.publish((mqtt_main_topic + "config").c_str(), get_config().c_str(), true);

    // Read sensor values or use dummy values
    sensors1.requestTemperatures();
    sensors1.setResolution(11);
    Temp_POOL_VL = sensors1.getTempC(POOL_VL);
    Temp_POOL_RL = sensors1.getTempC(POOL_RL);
    Temp_POOL_IST = sensors1.getTempC(POOL_IST);
    Temp_WWA_IST = sensors1.getTempC(WWA_IST);
    Temp_SK_VL = sensors1.getTempC(SK_VL);
    Temp_SK_RL = sensors1.getTempC(SK_RL);
    Temp_SK_IST = sensors1.getTempC(SK_IST);
    // Temp_POOL_VL = 12;
    // Temp_POOL_RL = 8;
    // Temp_POOL_IST = 10;
    // Temp_WWA_IST = 15;
    // Temp_SK_VL = 16;
    // Temp_SK_RL = 11;
    // Temp_SK_IST = 14;

    delay(2000);
    // Read sensor values or use dummy values
    sensors2.requestTemperatures();
    sensors2.setResolution(11);
    Temp_WWH_VL = sensors2.getTempC(WWH_VL);
    Temp_WWH_RL = sensors2.getTempC(WWH_RL);
    Temp_WWH_IST = sensors2.getTempC(WWH_IST);
    // Temp_WWH_VL = 23;
    // Temp_WWH_RL = 19;
    // Temp_WWH_IST = 22;

    // Calculate temperature differences
    diff_wwh = Temp_SK_IST-Temp_WWH_IST;
    diff_pool = Temp_SK_IST-Temp_POOL_IST;
    diff_wwa = Temp_SK_IST-Temp_WWA_IST;

    // Hysteresis for WWH
    wwh_relais_state = digitalRead(RELAIS_1);
    // Check if relais state has changed
    if (wwh_relais_state != last_wwh_relais_state){
      // Check if relais state has changed from on to off or off to on
      if (wwh_relais_state == HIGH) {
        wwh_limit = wwh_min;
      }
      else {
        wwh_limit = wwh_max;
      }
    }
    last_wwh_relais_state = wwh_relais_state;

    if (strcmp(mode, "automatic") == 0){ 
      if (strcmp(priority, "normal") == 0){  // [Normal] Normalfall Temperaturgrenzen bestimmen, welche Pumpe laeuft
        if (Temp_WWH_IST < wwh_limit && diff_wwh > 8){
          digitalWrite(RELAIS_1, LOW);
          digitalWrite(RELAIS_2, HIGH);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_POOL_IST < 33 && diff_pool > 12){
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, LOW);
            digitalWrite(RELAIS_3, HIGH);
          }
          else {
            if (Temp_WWA_IST < 58 && diff_wwa > 14) {
              digitalWrite(RELAIS_1, HIGH);
              digitalWrite(RELAIS_2, HIGH);
              digitalWrite(RELAIS_3, LOW);
            }
            else {
              digitalWrite(RELAIS_1, HIGH);
              digitalWrite(RELAIS_2, HIGH);
              digitalWrite(RELAIS_3, HIGH);
            }
          }
        }
      }
      if (strcmp(priority, "pool") == 0){  // [Pool] WWH ist deaktiviert und auf Oelheizung, Automatic nur Pool und WWA
        if (Temp_POOL_IST < 33 && diff_pool > 9){
          digitalWrite(RELAIS_1, HIGH);
          digitalWrite(RELAIS_2, LOW);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_WWA_IST < 58 && diff_wwa > 9) {
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, HIGH);
            digitalWrite(RELAIS_3, LOW);
          }
          else {
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, HIGH);
            digitalWrite(RELAIS_3, HIGH);
          }
        }
      }
      if (strcmp(priority, "winter") == 0){  // [Winter] Pool ist still gelegt, WWA ist auf "Frostschutz"
        if (Temp_WWH_IST < wwh_limit && diff_wwh > 9){
          digitalWrite(RELAIS_1, LOW);
          digitalWrite(RELAIS_2, HIGH);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_WWA_IST < 58 && diff_wwa > 9) {
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, HIGH);
            digitalWrite(RELAIS_3, LOW);
          }
          else {
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, HIGH);
            digitalWrite(RELAIS_3, HIGH);
          }
        }
      }
      if (strcmp(priority, "wwaoff") == 0){  // obsolete(theoretisch) WWA ist still gelegt, Automatic nur Pool und WWH
        if (Temp_WWH_IST < wwh_limit && diff_wwh > 8){
          digitalWrite(RELAIS_1, LOW);
          digitalWrite(RELAIS_2, HIGH);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_POOL_IST < 33 && diff_pool > 12){
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, LOW);
            digitalWrite(RELAIS_3, HIGH);
          }
          else {
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, HIGH);
            digitalWrite(RELAIS_3, HIGH);
          }
        }
      }
    }

    // Publish temperature values to MQTT
    mqttClient.publish((mqtt_main_topic + "sensor").c_str(), get_temp().c_str());

    // Publish HA discovery info to MQTT
    mqttClient.publish((mqtt_discovery_topic_sensor + "wwh_vl/config").c_str(), set_ha_discovery_sensor("wwh_vl").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "wwh_rl/config").c_str(), set_ha_discovery_sensor("wwh_rl").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "wwh_ist/config").c_str(), set_ha_discovery_sensor("wwh_ist").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "pool_vl/config").c_str(), set_ha_discovery_sensor("pool_vl").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "pool_rl/config").c_str(), set_ha_discovery_sensor("pool_rl").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "pool_ist/config").c_str(), set_ha_discovery_sensor("pool_ist").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "sk_vl/config").c_str(), set_ha_discovery_sensor("sk_vl").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "sk_rl/config").c_str(), set_ha_discovery_sensor("sk_rl").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "sk_ist/config").c_str(), set_ha_discovery_sensor("sk_ist").c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "wwa_ist/config").c_str(), set_ha_discovery_sensor("wwa_ist").c_str());

    mqttClient.publish((mqtt_discovery_topic_sensor + "free_mem/config").c_str(), set_ha_discovery_free_mem().c_str());
    mqttClient.publish((mqtt_discovery_topic_sensor + "uptime/config").c_str(), set_ha_discovery_uptime().c_str());

    mqttClient.publish((mqtt_discovery_topic_select + "mode/config").c_str(), set_ha_discovery_mode().c_str());
    mqttClient.publish((mqtt_discovery_topic_select + "priority/config").c_str(), set_ha_discovery_priority().c_str());
    mqttClient.publish((mqtt_discovery_topic_number + "wwh_min/config").c_str(), set_ha_discovery_wwh_min().c_str());
    mqttClient.publish((mqtt_discovery_topic_number + "wwh_max/config").c_str(), set_ha_discovery_wwh_max().c_str());

    // Define LCD output headers
    lcd.clear();
    lcd.setCursor(4,0);
    lcd.print("WWH Pool WWA SK");
    lcd.setCursor(0,1);
    lcd.print("VL: ");
    lcd.setCursor(0,2);
    lcd.print("RL: ");
    lcd.setCursor(0,3);
    lcd.print("IST:");

    // Show mode and priority on LCD
    lcd.setCursor(0,0);
    if (strcmp(mode, "automatic") == 0) { 
      if (strcmp(priority, "normal") == 0) { 
        lcd.print("N");
        lcd.print(" "); 
        lcd.setCursor(3,0);
        lcd.print(" "); 
        lcd.setCursor(7,0);
        lcd.print(" ");
      }
      else {
        lcd.print("A");
      }
      
      lcd.setCursor(3,0);
      if (strcmp(priority, "winter") == 0) {
        lcd.print("@");
      }
      else {
        lcd.print(" ");
      }

      lcd.setCursor(7,0);
      if (strcmp(priority, "pool") == 0) {
        lcd.print("@");
      }
      else {
        lcd.print(" ");
      }

      lcd.setCursor(12,0);
      if (strcmp(priority, "wwaoff") == 0) {
        lcd.print("-");
      }
      else {
        lcd.print(" ");
      }
    }

    // Show temperature values on LCD (WWH)
    lcd.setCursor(5,1);
    lcd.print(Temp_WWH_VL,0);
    lcd.setCursor(5,2);
    lcd.print(Temp_WWH_RL,0);
    lcd.setCursor(5,3);
    lcd.print(Temp_WWH_IST,0);

    // Show temperature values on LCD (Pool)
    lcd.setCursor(9,1);
    lcd.print(Temp_POOL_VL,0);
    lcd.setCursor(9,2);
    lcd.print(Temp_POOL_RL,0);
    lcd.setCursor(9,3);
    lcd.print(Temp_POOL_IST,0);

    // Show temperature values on LCD (WWA)
    // lcd.setCursor(13,1);
    // lcd.print(Temp_WWA_VL,0);
    // lcd.setCursor(13,2);
    // lcd.print(Temp_WWA_RL,0);
    lcd.setCursor(13,3);
    lcd.print(Temp_WWA_IST,0);
    
    // Show temperature values on LCD (SK)
    lcd.setCursor(17,1);
    lcd.print(Temp_SK_VL,0);
    lcd.setCursor(17,2);
    lcd.print(Temp_SK_RL,0);
    lcd.setCursor(17,3);
    lcd.print(Temp_SK_IST,0);
  }
}