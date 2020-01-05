/*------------------------------------------------------------------------------
  12/31/2019
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: webserver_jpeg_animation_ws2812.ino
  ------------------------------------------------------------------------------
  Description:
  Code for YouTube video demonstrating how to drag and drop files to the File
  System (SPIFFS) and display them on a WS2812/NeoPixel LED matrix. Added
  features allow us to display all .jpeg files in flash memory sequentially until
  the web server receives a request to stop.
  https://youtu.be/3lQ7_aotw1s

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
// - curl -F "file=@$PWD/index.html" 192.168.1.XX/upload
// - curl -X DELETE 192.168.1.XX/delete?file=/filename.extension
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <JPEGDecoder.h>

#define FS_NO_GLOBALS //avoids conflicts with JPEGDecoder
#include <FS.h>

#include <Adafruit_NeoPixel.h>

// Map the serpentile-style matrix to the NeoPixel format
const uint8_t remap[8][8] = {
  {7, 6, 5, 4, 3, 2, 1, 0},
  {8, 9, 10, 11, 12, 13, 14, 15},
  {23, 22, 21, 20, 19, 18, 17, 16},
  {24, 25, 26, 27, 28, 29, 30, 31},
  {39, 38, 37, 36, 35, 34, 33, 32},
  {40, 41, 42, 43, 44, 45, 46, 47},
  {55, 54, 53, 52, 51, 50, 49, 48},
  {56, 57, 58, 59, 60, 61, 62, 63},
};

// Gamma correction table
const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

// Define LED matrix object
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(64, D6, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server;
char* ssid = "YOUR_SSID";
char* password = "YOUR_PASSWORD";

// hold uploaded file
fs::File fsUploadFile;

// library and parameters for scheduling animation task
#include <TaskScheduler.h>

void playSequenceCallback();
Task playSequence(0, TASK_FOREVER, &playSequenceCallback);
Scheduler runner;

void setup()
{
  SPIFFS.begin();
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED)
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
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "{\"success\":1}");
  }, handleFileUpload);

  server.on("/play", HTTP_GET, handleFilePlay);
  server.on("/stop", HTTP_GET, handleFileStop);

  // called when the url is not defined
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "File Not Found!");
    }
  });
  server.begin();
  pixels.begin();
  pixels.setBrightness(128);
  pixels.show(); // initializes pixel matrix to off

  runner.addTask(playSequence);
}

void loop()
{
  server.handleClient();
  runner.execute();
}

void playSequenceCallback() {
  String path = "/";
  // Assuming there are no subdirectories
  fs::Dir dir = SPIFFS.openDir(path);
  while (dir.next())
  {
    fs::File entry = dir.openFile("r");
    String filename = entry.name();
    if (filename.endsWith(".jpg")) {
      displayJpegMatrix(filename);
      delay(5);
    }
    entry.close();
  }
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  return "text/plain";
}

void displayJpegMatrix(String path) {
  if (JpegDec.decodeFsFile(path)) {
    uint32_t mcuPixels = JpegDec.MCUWidth * JpegDec.MCUHeight;
    uint8_t row = 0; uint8_t col = 0;

    while (JpegDec.read()) {
      uint16_t *pImg = JpegDec.pImage;
      for (uint8_t i = 0; i < mcuPixels; i++) {
        // Extract the red, green, blue values from each pixel
        uint8_t b = uint8_t((*pImg & 0x001F) << 3); // 5 LSB for blue
        uint8_t g = uint8_t((*pImg & 0x07C0) >> 3); // 6 'middle' bits for green
        uint8_t r = uint8_t((*pImg++ & 0xF800) >> 8); // 5 MSB for red
        // Calculate the matrix index (column and row)
        col = JpegDec.MCUx * 8 + i % 8;
        row = JpegDec.MCUy * 8 + i / 8;
        // Set the matrix pixel to the RGB value
        pixels.setPixelColor(remap[row][col],
                             pgm_read_byte(&gamma8[r]),
                             pgm_read_byte(&gamma8[g]),
                             pgm_read_byte(&gamma8[b]));
      }
    }
    pixels.show();
  }
}

void handleFileDelete() {
  // make sure we get a file name as a URL argument
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS!");
  String path = server.arg(0);
  // protect root path
  if (path == "/") return server.send(500, "text/plain", "BAD PATH!");
  // check if the file exists
  if (!SPIFFS.exists(path)) return server.send(404, "text/plain", "FILE NOT FOUND!");
  SPIFFS.remove(path);
  Serial.println("DELETE: " + path);
  String msg = "deleted file: " + path;
  server.send(200, "text/plain", msg);
}

bool handleFileRead(String path) {
  // Serve index file when top root path is accessed
  if (path.endsWith("/")) path += "index.html";
  // Different file types require different actions
  String contentType = getContentType(path);

  if (SPIFFS.exists(path)) {
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
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
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
  while (dir.next())
  {
    fs::File entry = dir.openFile("r");
    // Separate by comma if there are multiple files
    if (output != "[")
      output += ",";
    output += String(entry.name()).substring(1);
    entry.close();
  }
  output += "]";
  server.send(200, "text/plain", output);
}

void handleFilePlay()
{
  playSequence.enable();
  server.send(200, "text/plain", "{\"success\":1}");
}

void handleFileStop()
{
  playSequence.disable();
  pixels.clear();
  pixels.show(); // clear all pixels
  server.send(200, "text/plain", "{\"success\":1}");
}
