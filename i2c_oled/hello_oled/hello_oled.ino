#include <Wire.h>
#include "util.h"

void setup()
{
  Wire.begin();	
  displayInit();                // initialze OLED display
  displayClear();               // clear the display
  setTextXY(0,0);               // Set the cursor position to 0th page (row), 0th column
  displayString("HELLO");
  setTextXY(3,7);               // Set the cursor position to 3rd page (row), 7th column
  displayString("OLED");
}

void loop()
{
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
  sendCommand(CMD_DISPLAY_OFF);         // display off
  delay(5);
  sendCommand(CMD_DISPLAY_ON);          // display on
  delay(5);
  sendCommand(CMD_DISPLAY_NORMAL);  // display can be either normal on inverse
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
