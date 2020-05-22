/*------------------------------------------------------------------------------
  03/01/2018
  Author: Cisco • A C R O B O T I C 
  Platforms: ESP8266
  Language: C++/Arduino
  File: simple_btc_coindesk.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to use the Coindesk API to fetch the 
  current price of Bitcoin.
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
#include <ArduinoJson.h>

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define CD_API "/v1/bpi/currentprice.json"
#define CD_URL "api.coindesk.com"

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

WiFiClient client;

void getData()
{
  const char request[] = 
    "GET " CD_API " HTTP/1.1\r\n"
    "User-Agent: ESP8266/0.1\r\n"
    "Accept: */*\r\n"
    "Host: " CD_URL "\r\n"
    "Connection: close\r\n"
    "\r\n";    
  Serial.print("Requesting URL: ");
  Serial.println(CD_URL);
  
  delay(100);
  
  if (!client.connect(CD_URL, 80))
  {
    Serial.println("Connection failed");
    return;
  }

  client.print(request);
  client.flush();
  delay(1000);

  uint16_t index = 0;
  while(client.connected())
  {
    if(client.available())
    {
      respBuffer[index++] = client.read();
      delay(1);
    }
  }
  client.stop();
  char * json = strchr(respBuffer,'{');
  String json_str = String(json);
  
  ///////////////////////////////////////////
  // The Response Buffer currently (03.11.18)
  // contains a stray 'd' character that
  // corrupts the data. This removes it, but
  // shouldn't be necessary when the issue is 
  // fixed!
  uint16_t idx_d = json_str.lastIndexOf('d');
  json_str.remove(idx_d,3);
  ///////////////////////////////////////////
  
  Serial.println(json_str);
  
  DynamicJsonBuffer jBuffer;
  JsonObject& root = jBuffer.parseObject(json_str);

  Serial.println("JsonObject: ");
  root.prettyPrintTo(Serial);
  Serial.println();

  JsonObject& bpi = root["bpi"];
  JsonObject& usd = bpi["USD"];
  String rate_float = usd["rate_float"];
  Serial.print("BTC (USD): $");
  Serial.println(rate_float);
}
