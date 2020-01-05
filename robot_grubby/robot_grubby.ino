/*------------------------------------------------------------------------------
  09/30/2019
  Author: Makerbro
  Platforms: ESP8266
  Language: C++/Arduino
  File: robot_grubby.ino
  ------------------------------------------------------------------------------
  Description:
  Firmware for trash-eating robot (Grubby) based on Eunchan Park's fantastic 
  design (https://www.thingiverse.com/thing:2824451). It uses a WeMos D1 Mini
  development board for the ESP8266 that will eventually feature WiFi 
  functionalities (e.g., notifications), a micro-sized SG90 servo, an IR 2y0a21 
  sensor, and a piezoelectric buzzer for emitting different sounds based on a 
  few states. The body is 3d-printed and the code is written in C++/Arduino.
  https://youtu.be/9AL5Xim_quk

  I encourage you to visit Eunchan Park's YT channel for other awesome designs:
  https://www.youtube.com/EunChanPark/

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

#include <Servo.h>

// let's make an instance of Servo
Servo armServo;

// this function automatically runs only once when the Arduino's power up
void setup() {

  // we use pin number 12 for control the robot.
  armServo.attach(12);
  // move the motor to default angle
  armServo.write(90);

  // pin A0 is for the distance sensor
  pinMode(A0, INPUT);
}

// in order to check coming hand,
// we will keep the sensor value these variables
int sensorValue = 0;
int prevSensorValue = 0;

// When the sensor value is hight than the THRESHOLD value,
// The code will call action();
const int THRESHOLD = 360;

void loop() {

  // read A7's analog value and asign the value into sensorValue
  sensorValue = analogRead(A7);

  // these two lines means that some object has come
  // from outside of Threshold to inside of it.
  if (prevSensorValue <= THRESHOLD) {
    if (sensorValue > THRESHOLD) {
      // It's time to action
      action();
    }
  }

  // regardless the action, save current sensor value prevSensorValue
  // so that we can check the direction
  prevSensorValue = sensorValue;

  // this delay controls how often this loop is running
  // without this delay, this loop runs too fast
  // then, the differences between previous and current sensor value
  // can not be meaningful.
  delay(10);
}


void action() {
  // eating sequence
  // wait 1000ms (1 second)
  delay(1000);

  // let's use pin12 for motor control
  armServo.attach(12);
  // let's move the motor to degree '10' (move the arm up)
  armServo.write(10);

  // wait 300 ms
  delay(300);

  // let's move the motor to degree '70' (move the arm down)
  armServo.write(70);
  delay(500);

  // after ate
  delay(100);
  armServo.write(50);
  delay(250);
  armServo.write(70);
  delay(250);
  armServo.write(50);
  delay(250);
  armServo.write(70);
  delay(250);
  armServo.write(50);
  delay(250);
  armServo.write(70);
  delay(250);
  armServo.write(50);
  delay(250);
  armServo.write(90);
  delay(250);

  // release arm's torque
  armServo.detach();
}
