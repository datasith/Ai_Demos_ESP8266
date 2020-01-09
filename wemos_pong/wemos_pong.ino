/*------------------------------------------------------------------------------
  11/11/2018
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: wemos_pong.ino
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video building a tiny arcade using the ESP8266, and OLED dis-
  play, and a couple of push buttons. The sketch demos a basic version of pong
  for the ESP8266. Find the wiring diagram in the sketch directory and the build
  process at:
  https://youtu.be/BRQIxeqqmu0

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
#include<U8g2lib.h>
#include<Wire.h>

// Controls
#define upButton D3
#define downButton D7

// Ball
int x, y, radius=3;
int xSpeed, ySpeed;

// Players
float userX=0, userY=30;
float comX=126, comY=30;
float userSpeed=1.23, comSpeed=1.23;
int rallyCount=0, maxRally=10; // add difficulty

// GUI
int border=9;
int maxHeight=63, maxWidth=127; // screen size
int padHeight=10, padWidth=2;
int gameHeight=maxHeight-border, gameWidth=maxWidth-padWidth;
int userScore=0, comScore=0, maxScore=7;
String displayScore="";

// Display object
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

void setup(){
  pinMode(upButton,INPUT_PULLUP);
  pinMode(downButton,INPUT_PULLUP);
  Serial.begin(115200);
  oledConfig();
  // Initialize ball position and speed
  x=random(50,70);
  y=random(border+radius+5,63-5);
  xSpeed=1;
  ySpeed=1;
}

void loop(){
  move();
  display();
}

void oledConfig() {
  oled.begin();  
  oled.setFont(u8g2_font_6x10_tf);
  oled.setFontRefHeightExtendedText();
  oled.setDrawColor(1);
  oled.setFontPosTop();
  oled.setFontDirection(0);
}

void move(){ 
  // If the ball has reached a boundary, change direction
  if(x+radius>gameWidth || x-radius<padWidth)
    xSpeed=-xSpeed;
  if(y+radius>=maxHeight || y-radius<=border)
    ySpeed=-ySpeed;
  // Move the ball to its new position
  x+=xSpeed;
  y+=ySpeed;

  // Move the user's pad
  if(digitalRead(downButton)==LOW){
    userY+=userSpeed;
    if(userY>=gameHeight) userY=53; // don't let pad go off playing area
  } else if(digitalRead(upButton)==LOW){
    userY-=userSpeed;
    if(userY<=border) userY=border;
  }

  // Move the com's pad; simple AI
  if(xSpeed){
    if(ySpeed>0) comY+=comSpeed; // if the ball is going down, move the pad down
    else comY-=comSpeed;         // and viceversa.

    if(comY>=gameHeight) comY=gameHeight; // don't let pad go off playing area
    if(comY<=border) comY=border;
  }

  // Check if com scored
  if((x-radius)<=(userX+1)){
    if(!(y+radius>=userY && y-radius<=userY+padHeight)){
      comScore++; // golazo
      delay(1000);
      x=random(50,70);
      y=random(border+radius+5,63-5);
    } else {
      rallyCount++;
    }
    if(rallyCount>maxRally) {
      rallyCount=0;
      xSpeed = xSpeed>0 ? xSpeed+1 : xSpeed-1;
      ySpeed = ySpeed>0 ? ySpeed+1 : ySpeed-1;
    }
    if(xSpeed>2 || ySpeed>2){
      xSpeed=2;
      ySpeed=2;
    } else if(xSpeed<-2 || ySpeed<-2){
      xSpeed=-2;
      ySpeed=-2;
    }
  }

  // Check if user scored
  if((comX-1)<=(x+radius)){
    if(!(y+radius>=comY && y-radius<=comY+padHeight)){
      userScore++; // Messi <3
      delay(1000);
      x=random(50,70);
      y=random(border+radius+5,63-5);           
    }
  }

  // Update score
  displayScore="ACROBOTIC u: "+String(userScore)+" | c: "+String(comScore);
}

void display(){
    oled.firstPage();
    do {
      oled.drawCircle(x,y,radius);
      oled.drawStr(0,0,displayScore.c_str());
      oled.drawLine(0,border,127,border);
      oled.drawBox(userX,userY,padWidth,padHeight);
      oled.drawBox(comX,comY,padWidth,padHeight);
    } while(oled.nextPage());

    // Game Over message
    if(userScore>=maxScore || comScore>=maxScore){
      oled.firstPage();
      do{
        const char* msg = userScore>=maxScore ? "YOU WON :)" : "YOU LOST :(";
        oled.drawStr(15,5,msg);
        oled.drawStr(30,30,"Press any Key to");
        oled.drawStr(30,55,"start over");
      } while(oled.nextPage());
      // Wait until a key is pressed
      while(1){
        if(!(digitalRead(upButton) && digitalRead(downButton))){
          break;
        }
        delay(20);
      }
      x=random(50,70);
      y=random(border+radius+5,maxHeight-5);
      userScore=0;
      comScore=0;
      // Mercy rule
      if(comScore>=maxScore){
        xSpeed=1;
        ySpeed=1;
        rallyCount=0;
      }
    }
}
