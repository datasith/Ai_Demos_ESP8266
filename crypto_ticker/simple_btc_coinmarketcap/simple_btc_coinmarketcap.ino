/*------------------------------------------------------------------------------
  03/01/2018
  Author: Cisco • A C R O B O T I C 
  Platforms: ESP8266
  Language: C++/Arduino
  File: simple_btc_coinmarketcap.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to use the Coinmarketcap API to fetch 
  the current price of Bitcoin.
  ------------------------------------------------------------------------------
  Do you like my work? You can support me:
  https://patreon.com/acrobotic
  https://paypal.me/acrobotic
  https://buymeacoff.ee/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products and kits to help fund future Open-Source 
  projects like this! We'll always put our best effort in every project, and 
  release all our design files and code for you to use. 
  https://acrobotic.com/
  https://amazon.com/shops/acrobotic
  ------------------------------------------------------------------------------
  License:
  Please see attached LICENSE.txt file for details.
------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define CMC_API "/v1/ticker/bitcoin/"
#define CMC_URL "api.coinmarketcap.com"

const char* fingerprint = "EF 9D 44 BA 1A 91 4C 42 06 B1 6A 25 71 26 58 61 BA DA FA B9";
static char respBuffer[4096];

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  getData();
  delay(10000);
}

WiFiClientSecure client;

void getData()
{
  const char request[] = 
    "GET " CMC_API " HTTP/1.1\r\n"
    "User-Agent: ESP8266/0.1\r\n"
    "Accept: */*\r\n"
    "Host: " CMC_URL "\r\n"
    "Connection: close\r\n"
    "\r\n";
    
  Serial.print("Requesting URL: ");
  Serial.println(CMC_URL);
  
  delay(100);
  
  if (!client.connect(CMC_URL, 443))
  {
    Serial.println("Connection failed");
    return;
  }

  // Unsure why verification isn't working.
  if (client.verify(fingerprint, CMC_URL)) {
    Serial.println("Certificate matches");
  } else {
    Serial.println("Certificate doesn't match!");
  }
  
  client.print(request);
  client.flush();
  delay(1000);

  uint16_t index = 0;
  char c;
  while(client.connected())
  {
    if(client.available())
    {
      c = client.read();
      respBuffer[index++] = c;
      delay(1);
    }
  }
  client.stop();
  char * json = strchr(respBuffer,'{');
  
  Serial.println(json);
  
  DynamicJsonBuffer jBuffer;
  JsonObject& root = jBuffer.parseObject(json);

  Serial.println("JsonObject: ");
  root.prettyPrintTo(Serial);
  Serial.println();

  String price_usd = root["price_usd"];
  Serial.print("BTC (USD): $");
  Serial.println(price_usd);
}
