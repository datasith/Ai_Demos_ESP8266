/*------------------------------------------------------------------------------
  11/29/2019
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: epd_showerthoughts.ino
  ------------------------------------------------------------------------------
  Description:
  Code for YouTube video demonstrating how to build an e-Paper Display picture 
  frame controlled by an ESP8266, and use it to display funny online quotes
  scraped from reddit (/r/showerthoughts):
  https://youtu.be/NFej0Jlwgxk
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
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include "Bitmaps.h"

// We won't need the GFX base class so disable it
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Instantiate the GxEPD2_BW class for our display type
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(SS, 4, 5, 16));

// Define WiFi and web server variables
ESP8266WebServer server;
char* ssid = "YOUR_SSID";
char* password = "YOUR_PASSWORD";

// Instantiate the WiFiClientSecure class to use it for creating a TLS connection
WiFiClientSecure client;

// Define web client constants
const char* host = "www.reddit.com";
const int httpsPort = 443;
const char fingerprint[] = "E3 C0 F1 CF CB A4 61 09 02 1A 74 06 71 83 CD A8 59 28 B4 0D";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  delay(2000);
  // Initialize communication with the display
  display.init(115200);
  // Draw the ACROBOTIC logo on the display
  drawBitmaps(ACROBOTIC_LOGO);

  // Configure the web server
  server.on("/", getShowerthought);
  server.begin();

  // Configure the web client
  client.setFingerprint(fingerprint);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}

void drawBitmaps(const unsigned char *bitmap) {
  // Configure the display according to our preferences
  display.setRotation(0);
  display.setFullWindow();
  // Display the bitmap image
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, bitmap, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_BLACK);
  } while(display.nextPage());
}

void getShowerthought() {
  // Connect to reddit.com and fetch the showerthought data using the web client
  if(!client.connect(host, httpsPort)) {
    Serial.println("connection failed!");
    return;
  }
  String url = "/r/Showerthoughts/top.json?sort=top&limit=1&t=day";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266/0.1\r\n" + 
               "Connection: close\r\n\r\n");
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  // Get the response from the reddit.com server
  String quote;
  while(client.connected()) {
    if(client.available()) {
      quote = client.readString();
      break;
    }
  }
  client.stop();
  // As the JSON-formatted response text can be too long, we'll parse it manually instead of using ArduinoJson
  int quote_start = quote.indexOf("\"title\"");
  // The showerthought quote ends right before a ', "' substring
  int quote_end = quote.indexOf(", \"", quote_start+1); // we start the search from the position where "title" is
  String showerthought = quote.substring(quote_start+9, quote_end);
  // Sanitize the string a bit
  showerthought.replace("\\\"","'"); // gets rid of escaped quotes in the text ('\"')
  // Display the showerthought on the EPD
  displayData(showerthought);

  // Don't leave the server hanging
  server.send(200, "text/plain", showerthought);
}

void displayData(String data) {
  // Display the showerthoughts logo
  drawBitmaps(SHOWERTHOUGHTS_LOGO);
  // Configure the display for displaying text
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  // Make sure the text is centered and doesn't overlap with the logo!
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(data, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = (display.width() - tbw) / 2;
  uint16_t y = (display.height() - tbh) / 2; // y is the base line!
  // Display the showerthougt
  do {
    display.setCursor(x, y);
    display.print(data);
  } while(display.nextPage());
  // Conserve power
  display.powerOff();
}
