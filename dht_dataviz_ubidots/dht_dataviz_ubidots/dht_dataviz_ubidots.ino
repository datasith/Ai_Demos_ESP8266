// Created by Ty Tower in June 11 2015
// Modified by S.Parada for ACROBOTIC Industries Oct 2015
// Based on DHTtester.ino By Adafruit
// Public domain

//  This code takes readings from a DHT22 sensor and posts them via Ubidots.com
//  For this exercise you will need to set up a free Ubidots account

#include <DHT.h>
#include <ESP8266WiFi.h>

#define errorPin 16
#define DHTPIN D2
#define DHTTYPE DHT22

// Instantiates and initializes the dht object
DHT dht(DHTPIN, DHTTYPE);

// Define and initialize constants and variables that we'll use later in the code
const int sleep_time = 20;  // Time to sleep (in seconds) between posts to Ubidots
WiFiClient client;

// After creating an account on Ubidots, you'll be able to setup variables where you 
// will store the data. In order to post the measurements to the Ubidots variables,
// we need their "IDs", which are given on the website
String variable_id1 = "variableId_temperature";
String variable_id2 = "variableId_humidity";

// In addition, we'll need the API token, which is what prevents other users
// Ubidots to publish their data to one of your variables
String token = "access_token";

// We'll also initialize the values for our Wi-Fi network
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

//////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void ubiSave_value(String, String);

// The setup function is executed once by the ESP8266 when it's powered up or reset
void setup()
{
  pinMode(errorPin, OUTPUT); // sets pin as an output to drive an LED for status indication
  // The following loop flashes the LED four times to indicate we're inside the setup function
  for (int i=0;i<4; i++)
  {
    digitalWrite(errorPin ,HIGH);
    delay(200);
    digitalWrite(errorPin ,LOW);
    delay(200);
  }

  // Initialize Serial (USB) communication, which will be used for sending debugging messages
  // to the computer
  Serial.begin(115200);
  
  // Start the communication with the DHT sensor by callibg the begin method of the dht object
  dht.begin();
  // Manual delay while the communication with the sensor starts
  delay(10);

  // Debug messsages to indicate we're about to connect to the network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Use the scanNetworks method inside the Wi-Fi class to scan for any available Wi-Fi networks
  // nearby. If none are found, go to sleep
  int n = WiFi.scanNetworks();

  Serial.println("scan done"); 
  if (n == 0)
  {
    Serial.println("no networks found");
    Serial.println("Going into sleep");
// ESP.deepSleep(sleep_time * 1000000);
  }

  // If networks are found, attempt to connect to our Wi-Fi network
  WiFi.begin(ssid, password);

  // While the connection is not established, IDLE inside this while loop
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Once the connection to our Wi-Fi netowrk is successful, print some debug messages
  Serial.println("");
  Serial.println("Wi-Fi connected");
}

////////////////////////////////////////////////////////////////////////////////
// Main code
void loop()
{
  // Read the current temperature and humidity measured by the sensor
  float temp = dht.readTemperature(true);
  float hum = dht.readHumidity();

  // Call our user-defined function ubiSave_value (defined below), and pass it both the 
  // measurements as well as the corresponding Ubidots variable IDs
  ubiSave_value(String(variable_id1), String(temp));
  ubiSave_value(String(variable_id2), String(hum));

  // Send some debug messages over USB
  Serial.println("Ubidots data");
  Serial.println("temperature: "+String(temp));
  Serial.println("humidity: "+String(hum));
  Serial.println(" Going to Sleep for a while !" );

  // deepSleep time is defined in microseconds. Multiply seconds by 1e6
  //ESP.deepSleep(sleep_time * 1000000);//one or other
  
  // Wait a few seconds before publishing additional data to avoid saturating the system
  delay(sleep_time*1000);  
}

////////////////////////////////////////////////////////////////////////////////
// User-defined functions
// We encapsulate the grunt work for publishing temperature and humidty values to Ubidots
// inside the function ubiSave_value
void ubiSave_value(String variable_id, String value)
{
  // Prepare the value that we're to send to Ubidots and get the length of the entire string
  // that's being sent
  String var = "{\"value\": " + value +"}"; // We'll pass the data in JSON format
  String length = String(var.length());

  // If we get a proper connection to the Ubidots API
  if (client.connect("things.ubidots.com", 80))
  {
    Serial.println("Connected to Ubidots...");
    delay(100);

    // Construct the POST request that we'd like to issue
    client.println("POST /api/v1.6/variables/"+variable_id+"/values HTTP/1.1");
    // We also use the Serial terminal to show how the POST request looks like
    Serial.println("POST /api/v1.6/variables/"+variable_id+"/values HTTP/1.1");
    // Specify the contect type so it matches the format of the data (JSON)
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    // Specify the content length
    client.println("Content-Length: "+ length);
    Serial.println("Content-Length: "+ length);
    // Use our own API token so that we can actually publish the data
    client.println("X-Auth-Token: "+ token);
    Serial.println("X-Auth-Token: "+ token);
    // Specify the host
    client.println("Host: things.ubidots.com\n");
    Serial.println("Host: things.ubidots.com\n");
    // Send the actual data
    client.print(var);
    Serial.print(var+"\n");
  }
  else
  {
    // If we can't establish a connection to the server:
    Serial.println("Ubidots connection failed...");
  }
  
  // If our connection to Ubidots is healthy, read the response from Ubidots
  // and print it to our Serial Monitor for debugging!
  while (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }
  
  // Done with this iteration, close the connection.
  if (client.connected())
  {
    Serial.println("Disconnecting from Ubidots...");
    client.stop();
  }
}
