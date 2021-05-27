#include "MQTT.h"
#include "PubSubClient.h"
#include "NTPClient.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>


const char* ssid = "*****";
const char* pass = "************";
const char* mqtt_server = "************";
const int mqtt_port = ****;
const char* mqtt_user = "*********";
const char* mqtt_pass = "*********";

const int car_green = D1, car_red = D2, car_yellow = D6,
ped_green = D3, ped_red = D4, button = D5;
const int first_interval = 8000;
const int second_interval = 13000;
const int size = 5;
const int size_2nd = 2;
const int led[size] = { car_green, car_yellow, car_red, ped_green, ped_red };
const int go_car[size_2nd] = { car_green, ped_red };
const int go_ped[size_2nd] = { car_red, ped_green };
bool led_state = 1, butt = 0, butt_flag = 0, state = 0;
int mode = 0;
String day, month, year, message_to_web;

unsigned int last_press = 0;
unsigned int prev_time = 0;
const unsigned int rattling = 50;
const unsigned int cur_time = 5000;

void callback(const MQTT::Publish& pub)
{
  Serial.print(pub.topic());
  Serial.print(" => ");
  Serial.println(pub.payload_string());

  String payload = pub.payload_string();

  if (String(pub.topic()) == "test/button")
  {
    int tmp = payload.toInt();
    mode = tmp;
  }
}

WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);
ESP8266WebServer HTTP(80);
WiFiUDP ntpUDP;
NTPClient  time_client(ntpUDP, "pool.ntp.org", 7200, 60000);

void setup() {
  Serial.begin(9600);
  for (auto i : led) pinMode(i, OUTPUT);
  for (auto i : led) digitalWrite(i, LOW);
  pinMode(button, INPUT);
  prev_time = millis();
  last_press = millis();
  for (auto i : go_car) digitalWrite(i, HIGH);
  HTTP.onNotFound(handleNotFound);
  HTTP.on("/", handleRoot);
  HTTP.begin();
  time_client.begin();
}

void loop() {
  time_client.update();
  unsigned long epoch_time = time_client.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epoch_time);
  
  day = ptm->tm_mday;
  month = ptm -> tm_mon + 1;
  year = ptm-> tm_year+1900;
  
  HTTP.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) return;
    Serial.println("WiFi connected");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("arduinoClient2").set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
        client.subscribe("test/button");
      }
      else Serial.println("Could not connect to MQTT server");
      
    }
    if (client.connected()) client.loop();
  }

  butt = digitalRead(button);

  if (butt == 1 && butt_flag == 0 && millis() - last_press > rattling) {
    butt_flag = 1;
    last_press = millis();
  }

  if (butt == 0 && butt_flag == 1 && millis() - last_press > rattling) {
    butt_flag = 0;
    if (millis() - prev_time >= cur_time) {
      message_to_web += year + "-" + month + "-" + day + " " + time_client.getFormattedTime() + "\n";
      handleRoot();
      traffic_light(mode);
      prev_time = millis();
    }
    last_press = millis();
  }
}

void traffic_light(int x) {
  if (x == 0) {
    delay(5000);

    digitalWrite(car_green, LOW);
    digitalWrite(car_yellow, HIGH);
    delay(2000);
    digitalWrite(car_yellow, LOW);
    digitalWrite(ped_red, LOW);
    digitalWrite(car_red, HIGH);
    digitalWrite(ped_green, HIGH);

    delay(5000);

    
    digitalWrite(car_yellow, HIGH);
    delay(2000);
    digitalWrite(car_red, LOW);
    digitalWrite(car_yellow, LOW);
    digitalWrite(ped_green, LOW);
    digitalWrite(car_green, HIGH);
    digitalWrite(ped_red, HIGH);
  }
  else if (x == 1) {
    delay(5000);

    for (int i = 0; i < 3; i++) {
      for (auto i : go_car) digitalWrite(i, LOW);
      delay(400);
      for (auto i : go_car) digitalWrite(i, HIGH);
      delay(400);
    }

    for (auto i : go_car) digitalWrite(i, LOW);
    for (auto i : go_ped) digitalWrite(i, HIGH);

    delay(5000);

    for (int i = 0; i < 3; i++) {
      for (auto i : go_ped) digitalWrite(i, LOW);
      delay(400);
      for (auto i : go_ped) digitalWrite(i, HIGH);
      delay(400);
    }

    for (auto i : go_ped) digitalWrite(i, LOW);
    for (auto i : go_car) digitalWrite(i, HIGH);
  }
}

void handleNotFound(){
  String message = "File not found\n\n";
  HTTP.send(404, "text/plain", message);
}

void handleRoot() {HTTP.send(200, "text/plain", message_to_web);}

