/*------------------------------------------------------------------------------
  03/01/2018
  Author: Cisco • A C R O B O T I C 
  Platforms: ESP8266
  Language: C++/Arduino
  File: simple_btc_oled.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to use the Coindesk API to fetch the 
  current price of Bitcoin an display it on a 0.96" OLED over I2C.
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
#include <Wire.h>
#include "util.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define CD_API "/v1/bpi/currentprice.json"
#define CD_URL "api.coindesk.com"

static char respBuffer[4096];

WiFiClient client;

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

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

  Wire.begin();  
  displayInit();                // initialze OLED display
  displayClear();               // clear the display
  setTextXY(0,0);               // Set the cursor position to 0th page (row), 0th column
  displayString("BTC/USD:");
}

String data;

void loop()
{
  getData();
  printData(data);
  delay(10000);
}

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
  String tmp = usd["rate_float"];
  data = tmp;
  Serial.print("BTC (USD): $");
  Serial.println(data);
}

void printData(String data)
{
  setTextXY(2,0);
  displayString("$");

  setTextXY(2,1);
  char __data[sizeof(data)];
  data.toCharArray(__data, sizeof(__data));
  displayString(__data);
}

void sendData(unsigned char data)
{
  Wire.beginTransmission(I2C_ADDRESS);  // begin I2C transmission
  Wire.write(CMD_MODE_DATA);            // set mode: data
  Wire.write(data);
  Wire.endTransmission();               // stop I2C transmission
}

void sendCommand(unsigned char command)
{
  Wire.beginTransmission(I2C_ADDRESS);  // begin I2C communication
  Wire.write(CMD_MODE_COMMAND);         // set mode: command
  Wire.write(command);
  Wire.endTransmission();               // End I2C communication
}

void displayInit()
{
  sendCommand(CMD_DISPLAY_OFF); //DISPLAYOFF
  sendCommand(0x8D);            //CHARGEPUMP Charge pump setting
  sendCommand(0x14);            //CHARGEPUMP Charge pump enable
  sendCommand(0xA1);            //SEGREMAP   Mirror screen horizontally (A0)
  sendCommand(0xC8);            //COMSCANDEC Rotate screen vertically (C0)
}

void setTextXY(unsigned char row, unsigned char col)
{
    sendCommand(0xB0 + row);                //set page address
    sendCommand(0x00 + (8*col & 0x0F));     //set column lower address
    sendCommand(0x10 + ((8*col>>4)&0x0F));  //set column higher address
}

void displayString(const char *str)
{
    unsigned char i=0, j=0, c=0;
    while(str[i])
    {
      c = str[i++];
      if(c < 32 || c > 127) //Ignore non-printable ASCII characters. This can be modified for multilingual font.
      {
        c=' '; //Space
      }
      for(j=0;j<8;j++)
      {
         //read bytes from code memory
         sendData(pgm_read_byte(&BasicFont[c-32][j])); //font array starts at 0, ASCII starts at 32. Hence the translation
      }
    }
}

void displayClear()
{
  unsigned char i=0, j=0;
  sendCommand(CMD_DISPLAY_OFF);     //display off
  for(j=0;j<8;j++)
  {        
    setTextXY(j,0);                                                                            
    {   
      for(i=0;i<16;i++)  //clear all columns
      {   
        displayString((const char*)" ");         
      }   
    }   
  }
  sendCommand(CMD_DISPLAY_ON);     //display on
  setTextXY(0,0);   
}
