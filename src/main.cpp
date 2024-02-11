#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// const char* hostname = "{{HOSTNAME}}";

// const char* ssid = "{{WIFI_SSID}}";
// const char* password = "{{WIFI_PASSWD}}";

// const char* mqtt_server = "{{MQTT_IP}}";
// const char* mqtt_user = "{{MQTT_USER}}";
// const char* mqtt_pass = "{{MQTT_PASSWD}}";
// const char* mqtt_topic_cmd = "{{MQTT_TOPIC_CMD}}";
// const char* mqtt_topic_state = "{{MQTT_TOPIC_BASE}}";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

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
int Auto = 0;
int Auto1 = 0;
int Auto2 = 1;
int Auto3 = 0;
int TGrenz_WWH = 50;
int WWHStatus = 0;
int lastWWHStatus =0 ;
unsigned long timer=0;

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
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  Serial.println("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());

  Serial.printf("\nIP address: %s\n", WiFi.localIP().toString().c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print("Payload: ");
  for (int i = 0; i < (int)length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.print("Length:");
  Serial.print(length);
}

// reconnect non-blocking
long lastReconnectAttempt = 0;

boolean reconnect() {
  if (mqttClient.connect(hostname, mqtt_user, mqtt_pass)) {
    // Once connected, publish an announcement...
    mqttClient.publish(mqtt_topic_state,"connected");
    // ... and resubscribe
    mqttClient.subscribe(mqtt_topic_cmd);
  }
  return mqttClient.connected();
}

void setup() {
  Serial.begin(115200);

  setup_wifi();

  mqttClient.setServer(mqtt_server, 1883);
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
    timer = (millis()/86400000);
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


    // schaltung();
    if (Auto == 0){
      if (Auto1 == 0 && Auto2 == 0 && Auto3 == 0){
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
      if (Auto1 == 1 && Auto2 == 0 && Auto3 == 0){
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
      if (Auto1 == 0 && Auto2 == 1 && Auto3 == 0){
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
      if (Auto1 == 0 && Auto2 == 0 && Auto3 == 1){
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


    //Definition der Überschriften
    lcd.clear();
    lcd.print(timer);
    lcd.setCursor(4,0);
    lcd.print("WWH Pool WWA SK");
    lcd.setCursor(0,1);
    lcd.print("VL: ");
    lcd.setCursor(0,2);
    lcd.print("RL: ");
    lcd.setCursor(0,3);
    lcd.print("IST:");
    if (Auto1 == 1) {
      lcd.setCursor(3,0);
      lcd.print("*");
    }
    else {
    lcd.setCursor(3,0);
      lcd.print(" ");
    }
    if (Auto2 == 1) {
      lcd.setCursor(7,0);
      lcd.print("*");
    }
    else {
      lcd.setCursor(7,0);
      lcd.print(" ");
    }
    if (Auto3 == 1) {
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