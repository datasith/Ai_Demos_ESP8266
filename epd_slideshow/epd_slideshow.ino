/*------------------------------------------------------------------------------
  01/21/2020
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: epd_slideshow.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to build an e-Paper Display picture 
  frame controlled by an ESP8266, and use it to display photos that users can
  drag and drop by accessing a web server running on the ESP8266 using a browser:
  https://youtu.be/p3jXovhnfCU
  
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
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

ESP8266WebServer server;
char* ssid = "YOUR_SSID";
char* password = "YOUR_PASSWORD";

// EPD parameters
static const uint16_t input_buffer_pixels = 800; // may affect performance
static const uint16_t max_row_width = 800; // for up to 7.5" display 800x480
static const uint16_t max_palette_pixels = 256; // for depth <= 8
uint8_t input_buffer[3 * input_buffer_pixels]; // up to depth 24
uint8_t output_row_mono_buffer[max_row_width / 8]; // buffer for at least one row of b/w bits
uint8_t output_row_color_buffer[max_row_width / 8]; // buffer for at least one row of color bits
uint8_t mono_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w

// We won't need the GFX base class so disable it
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Instantiate the GxEPD2_BW class for our display type
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(SS, 4, 5, 16));

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
  delay(1000);

  // list available files
  server.on("/list", HTTP_GET, handleFileList);
  // delete file
  server.on("/delete", HTTP_DELETE, handleFileDelete);
  // handle file upload
  server.on("/upload", HTTP_POST, [](){
    server.send(200, "text/plain", "{\"success\":1}");
  }, handleFileUpload);
  // handle calls to start/stop slideshow
  server.on("/start", HTTP_GET, handleDisplayStart);
  server.on("/stop", HTTP_GET, handleDisplayStop);
  
  // called when the url is not defined
  server.onNotFound([](){
    if(!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "File Not Found!");
    }
  });
  server.begin();

  // Initialize communication with the display
  display.init(115200);
}

// global flag to run/stop the slideshow on the display
bool RUN_PHOTO_GALLERY = false;

void loop()
{
  server.handleClient();
  // when the slideshow is running, change the picture every 5 seconds
  static uint16_t t = 0;
  uint16_t loop_time = millis() - t;
  if(RUN_PHOTO_GALLERY)
  {
    if(loop_time > 5*1000)
    {
      displayBmpEpd();
      t = millis();
    }
  }
}

// global variables for tracking the number of picture files available and 
// the current one that's being displayed
uint16_t num_of_files = 0;
uint16_t file_index = 0;

// functions for determining which file to display
void handleDisplayStart() {
  // start the slideshow
  if(!RUN_PHOTO_GALLERY) {
    RUN_PHOTO_GALLERY = true;
    // configure the display
    display.setRotation(0);
    display.setFullWindow();

    // reset number of files counter before starting the count
    num_of_files = 0;

    // assuming no subdirectories, count the number of .bmp files available
    fs::Dir dir = SPIFFS.openDir("/");
    while(dir.next()) {
      fs::File file = dir.openFile("r");
      String filename = String(file.name()).substring(1);
      if(getContentType(filename) == "image/bmp")
        num_of_files+=1;
    }
  }
  // send a response back to the client
  server.send(200, "text/plain", "{\"start\":1}");
}

void handleDisplayStop() {
  if(RUN_PHOTO_GALLERY)
    RUN_PHOTO_GALLERY = false;
  // send a response back to the client
  server.send(200, "text/plain", "{\"stop\":1}");  
}

void displayBmpEpd() {
  // handle the image display
  if(num_of_files == 0)
    return;

  // find the file we need to display, and keep track of its index
  fs::Dir dir = SPIFFS.openDir("/");
  uint16_t i = 0;
  // loop through all the files
  while(dir.next()) {
    fs::File file = dir.openFile("r");
    String filename = String(file.name()).substring(1);
    // check if it's a .bmp
    if(getContentType(filename) != "image/bmp")
      continue;
    // check if we're at the index of the correct file
    if(i++ < file_index)
      continue;
    filename = "/" + filename;
    // reset the index if we're at the last file in the file system
    if(i == num_of_files) file_index = 0;
    // otherwise set the index
    else file_index = i;

    // use the GxEPD2 function to display the file
    drawBitmapFromSpiffs(filename.c_str(), 0, 0, false);
    break;
  }
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".bmp")) return "image/bmp";  
  return "text/plain";
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
  // stop the slideshow every time a new file is added
  RUN_PHOTO_GALLERY = false;
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

// EPD image processing functions
void drawBitmapFromSpiffs(const char *filename, int16_t x, int16_t y, bool with_color)
{
  fs::File file;
  bool valid = false; // valid format to be handled
  bool flip = true; // bitmap is stored bottom-to-top
  uint32_t startTime = millis();
  if ((x >= display.width()) || (y >= display.height())) return;
  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');
#if defined(ESP32)
  file = SPIFFS.open(String("/") + filename, "r");
#else
  file = SPIFFS.open(filename, "r");
#endif
  if (!file)
  {
    Serial.print("File not found");
    return;
  }
  // Parse BMP header
  if (read16(file) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(file);
    uint32_t creatorBytes = read32(file);
    uint32_t imageOffset = read32(file); // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width  = read32(file);
    uint32_t height = read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file); // bits per pixel
    uint32_t format = read32(file);
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      if (w <= max_row_width) // handle with direct drawing
      {
        valid = true;
        uint8_t bitmask = 0xFF;
        uint8_t bitshift = 8 - depth;
        uint16_t red, green, blue;
        bool whitish, colored;
        if (depth == 1) with_color = false;
        if (depth <= 8)
        {
          if (depth < 8) bitmask >>= depth;
          //file.seek(54); //palette is always @ 54
          file.seek(imageOffset - (4 << depth)); // 54 for regular, diff for colorsimportant
          for (uint16_t pn = 0; pn < (1 << depth); pn++)
          {
            blue  = file.read();
            green = file.read();
            red   = file.read();
            file.read();
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
            if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
            mono_palette_buffer[pn / 8] |= whitish << pn % 8;
            if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
            color_palette_buffer[pn / 8] |= colored << pn % 8;
          }
        }
        display.clearScreen();
        uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
        for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
        {
          uint32_t in_remain = rowSize;
          uint32_t in_idx = 0;
          uint32_t in_bytes = 0;
          uint8_t in_byte = 0; // for depth <= 8
          uint8_t in_bits = 0; // for depth <= 8
          uint8_t out_byte = 0xFF; // white (for w%8!=0 boarder)
          uint8_t out_color_byte = 0xFF; // white (for w%8!=0 boarder)
          uint32_t out_idx = 0;
          file.seek(rowPosition);
          for (uint16_t col = 0; col < w; col++) // for each pixel
          {
            // Time to read more pixel data?
            if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
            {
              in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
              in_remain -= in_bytes;
              in_idx = 0;
            }
            switch (depth)
            {
              case 24:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                break;
              case 16:
                {
                  uint8_t lsb = input_buffer[in_idx++];
                  uint8_t msb = input_buffer[in_idx++];
                  if (format == 0) // 555
                  {
                    blue  = (lsb & 0x1F) << 3;
                    green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                    red   = (msb & 0x7C) << 1;
                  }
                  else // 565
                  {
                    blue  = (lsb & 0x1F) << 3;
                    green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                    red   = (msb & 0xF8);
                  }
                  whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                  colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                }
                break;
              case 1:
              case 4:
              case 8:
                {
                  if (0 == in_bits)
                  {
                    in_byte = input_buffer[in_idx++];
                    in_bits = 8;
                  }
                  uint16_t pn = (in_byte >> bitshift) & bitmask;
                  whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  in_byte <<= depth;
                  in_bits -= depth;
                }
                break;
            }
            if (whitish)
            {
              // keep white
            }
            else if (colored && with_color)
            {
              out_color_byte &= ~(0x80 >> col % 8); // colored
            }
            else
            {
              out_byte &= ~(0x80 >> col % 8); // black
            }
            if ((7 == col % 8) || (col == w - 1)) // write that last byte! (for w%8!=0 boarder)
            {
              output_row_color_buffer[out_idx] = out_color_byte;
              output_row_mono_buffer[out_idx++] = out_byte;
              out_byte = 0xFF; // white (for w%8!=0 boarder)
              out_color_byte = 0xFF; // white (for w%8!=0 boarder)
            }
          } // end pixel
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          display.writeImage(output_row_mono_buffer, output_row_color_buffer, x, yrow, w, 1);
        } // end line
        Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
        display.refresh();
      }
    }
  }
  file.close();
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}

uint16_t read16(fs::File& f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File& f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
