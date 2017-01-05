// Created by MakerBro for ACROBOTIC Industries Aug 2016
// Public domain

// This code sends test data to the IFTTT Maker Channel to trigger
// an action such as sending email or SMS.
// For this exercise you will need to set up a free Ubidots account
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// Define and initialize constants and variables that we'll use later in the code
WiFiClientSecure client;

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

//////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void ifttt_post(String);

//////////////////////////////////////////////////////////////////////////////////
// IFTTT Constants //
// IFTTT destination server:
const char* IFTTTServer = "maker.ifttt.com";
// IFTTT https por:
const int httpsPort = 443;
// IFTTT Event:
const String MakerIFTTT_Event = "maker_channel_event";
// IFTTT private key:
const String MakerIFTTT_Key = "maker_channel_key";

String httpHeader = "POST /trigger/"+MakerIFTTT_Event+"/with/key/"+MakerIFTTT_Key+" HTTP/1.1\r\n"+
                    "Host: "+IFTTTServer+"\r\n" + 
                    "Content-Type: application/json\r\n";

// The setup function is executed once by the ESP8266 when it's powered up or reset
void setup()
{
  // Initialize Serial (USB) communication, which will be used for sending debugging messages
  // to the computer
  Serial.begin(115200);
  
  // Debug messsages to indicate we're about to connect to the network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // If networks are found, attempt to connect to our Wi-Fi network
  WiFi.begin(ssid, password);

  // While the connection is not established, IDLE inside this while loop
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Once the connection to our Wi-Fi netowrk is successful, print some debug messages
  Serial.println("");
  Serial.println("Wi-Fi connected");

  // Call our user-defined function ubiSave_value (defined below), and pass it both the 
  // measurements as well as the corresponding IFTTT variable IDs
  ifttt_post(String("DATA"));    
}

////////////////////////////////////////////////////////////////////////////////

void loop()
{
  // Wait a few seconds before publishing additional data to avoid saturating the system
  delay(20000);
}

////////////////////////////////////////////////////////////////////////////////

// We encapsulate the grunt work for publishing data values to IFTTT
// inside the function ubiSave_value
void ifttt_post(String value)
{ 
  if (client.connect(IFTTTServer, httpsPort) <= 0)
  {
    Serial.println("Failed to connect to server.");
    return;
  }
  Serial.println("Connected. Posting to IFTT Event!");

  // Some test data to send
  String params1, params2,params3;
  params1 = String(1);
  params2 = String(2);
  params3 = String(3);
  
  // We'll send the data in JSON format
  String data="{\"value1\":\""+params1+"\",\"value2\":\""+params2+"\",\"value3\":\""+params3+"\"}";
  String num = String(data.length());

  // Send the request to the server
  client.print(httpHeader);
  // Send the actual data
  client.print("Content-Length: "); 
  client.print(num);
  client.print("\r\n\r\n");
  client.print(data);

  // Wait until data is sent
  delay(1000);

  // If our connection to IFTTT is healthy, read the response and print 
  // it to our Serial Monitor for debugging!
  Serial.println("\nServer response (if any):");
  while(client.available())
  {
    char c = client.read();
    Serial.print(c);
  }
  Serial.println("---------------------------\n");
}
