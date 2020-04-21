/*------------------------------------------------------------------------------
  04/10/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: webserver_rgb_websockets.ino
  ------------------------------------------------------------------------------
  Description:
  Code for YouTube video demonstrating how to use analog voltages to control the
  background color of a webpage. The voltages are read with an ADS1115 4-Channel
  ADC and the server-client communication takes place over a websocket:
  https://youtu.be/kynSxSl0uKY

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
#include <WebSocketsServer.h>
#include <Adafruit_ADS1015.h>
#include <Ticker.h>

// instantiate server objects
ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

// define program variables
char* ssid = "BEARS";
char* password = "tenonezero";
bool read_data = false;

// declare a timer variable
Ticker timer;

// declare a variable for the ADS1115
Adafruit_ADS1115 ads;

// raw string literal for containing the page to be sent to clients
char webpage[] PROGMEM = R"=====(
<html>
<head>
  <script>
    var Socket;
    function RGBToHex(r,g,b) {
      // format 3 number values (0~255) to #NNNNNN (hex)
      r = r.toString(16);
      g = g.toString(16);
      b = b.toString(16);
      if(r.length == 1) 
        r = "0" + r;
      if(g.length == 1) 
        g = "0" + g;
      if(b.length == 1) 
        b = "0" + b;                
      return "#" + r + g + b;
    }
    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.onmessage = function(event){
        // receive the color data from the server
        var data = JSON.parse(event.data);
        console.log(data);
        // change the color of the body's background using the data
        document.body.style.background = RGBToHex(data['red'],data['green'],data['blue']);
      }
    }  
  </script>
</head>
<body onload="javascript:init()">
  <!-- EMPTY BODY -->
</body>
</html>
)=====";

void setup()
{
  // connect to the local wi-fi
  WiFi.begin(ssid,password);
  Serial.begin(115200);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // define the routes in which the server is accessible
  server.on("/",[](){
    server.send_P(200, "text/html", webpage);  
  });
  server.begin();
  webSocket.begin();
  // initialize ADC and timer function
  timer.attach(/*rate in secs*/ 0.1, /*callback*/ readData);
  ads.begin();
  // not planning on receiving any data from the client
  // webSocket.onEvent(webSocketEvent);
}

void readData() {
  // should only be used to set/unset flags
  read_data = true;
}

void loop()
{
  webSocket.loop();
  server.handleClient();
  if(read_data){
    // get the measurements from the 3 ADC channels
    uint16_t adc0 = ads.readADC_SingleEnded(0);
    // scale the data down to the [0, 255] range
    adc0 = adc0 >> 6; // original range i have is 0–16384 despite being a 16-bit measurement
    uint16_t adc1 = ads.readADC_SingleEnded(1) >> 6;
    uint16_t adc2 = ads.readADC_SingleEnded(2) >> 6;
    // build the JSON-formatted string for sending over the websocket
    String json = "{\"red\":";
    json += adc0;
    json += ",\"green\":";
    json += adc1; 
    json += ",\"blue\":";
    json += adc2;        
    json += "}";
    Serial.println(json); // DEBUGGING
    webSocket.broadcastTXT(json.c_str(), json.length());
    // reset the flag
    read_data = false;
  }
}
