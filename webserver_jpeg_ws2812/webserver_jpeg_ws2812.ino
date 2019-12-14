/*------------------------------------------------------------------------------
  03/11/2019
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: webserver_jpeg_ws2812.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to drag and drop files to the File 
  System (SPIFFS) and display them on a WS2812/NeoPixel LED matrix.
  https://youtu.be/u_C7robY118

  Do you like my videos? You can support the channel:
  https://patreon.com/acrobotic
  https://paypal.me/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 

  https://acrobotic.com/
  https://amazon.com/shops/acrobotic
  ------------------------------------------------------------------------------
  License:
  Please see attached LICENSE.txt file for details.
------------------------------------------------------------------------------*/
// NOTES:
// - Ensure flashing the code with space for the SPIFFS
// - curl -F "file=@$PWD/index.html" 192.168.1.18/upload
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <JPEGDecoder.h>

#define FS_NO_GLOBALS //avoids conflicts with JPEGDecoder
#include <FS.h>

#include <Adafruit_NeoPixel.h>

// Map the serpentile-style matrix to the NeoPixel format
const uint8_t remap[16][16] = {
  {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
  {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
  {47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32},
  {48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63},
  {79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64},
  {80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95},
  {111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96},
  {112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127},
  {143,142,141,140,139,138,137,136,135,134,133,132,131,130,129,128},
  {144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159},
  {175,174,173,172,171,170,169,168,167,166,165,164,163,162,161,160},
  {176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191},
  {207,206,205,204,203,202,201,200,199,198,197,196,195,194,193,192},
  {208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223},
  {239,238,237,236,235,234,233,232,231,230,229,228,227,226,225,224},
  {240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}  
};

// Define LED matrix object
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(256, D4, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server;
char* ssid = "YOUR_SSID";
char* password = "YOUR_PASSWORD";

// hold uploaded file
fs::File fsUploadFile;

void setup()
{
  SPIFFS.begin();
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

  // list available files
  server.on("/list", HTTP_GET, handleFileList);
  // delete file
  server.on("/delete", HTTP_DELETE, handleFileDelete);
  // handle file upload
  server.on("/upload", HTTP_POST, [](){
    server.send(200, "text/plain", "{\"success\":1}");
  }, handleFileUpload);
  // called when the url is not defined
  server.onNotFound([](){
    if(!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "File Not Found!");
    }
  });
  server.begin();
  pixels.begin();
  pixels.setBrightness(128);
  pixels.show(); // initializes pixel matrix to off
}

void loop()
{
  server.handleClient();
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  return "text/plain";
}

void displayJpegMatrix(String path) {
  if(JpegDec.decodeFsFile(path)){
    uint32_t mcuPixels = JpegDec.MCUWidth * JpegDec.MCUHeight;
    uint8_t row = 0; uint8_t col = 0;

    while(JpegDec.read()){
      uint16_t *pImg = JpegDec.pImage;
      for(uint8_t i=0; i < mcuPixels; i++){
        // Extract the red, green, blue values from each pixel
        uint8_t b = uint8_t((*pImg & 0x001F)<<3); // 5 LSB for blue
        uint8_t g = uint8_t((*pImg & 0x07C0)>>3); // 6 'middle' bits for green
        uint8_t r = uint8_t((*pImg++ & 0xF800)>>8); // 5 MSB for red
        // Calculate the matrix index (column and row)
        col = JpegDec.MCUx*8 + i%8;
        row = JpegDec.MCUy*8 + i/8;
        // Set the matrix pixel to the RGB value
        pixels.setPixelColor(remap[row][col], pixels.Color(r,g,b));
      }
    }
    pixels.show();
  }
}

void handleFileDelete() {
  // make sure we get a file name as a URL argument
  if(server.args()==0) return server.send(500, "text/plain", "BAD ARGS!");
  String path = server.arg(0);
  // protect root path
  if(path == "/") return server.send(500, "text/plain", "BAD PATH!");
  // check if the file exists
  if(!SPIFFS.exists(path)) return server.send(404, "text/plain", "FILE NOT FOUND!");
  SPIFFS.remove(path);
  Serial.println("DELETE: " + path);
  String msg = "deleted file: " + path;
  server.send(200, "text/plain", msg);
}

bool handleFileRead(String path) {
  // Serve index file when top root path is accessed
  if(path.endsWith("/")) path += "index.html";
  // Different file types require different actions
  String contentType = getContentType(path);

  if(SPIFFS.exists(path)) {
    fs::File file = SPIFFS.open(path, "r");
    if (contentType == "image/jpeg")
      // display the image on the LED matrix
      displayJpegMatrix(path);
    // Display the file on the client's browser
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false; // if the file doesn't exist or can't be opened
}

void handleFileUpload()
{
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if(!filename.startsWith("/"))
      filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
  } else if(upload.status == UPLOAD_FILE_WRITE)
  {
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END)
  {
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileList()
{
  String path = "/";
  // Assuming there are no subdirectories
  fs::Dir dir = SPIFFS.openDir(path);
  String output = "[";
  while(dir.next())
  {
    fs::File entry = dir.openFile("r");
    // Separate by comma if there are multiple files
    if(output != "[")
      output += ",";
    output += String(entry.name()).substring(1);
    entry.close();
  }
  output += "]";
  server.send(200, "text/plain", output);
}
