//LIBRARIES ======================
#include <Time.h>
#include <TimeLib.h>
#include <Ticker.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <DS1307.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>

//DEFINES AND FOWARDS DECLARATIONS ======================

#define routeToPost "/post/log/tomada/"
#define serial "sensorCorrente"
#define timeToPost 20
#define power 220 //220 V
void flagPost();

//OBJECTS ======================

StaticJsonBuffer<100> jsonBuffer; //parameters library ArduinoJson
SocketIOClient socket; //Instance library SocketIOClient
DS1307 rtc(4, 5); //Instance library rtc
Time temp; //Instance library Time 
JsonObject& root = jsonBuffer.createObject();
char dateBuffer[30]; //by default of some library
String ipStr; //by default of some library

//VARIABLES ======================

char host[] = "api.saiot.ect.ufrn.br"; //send to host
int port = 80; //send to port
extern String RID; //Constants of SocketIOClient
extern String Rname;
extern String Rcontent;
int pinSensor = A0; //Sensor's pin at ESP8266
int sensorValueI = 0; //Sersor's value data i of n  
double sensorValueAcc = 0; //Sensor's value accumulated 
float sensorValueAcc = 0; //Current Value
float sensibility = 0.185; //mv/A in/out ratio of the sensor
float voltsPerBit = 0.00329; //Minimal fluctuation of voltage to add a unity in the ADC
int nData; //Number of data sampled 
double timeBegin, timeEnd; //store the time that begun and finished the data sampling. 
int year, month, day, hour, minute, second; //store the time request (see time.ino)
bool stopGettingData = false; //flag that ends the sampling (ver post.ino)
double loadPower; //power consumed by the load in the sampling time

//SETUP ======================

void setup() {

  Serial.begin(115200);
  pinMode(pinSensor, INPUT_PULLUP);
  sending.attach(timeToPost, flagPost); //interruption: each timeToPost seconds the function flagToPost is called 
  delay(10);
  WiFiManager wifis; 
  wifis.autoConnect();
  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  if (!socket.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
}

//LOOP ======================

void loop() {
  socket.monitor();
  nData = 0; //resetting number of samples
  double timeBegin = millis();
  while(!stopGettingData){
    sensorValueI = analogRead(pinSensor); //read value in the analogic pin 
    sensorValueI = map(sensorValueI, 1, 775, 1, 512); //manual conversion (see README.md)
    sensorValueI -= 511; //offset (see README)
    sensorValueAcc += sensorValueI * sensorValueI; //sum of the data' squares
    delay(10);
    nData++; //counting number of samples
  }
  double timeEnd = millis();

  loadPower = rms(sensorValueAcc, timeEnd-timeBegin);
  postIt(loadPower);
}
