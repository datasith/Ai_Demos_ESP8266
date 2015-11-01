// Created by Ty Tower in June 11 2015
// Modified by S.Parada for Acrobotic Industries Oct 2015
//  Based on DHTtester.ino By Adafruit
//  Public domain


//  This code takes readings from a DHT22 sensor and posts them via Ubidots.com
//  For this exercise you will need to set up a free Ubidots 

#include "DHT.h"
#include <ESP8266WiFi.h>

#define errorPin 13
#define DHTPIN D2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int sleepTimeS = 20;// Time to sleep (in seconds):
long lastReadingTime = 0;
WiFiClient client;
char results[4];
String idvariable1 = "------your_temperature_variableID_here--------";
String idvariable2 = "------your_humidity_variableID_here----------";
String token = "---------your_token_goes_here------------";

//////////////////////////////////////////////////////////////////////////////////

void setup(){

  pinMode(errorPin, OUTPUT);
  const char* ssid = "YourRouterName";
  const char* password = "YourRouterPassword";

  for (int i=0;i<4; i++){   // let know we are working
    digitalWrite(errorPin ,HIGH);
    delay(200);
    digitalWrite(errorPin ,LOW);
    delay(200);
  }

  // Create an instance of the server
  // specify the port to listen on as an argument

  WiFiServer server(80);
  Serial.begin(115200);
  dht.begin();
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");

  if (n == 0){
    Serial.println("no networks found");
    Serial.println("Going into sleep");
// ESP.deepSleep(sleepTimeS * 1000000);
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("WiFi Server started");
  Serial.println(WiFi.localIP());


}

////////////////////////////////////////////////////////////////////////////////

void loop(){
  delay(2000);
  float temp = dht.readTemperature(true);
  float hum = dht.readHumidity(); 
  ubiSave_value(String(idvariable1), String(temp));
  ubiSave_value(String(idvariable2), String(hum));
  
  Serial.println("Ubidots data");
  Serial.println("temperature: "+String(temp));
  Serial.println("humidity: "+String(hum));
  Serial.println(" Going to Sleep for a while !" );

  // deepSleep time is defined in microseconds. Multiply seconds by 1e6
  //ESP.deepSleep(sleepTimeS * 1000000);//one or other
  delay(20000);
}

void ubiSave_value(String idvariable, String value) {
  // if you get a connection, report back via serial:
  int num=0;
  String var = "{\"value\": " + String(value)+"}";
  num = var.length();
  if (client.connect("things.ubidots.com", 80)) {
    Serial.println("connected ubidots");
    delay(100);
    client.println("POST /api/v1.6/variables/"+idvariable+"/values HTTP/1.1");
    Serial.println("POST /api/v1.6/variables/"+idvariable+"/values HTTP/1.1");
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    client.println("Content-Length: "+String(num));
    Serial.println("Content-Length: "+String(num));
    client.println("X-Auth-Token: "+token);
    Serial.println("X-Auth-Token: "+token);
    client.println("Host: things.ubidots.com\n");
    Serial.println("Host: things.ubidots.com\n");
    client.print(var);
    Serial.print(var+"\n");

  }
  else {
    // if you didn't get a connection to the server:
    Serial.println("ubidots connection failed");
  }

  if (!client.connected()) {
    Serial.println("NotConnected");
    Serial.println("disconnecting ubidots.");
    client.stop();
    // do nothing forevermore:
    for(;;);
  }

  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}
