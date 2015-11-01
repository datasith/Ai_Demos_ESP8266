# Ai_Demos_ESP8266

Example code for using the ESP8266 Development Board and a DHT-22 Temperature
and Humidity sensor for building a web-connected weather station.  We set up a
widget on ubidots.com for uploading and visualizing the data collected.

## Description

For more details, check out the tutorial page at:

   * http://learn.acrobotic.com/tutorials/post/esp8266-iot-weather-station

Developed by S.Parada for ACROBOTIC Industries.  Please consider buying 
products from us to help fund future Open-Source projects like this! We'll
always put our best effort in every project, and release all our design 
files and code for you to use. 

## Installation

After cloning the repository you'll need to follow these steps to get your IoT
Wather Station up and running:

1. Initialize the included submodule by entering the command:
git submodule update --init -- iot_weather_station/lib/Adafruit_DHT_Library/

2. Install the Adafruit_DHT_Library to your Arduino IDE:
The library is included inside the lib subdirectory.  You can copy/move the
Adafruit_DHT_Library to your Arduino libraries directory.  Alternatively, the
most recent versions of Arduino IDE allow you to install the library directly
through the menu options:

Sketch > Include Library > Manage Libraries...

And selecting "DHT sensor library" from the options.  Note that an internet
connection is needed for following this method.

3. After the DHT library is installed, we'll need to make sure that our ESP8266
is recognized by the Arduino IDE.  The gist of it involves goint through the
menu options:

Arduino > Preferences

And adding to the field "Additional Boards Manager URLs" the URL:

http://arduino.esp8266.com/stable/package_esp8266com_index.json

Note that if you alreday have an URL in this field, you can add additional ones
by using a comma to separate them!

Then, you can install the tools needed by the Arduino IDE to support the ESP8266
Development Board by going through the menu options:

Tools > Board > Boards Manager...

And selecting the esp8266 by ESP8266 Community option, and clicking the
"Install" button.  After restarting the Arduino IDE, everything should ge set.
For more details please check:

http://learn.acrobotic.com/tutorials/post/esp8266-getting-started

4. You can now open the ot_weather_station.ino/iot_weather_station.ino file and
upload it to your ESP8266 Development Board.  Remember to select the board
option "NodeMCU 1.0 (ESP-12E Module)".

## License

Released under the MIT license. Please check LICENSE.txt for more information. 
All text above must be included in any redistribution.
