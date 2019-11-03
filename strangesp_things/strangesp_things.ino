#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "constants.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#define PIN D2

MDNSResponder mdns;
ESP8266WebServer server(80);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(26, PIN, NEO_GRB + NEO_KHZ800);


void setup()
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(115200);
  WiFi.begin(constants::ssid, constants::password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("strangesp-things", WiFi.localIP()))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", []()
  {
    server.send(200, "text/html", constants::webpage);
  });

  server.on("/process", HTTP_OPTIONS, processMessageOptions);
  server.on("/process", HTTP_POST, processMessage);

  // Start TCP (HTTP) server
  server.begin();

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop()
{
  server.handleClient();
}

void processMessageOptions()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "" );
}

void processMessage()
{
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");

  StaticJsonBuffer<200> jBuffer;
  JsonObject& jObject = jBuffer.parseObject(server.arg("plain"));
  if (!jObject.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  
  //jObject.printTo(Serial);
  String str = jObject["message"];
  str.toUpperCase();

  for (int i = 0; i < str.length(); ++i)
  {
    strip.clear();
    //Serial.println(str[i]);
    if(isSpace(str[i]));
    else if(isAlpha(str[i]))
    {
      int pixel = str[i]-'A';
      int color_index = random(0,255);
      strip.setPixelColor(pixel, Wheel(color_index));
    }
    strip.show();
    delay(1000);        
  }
  strip.clear();
  strip.show();
  server.send ( 200, "text/json", "{success:true}" );
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
