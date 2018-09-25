#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_MAX31865.h>
 
const char* ssid = "your network name";
const char* password =  "your network password";
const char* mqttServer = "your pi's ip address";
const int mqttPort = 1883;
const char* mqttUser = ""; // leave blank unless you setup an mqtt username
const char* mqttPassword = ""; // leave blank unless you setup a password
const char* mqttTopic = "temp1";
   
WiFiClient espClient;
PubSubClient client(espClient);
// Use software SPI:         CS, DI, DO, CLK
Adafruit_MAX31865 max1 = Adafruit_MAX31865(16, 5, 4, 0); //NodeMCU pins connected to: D0, D1, D2, D3

// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31865 max1 = Adafruit_MAX31865(14);

#define RREF 428 // The value of the Rref resistor. Use 430.0!
 
void setup() {
 
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }


  client.publish(mqttTopic, "MQTT Temp message received");
  client.subscribe(mqttTopic);
  max1.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  //client.publish("MQTTRREF", String(RREF).c_str(), true);
  //client.subscribe("MQTTRREF"); 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
 
void loop() {
  client.loop();
  uint16_t rtd = max1.readRTD();
  float Ctemp = max1.temperature(100, RREF);
  float Ftemp = max1.temperature(100, RREF)*9/5+32;
  //Serial.println(Ctemp);     //Celcius output
  //Serial.println(Ftemp);     //Farenheght output  
  client.publish(mqttTopic, String(Ftemp).c_str(), true);
  delay(100);
}
