// Created by Ty Tower in June 11 2015
// Modified by S.Parada for ACROBOTIC Industries Oct 2015
// Based on DHTtester.ino By Adafruit
// Public domain

//  This code takes readings from a DHT22 sensor and posts them via Ubidots.com
//  For this exercise you will need to set up a free Ubidots account

#include <DHT.h>
#include <ESP8266WiFi.h>

#define errorPin 13
#define DHTPIN D2
#define DHTTYPE DHT22

// Instantiates and initializes the dht object
DHT dht(DHTPIN, DHTTYPE);

// Define and initialize constants and variables that we'll use later in the code
const int sleepTimeS = 20;  // Time to sleep (in seconds) between posts to Ubidots
long lastReadingTime = 0;
WiFiClient client;
char results[4];

// After creating an account on Ubidots, you'll be able to setup variables where you 
// will store the data. In order to post the measurements to the Ubidots variables,
// we need their "IDs", which are given on the website
String idvariable1 = "------your_temperature_variableID_here--------";
String idvariable2 = "------your_humidity_variableID_here----------";

// In addition, we'll need the API token, which is what prevents other users
// Ubidots to publish their data to one of your variables
String token = "---------your_token_goes_here------------";

//////////////////////////////////////////////////////////////////////////////////

// The setup function is executed once by the ESP8266 when it's powered up or reset
void setup()
{
  pinMode(errorPin, OUTPUT); // sets pin as an output to drive an LED for status indication
  const char* ssid = "YourWi-FiName";
  const char* password = "YourWi-FirPassword";
  // The following loop flashes the LED four times to indicate we're inside the setup function
  for (int i=0;i<4; i++)
  {
    digitalWrite(errorPin ,HIGH);
    delay(200);
    digitalWrite(errorPin ,LOW);
    delay(200);
  }

  // Create an instance of the server and specify the port to listen on as an argument
  Wi-FiServer server(80);

  // Initialize Serial (USB) communication, which will be used for sending debugging messages
  // to the computer
  Serial.begin(115200);
  
  // Start the communication with the DHT sensor by callibg the begin method of the dht object:W
  dht.begin();
  // Manual delay while the communication is started
  delay(10);

  // Debug messsages to indicate we're about to connect to the netowrk
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Use the scanNetworks method inside the Wi-Fi class to scan for any available Wi-Fi networks
  // nearby. If none are found, go to sleep
  int n = Wi-Fi.scanNetworks();

  Serial.println("scan done"); 
  if (n == 0){
    Serial.println("no networks found");
    Serial.println("Going into sleep");
// ESP.deepSleep(sleepTimeS * 1000000);
  }

  // If networks are found, attempt to connect to our Wi-Fi network
  Wi-Fi.begin(ssid, password);

  // While the connection is not established, IDLE inside this while loop
  while (Wi-Fi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Once the connection to our Wi-Fi netowrk is successful, print some debug messages
  Serial.println("");
  Serial.println("Wi-Fi connected");

  // And then start the server
  server.begin();
  
  Serial.println("Wi-Fi Server started");
  Serial.println(Wi-Fi.localIP());
}

////////////////////////////////////////////////////////////////////////////////

void loop(){
  // Read the current temperature and humidity measured by the sensor
  float temp = dht.readTemperature(true);
  float hum = dht.readHumidity();

  // Call our user-defined function ubiSave_value (defined below), and pass it both the 
  // measurements as well as the corresponding Ubidots variable IDs
  ubiSave_value(String(idvariable1), String(temp));
  ubiSave_value(String(idvariable2), String(hum));

  // Send some debug messages over USB
  Serial.println("Ubidots data");
  Serial.println("temperature: "+String(temp));
  Serial.println("humidity: "+String(hum));
  Serial.println(" Going to Sleep for a while !" );

  // deepSleep time is defined in microseconds. Multiply seconds by 1e6
  //ESP.deepSleep(sleepTimeS * 1000000);//one or other
  
  // Wait a few seconds before publishing additional data to avoid saturating the system
  delay(20000);  
}

////////////////////////////////////////////////////////////////////////////////

// We encapsulate the grunt work for publishing temperature and humidty values to Ubidots
// inside the function ubiSave_value
void ubiSave_value(String idvariable, String value) {

  // Prepare the value that we're to send to Ubidots and get the length of the entire string
  // that's being sent
  int num=0;
  String var = "{\"value\": " + String(value)+"}"; // We'll pass the data in JSON format
  num = var.length();

  // If we get a proper connection to the Ubidots API
  if (client.connect("things.ubidots.com", 80)) {
    Serial.println("connected ubidots");
    delay(100);

    // Construct the POST request that we'd like to issue
    client.println("POST /api/v1.6/variables/"+idvariable+"/values HTTP/1.1");
    // We also use the Serial terminal to show how the POST request looks like
    Serial.println("POST /api/v1.6/variables/"+idvariable+"/values HTTP/1.1");
    // Specify the contect type so it matches the format of the data (JSON)
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    // Specify the content length
    client.println("Content-Length: "+String(num));
    Serial.println("Content-Length: "+String(num));
    // Use our own API token so that we can actually publish the data
    client.println("X-Auth-Token: "+token);
    Serial.println("X-Auth-Token: "+token);
    // Specify the host
    client.println("Host: things.ubidots.com\n");
    Serial.println("Host: things.ubidots.com\n");
    // Send the actual data
    client.print(var);
    Serial.print(var+"\n");
  }
  else {
    // If we can't establish a connection to the server:
    Serial.println("Ubidots connection failed");
  }
  // If we've lost the connection to Wi-Fi
  if (!client.connected()) {
    Serial.println("NotConnected");
    Serial.println("disconnecting ubidots.");
    client.stop();
    // do nothing forevermore:
    for(;;);
  }
  // If our connection to Ubidots is healthy, read the response from Ubidots
  // and print it to our Serial Monitor for debugging!
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}
