/*------------------------------------------------------------------------------
  11/01/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: wemos_yt_tracker.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video where I build a little desktop display to show the 
  number of subscribers of a YouTube channel. I also display the difference in 
  subscribers over 24h by storing data in flash memory (SPIFFS).
  https://youtu.be/H8C6cpVGLFU

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
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <FS.h>

// Wi-Fi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Initialize display communication
U8G2_SSD1306_64X48_ER_F_HW_I2C display(U8G2_R2, 0);

// Client for requesting data from YT API
WiFiClientSecure client;

// YT API credentials
#define YT_API_KEY "YOUR_API_KEY" // Keep it secret (i.e., don't commit to Github!!!)
#define YT_CHANNEL_ID "YT_CHANNEL_ID" // e.g., UCqk4hT4XpzUVVUfsIDNzvPw (ACROBOTIC)
#define YT_URL "www.googleapis.com"

// Data processing variables
static char respBuffer[4096];
int subscriberCount = 0;

// Data storage variables
const int SAVE_INTERVAL = 6*60*24;
int oldSubscriberCount = 0, cycleCounter = 0;
File file;

void setup(void) {
  // Initialize file system
  SPIFFS.begin();

  // Initialize display
  display.begin();
  display.setFontPosTop();  

  // Connect to the internet
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    drawAnimation(counter++);
  }
  Serial.println();
  Serial.println(WiFi.localIP());

  // Get stored subscriber count
  file = SPIFFS.open("/subscribers.json", "r");
  DynamicJsonBuffer jBuffer;
  JsonObject* jObject;  
  if(file){
    String str;
    while (file.available()){
      str += char(file.read());
    }
    file.close();   
    jObject = &jBuffer.parseObject(str); //returns &jobject
  } else {
    jObject = &jBuffer.createObject();    
    jObject->set("subscriberCount",0);  
  }
  oldSubscriberCount = (*jObject)["subscriberCount"];
  file.close();
}

void loop(void) {
  getData();
  if(cycleCounter++ > SAVE_INTERVAL) {
    saveData();
    cycleCounter = 0;
  }
  drawData();
  delay(10000);
}

void drawAnimation(int counter) {
  display.clearBuffer();
  display.setFont(u8g2_font_5x8_mf);    
  display.drawStr(6,20,"Connecting");
  display.setFont(u8g2_font_m2icon_5_tf);      
  display.drawGlyph(20,34, counter % 3 == 0 ? 0x0053 : 0x0051);
  display.drawGlyph(28,34, counter % 3 == 1 ? 0x0053 : 0x0051);
  display.drawGlyph(36,34, counter % 3 == 2 ? 0x0053 : 0x0051);
  display.sendBuffer();
}

void getData() {
  const char request[] = 
    "GET /youtube/v3/channels?part=statistics&id=" YT_CHANNEL_ID "&key=" YT_API_KEY " HTTP/1.1\r\n"
    "User-Agent: ESP8266/0.1\r\n"
    "Accept: */*\r\n"
    "Host: " YT_URL "\r\n"
    "Connection: close\r\n"
    "\r\n";
  if (!client.connect(YT_URL, 443))
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
 
  char *json = strchr(respBuffer,'{');
  DynamicJsonBuffer jBufferIn, jBufferOut;
  JsonObject& jObject = jBufferIn.parseObject(json);
  subscriberCount = atoi(jObject["items"][0]["statistics"]["subscriberCount"]);
}

void saveData() {
  String str = "{\"subscriberCount\":"+String(subscriberCount)+"}";
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(str);
  file = SPIFFS.open("/subscribers.json", "w");
  if(file) {
    jObject.printTo(file);
  }
  oldSubscriberCount = subscriberCount;
  file.close();
}

void drawData()
{
  int diff = subscriberCount - oldSubscriberCount;
  display.clearBuffer();
  display.setFont(u8g2_font_micro_tr);  
  display.drawStr(0,0,"Subscribers");   
  
  display.setFont(u8g2_font_7x14B_tn);
  display.setCursor(35, 12);
  display.print(diff);  

  display.setFont(u8g2_font_open_iconic_arrow_2x_t);
  if(diff>=0)
    display.drawGlyph(0, 12, 0x0047);  // Up arrow
  else
    display.drawGlyph(0, 12, 0x0044);  // Down arrow
  
  display.setFont(u8g2_font_micro_tr);  
  display.drawStr(0,32,"Total:");   
  display.setCursor(26, 32);
  display.print(subscriberCount); 

  display.drawHLine(0,40,64);
  display.setFont(u8g2_font_m2icon_5_tf);  
  display.drawGlyph(24,42, 0x0053);   // Open
  display.drawGlyph(30,42, 0x0051);   // •
  display.drawGlyph(36,42, 0x0051);   // •
  display.sendBuffer();
}
