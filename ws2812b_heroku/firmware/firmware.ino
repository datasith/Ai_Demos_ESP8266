/*------------------------------------------------------------------------------
  10/24/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: firmware.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to deploy an IoT app. It involves
  running a web server on a cloud service (in this case Heroku), and using
  websockets to get the input from a user—the color of an RGB LED—via a webpage 
  served by the web server. The selected color is sent to the ESP8266, which 
  is setup to act as a client on the websocket.
  https://youtu.be/lteGQrY5Yu4

  Do you like my videos? You can support the channel:
  https://patreon.com/acrobotic
  https://paypal.me/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 

  https://acrobotic.com/
  https://amazon.com/acrobotic
  ------------------------------------------------------------------------------
  License:
  Please see attached LICENSE.txt file for details.
------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>

// Connecting to the internet
char * ssid = "YOUR_SSID";
char * password = "YOUR_PASSWORD";

// Controlling the WS2812B
CRGB led = CRGB::Black;

// Setting up the websocket client
WebSocketsClient webSocket;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initializing the WS2812B communication
  FastLED.addLeds<WS2812B, D2, GRB>(&led, 1);
  setRgb(0,0,0);

  // Initializing the websocket communication
//  webSocket.begin("YOUR_COMPUTER_IP", 8000, "/subscribe");
  webSocket.begin("YOUR_HEROKU_APP_URL", 80, "/subscribe");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); 
}

void loop() {
  // put your main code here, to run repeatedly:
  webSocket.loop();

} 

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] Received text: %s\n", payload);
      DynamicJsonBuffer jBuffer;
      JsonObject &root = jBuffer.parseObject(payload);
      setRgb(root["r"],root["g"],root["b"]);
      break;
  }
}

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  led.r = r;
  led.g = g;
  led.b = b;
  FastLED.show();
}
