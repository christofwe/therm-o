#include <string>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BH6
#include "secrets.h"

LiquidCrystal_I2C lcd(0x27,20,4);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
JsonDocument doc;

String mqtt_main_topic = "therm-o/";
const char* mqtt_topic_cmd = "therm-o/cmd";

char mode[] = "normal";
bool automatic = false;

float diffWWH = 0;
float diffPOOL = 0;
float diffWWA = 0;
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

int TGrenz_WWH = 50;
int WWHStatus = 0;
int lastWWHStatus =0 ;
unsigned long uptime=0;

String get_config(){
  JsonDocument doc;
  doc["mode"] = mode;
  doc["automatic"] = (automatic ? "true" : "false");
  doc["tgrenz_wwh"] = TGrenz_WWH;
  doc["uptime"] = uptime;
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
// neuer Tauchsensor DeviceAddress SK_IST = { 40, 97, 100, 17, 178, 75, 200, 16 };
DeviceAddress SK_IST = { 40, 255, 86, 187, 193, 23, 1, 96 };

DeviceAddress WWH_RL = { 40, 255, 175, 191, 193, 23, 1, 59 };
DeviceAddress WWH_VL = { 40, 255, 80, 192, 193, 23, 1, 192 };
// neue Adresse DeviceAddress WWH_IST = { 40, 97, 100, 17, 188, 122, 112, 253 };
DeviceAddress WWH_IST = { 40, 255, 76, 16, 194, 23, 1, 120 };

void setup_wifi() {
  delay(10);

  // We start by connecting to a WiFi network
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
  if (doc.containsKey("automatic")){
    automatic = doc["automatic"];
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

// loop non-blocking
void loop()
{
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
    // Client connected
    mqttClient.loop();

    delay(1000);
    Serial.println(String(ESP.getFreeHeap()));
    mqttClient.publish((mqtt_main_topic + "config").c_str(), get_config().c_str());

    uptime = (millis()/86400000);
    sensors1.requestTemperatures();
    sensors1.setResolution(11);

    // float Temp_POOL_VL = sensors1.getTempC(POOL_VL);
    // float Temp_POOL_RL = sensors1.getTempC(POOL_RL);
    // float Temp_POOL_IST = sensors1.getTempC(POOL_IST);
    // float Temp_WWA_IST = sensors1.getTempC(WWA_IST);
    // float Temp_SK_VL = sensors1.getTempC(SK_VL);
    // float Temp_SK_RL = sensors1.getTempC(SK_RL);
    // float Temp_SK_IST = sensors1.getTempC(SK_IST);
    float Temp_POOL_VL = 12;
    float Temp_POOL_RL = 8;
    float Temp_POOL_IST = 10;
    float Temp_WWA_IST = 15;
    float Temp_SK_VL = 16;
    float Temp_SK_RL = 11;
    float Temp_SK_IST = 14;


    delay(1000);
    sensors2.requestTemperatures();
    sensors2.setResolution(11);

    // float Temp_WWH_VL = sensors2.getTempC(WWH_VL);
    // float Temp_WWH_RL = sensors2.getTempC(WWH_RL);
    // float Temp_WWH_IST = sensors2.getTempC(WWH_IST);
    float Temp_WWH_VL = 23;
    float Temp_WWH_RL = 19;
    float Temp_WWH_IST = 22;

    diffWWH = Temp_SK_IST-Temp_WWH_IST;
    diffPOOL = Temp_SK_IST-Temp_POOL_IST;
    diffWWA = Temp_SK_IST-Temp_WWA_IST;
    
    // Eventuell nutzbar für Sensorausfall!!!
    // DeviceCount = sensors.getDeviceCount();
    // Serial.print(DeviceCount);


    // Hysterese;
    WWHStatus = digitalRead(RELAIS_1);
    if (WWHStatus != lastWWHStatus){
      //Wenn RELAIS_1 schaltet wird hier reingesprungen;
      if (WWHStatus == HIGH) {
        //Wenn RELAIS_1 von an nach aus wechselt gilt diese Grenztemperatur;
        TGrenz_WWH = 50;
      }
      else {
        //Wenn RELAIS_1 von aus nach an wechselt gilt diese Grenztemperatur;
        TGrenz_WWH = 58;
      }
    }
    lastWWHStatus = WWHStatus;


    if (automatic){ 
      if (strcmp(mode, "normal") == 0){  // [Normal] Normalfall Temperaturgrenzen bestimmen, welche Pumpe laeuft
        if (Temp_WWH_IST < TGrenz_WWH && diffWWH > 8){
          digitalWrite(RELAIS_1, LOW);
          digitalWrite(RELAIS_2, HIGH);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_POOL_IST < 33 && diffPOOL > 12){
            digitalWrite(RELAIS_1, HIGH);
            digitalWrite(RELAIS_2, LOW);
            digitalWrite(RELAIS_3, HIGH);
          }
          else {
            if (Temp_WWA_IST < 58 && diffWWA > 14) {
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
      if (strcmp(mode, "poolprio") == 0){  // [Poolprio] WWH ist deaktiviert und auf Oelheizung, Automatic nur Pool und WWA
        if (Temp_POOL_IST < 33 && diffPOOL > 9){
          digitalWrite(RELAIS_1, HIGH);
          digitalWrite(RELAIS_2, LOW);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_WWA_IST < 58 && diffWWA > 9) {
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
      if (strcmp(mode, "winter") == 0){  // [Winter] Pool ist still gelegt, WWA ist auf "Frostschutz"
        if (Temp_WWH_IST < TGrenz_WWH && diffWWH > 9){
          digitalWrite(RELAIS_1, LOW);
          digitalWrite(RELAIS_2, HIGH);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_WWA_IST < 58 && diffWWA > 9) {
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
      if (strcmp(mode, "wwaaus") == 0){  // obsolete(theoretisch) WWA ist still gelegt, Automatic nur Pool und WWH
        if (Temp_WWH_IST < TGrenz_WWH && diffWWH > 8){
          digitalWrite(RELAIS_1, LOW);
          digitalWrite(RELAIS_2, HIGH);
          digitalWrite(RELAIS_3, HIGH);
        }
        else {
          if (Temp_POOL_IST < 33 && diffPOOL > 12){
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

    // Publish to MQTT
    mqttClient.publish((mqtt_main_topic + "sensor").c_str(), get_temp().c_str());

    //Definition der Überschriften
    lcd.clear();
    lcd.print(uptime);
    lcd.setCursor(4,0);
    lcd.print("WWH Pool WWA SK");
    lcd.setCursor(0,1);
    lcd.print("VL: ");
    lcd.setCursor(0,2);
    lcd.print("RL: ");
    lcd.setCursor(0,3);
    lcd.print("IST:");
    if (strcmp(mode, "poolprio") == 0) {
      lcd.setCursor(3,0);
      lcd.print("*");
    }
    else {
    lcd.setCursor(3,0);
      lcd.print(" ");
    }
    if (strcmp(mode, "winter") == 0) {
      lcd.setCursor(7,0);
      lcd.print("*");
    }
    else {
      lcd.setCursor(7,0);
      lcd.print(" ");
    }
    if (strcmp(mode, "wwaaus") == 0) {
      lcd.setCursor(12,0);
      lcd.print("*");
    }
    else {
      lcd.setCursor(12,0);
      lcd.print(" ");
    }

    // Anzeige der Temperaturen Warmwasser Haus Kreislauf
    lcd.setCursor(5,1);
    lcd.print(Temp_WWH_VL,0);
    lcd.setCursor(5,2);
    lcd.print(Temp_WWH_RL,0);
    lcd.setCursor(5,3);
    lcd.print(Temp_WWH_IST,0);

    // Anzeige der Temperaturen Pool Kreislauf
    lcd.setCursor(9,1);
    lcd.print(Temp_POOL_VL,0);
    lcd.setCursor(9,2);
    lcd.print(Temp_POOL_RL,0);
    lcd.setCursor(9,3);
    lcd.print(Temp_POOL_IST,0);

    // Anzeige der Temperaturen Warmwasser Aussen Kreislauf
    // lcd.setCursor(13,1);
    // lcd.print(Temp_WWA_VL,0);
    // lcd.setCursor(13,2);
    // lcd.print(Temp_WWA_RL,0);
    lcd.setCursor(13,3);
    lcd.print(Temp_WWA_IST,0);
    
    // Anzeige der Temperaturen Solarkollektoren
    lcd.setCursor(17,1);
    lcd.print(Temp_SK_VL,0);
    lcd.setCursor(17,2);
    lcd.print(Temp_SK_RL,0);
    lcd.setCursor(17,3);
    lcd.print(Temp_SK_IST,0);
  }

}