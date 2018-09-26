#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Streaming.h>
#include <PID_v1.h>

#define SLEEP_DELAY_IN_SECONDS  30
#define ONE_WIRE_BUS            D4      // DS18B20 data pin
#define heaterPin               D1      // SSR output pin

const char* ssid = "enter you netwok name here";
const char* password = "network password";

const char* mqtt_server = "raspberrypi";
const char* mqtt_username = "<MQTT_BROKER_USERNAME>";
const char* mqtt_password = "<MQTT_BROKER_PASSWORD>";
const char* mqtt_topic_1 = "readTemp";
const char* mqtt_topic_2 = "setTemp";
const char* mqtt_topic_3 = "errorMssg";

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
DeviceAddress tempSensor;

char temperatureString[6];
String varTemp = "0";
float fltVarTemp;

double Setpoint, Input, Output;
PID myPID(&Input, &Output, &Setpoint,500,1,20, DIRECT);


int WindowSize = 5000;
unsigned long windowStartTime;

void setup() {
  pinMode(heaterPin,OUTPUT);
  // setup serial port
  Serial.begin(115200);

  // setup WiFi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // setup OneWire bus
  DS18B20.begin();
     
  //PID setup
  Setpoint = 10;
  windowStartTime = millis();
  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetMode(AUTOMATIC);
}

void setup_wifi() {
  delay(10);
  // Start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char* topic, byte* payload, unsigned int length) {

 payload[length]='\0';
 varTemp = String((char*)payload);
 Serial.println(varTemp);

 Serial << "Message arrived [" << mqtt_topic_2 << "] " << endl;
 
 for (int i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  Serial.print(receivedChar);
  }
  Serial.println();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    //if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
    if (client.connect("ESP8266Client")) { 
      Serial.println("connected");
      client.subscribe(mqtt_topic_2);
    } else {
      Serial << "failed, rc=" << client.state() << " try again in 5 seconds" << endl;
      Serial.println("");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float getTemperature() {
  //Serial << "Requesting DS18B20 temperature..." << endl;
  float temp;
 while (DS18B20.getDeviceCount() == 0) {
    Serial.println("Temperature sensor disconnected");
    client.publish(mqtt_topic_3, "Temp sensor disconnected",1);    
    delay(1000);
  }
  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (getTemperature() == -127) {
    Serial.println("Temperature sensor disconnected");
    client.publish(mqtt_topic_3, "Temp sensor disconnected",1);
    digitalWrite(heaterPin,LOW);
    delay(1000);
  
  } else {
    
    float displayTemp = getTemperature();
    //Convert from celcius to farenheight
    displayTemp = displayTemp * 1.8 + 32;
    // convert temperature to a string with two digits before the comma and 2 digits for precision
    dtostrf(displayTemp, 2, 2, temperatureString);
    // send temperature to the serial console
    Serial << "Sensor read temperature:" << temperatureString << " F." << endl;
    // send temperature to the MQTT topic
    client.publish(mqtt_topic_1, temperatureString);
    //convert set temp received from mqtt, from string to double
    fltVarTemp = atof (varTemp.c_str());

    Serial << "Target temperature:" << fltVarTemp << " F." << endl; 
 
    Serial.println(); 

    Setpoint = fltVarTemp;
    Input = displayTemp;
    myPID.Compute();

    unsigned long now = millis();
    if(now - windowStartTime>WindowSize)
    { //time to shift the Relay Window
      windowStartTime += WindowSize;
    }
    if(Output > now - windowStartTime) digitalWrite(heaterPin,HIGH);
    else digitalWrite(heaterPin,LOW);
  
  } 
}
