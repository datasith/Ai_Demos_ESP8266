/*------------------------------------------------------------------------------
  03/01/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: simple_wug_oled.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to use the Weather Underground API to
  fetch the current weather data of a specific location:
  https://youtube.com/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 
  https://acrobotic.com/
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

#define WU_API_KEY "YOUR_WU_API_KEY"
#define WU_LOCATION "Australia/Sydney"
#define WU_URL "api.wunderground.com"

WiFiClient client;
static char respBuffer[4096];

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

  Wire.begin();  
  displayInit();                // initialze OLED display
  displayClear();               // clear the display
  setTextXY(0,0);               // Set the cursor position to 0th page (row), 0th column
  displayString("DATA:");  

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
    "GET /api/" WU_API_KEY "/conditions/q/" WU_LOCATION ".json HTTP/1.1\r\n"
    "User-Agent: ESP8266/0.1\r\n"
    "Accept: */*\r\n"
    "Host: " WU_URL "\r\n"
    "Connection: close\r\n"
    "\r\n";
  Serial.println(request);
  client.connect(WU_URL, 80);
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

  DynamicJsonBuffer jBuffer;
  JsonObject& root = jBuffer.parseObject(json);
  JsonObject& current = root["current_observation"];
  String temp_c = current["temp_c"];
  String weather = current["weather"];
  data = "Temp(C): "+temp_c;//+"\nWeather: "+weather;
  Serial.println(data);
}

void printData(String data)
{
  setTextXY(2,0);
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
