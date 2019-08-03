//#include "BluefruitRoutines.h"
#include <Wire.h>
#include <PushButton.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Mouse.h>

#define RANGE 7  //How far the mouse moves
#define RESPONSEDELAY 5  //How often the loop runs
#define AVERAGEFACTOR 20  //How many captures per movement check
#define CALIBRATIONFACTOR 200  //How many captures for calibration
#define DEADZONE 200  //How far before movement registered

PushButton mainButton(3);  //Main button
PushButton scrollButton(4);  //Scroll button
Adafruit_LIS3DH lis = Adafruit_LIS3DH();  //Create acc object
int xCalibrated = 0;  //Base value for x
int yCalibrated = 0;  //Base value for y
int xDistance = 0;  //Distance to move in x direction
int yDistance = 0; //Distance to move in y direction
bool bolScroll = false;  //Is scroll mode active
long xCurrent = 0L;  //Current location of x
long yCurrent = 0L;  //Current location of y

void setup()
{
  //Assign button pins
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  //Set buttons to active low
  scrollButton.setActiveLogic(LOW);
  mainButton.setActiveLogic(LOW);

  //Begin acc
  lis.begin(0x18);
  
  //Set RANGE for Acc
  lis.setRange(LIS3DH_RANGE_16_G);   // 2, 4, 8 or 16 G!

  //Calibrate X and Y axis
  calibrate();

  Mouse.begin();
  //Serial.begin(9600);
  //Serial.println("Exiting Setup");
}

void loop()
{
  //Update buttons
  scrollButton.update();
  mainButton.update();


  //Check for button events
  if (scrollButton.isActive() && mainButton.isActive())  //Click both to calibrate
  {
    Serial.println("Calling Calibrate");
    calibrate();
  }
  else
  {
    if (scrollButton.isClicked())  //Check if scroll mode change
    {
      bolScroll = !bolScroll;
    }
    if (mainButton.isActive())  //Check for main button
    {
      Mouse.press(MOUSE_LEFT);  //Press but don't release to allow for dragging
    }
    if (mainButton.isReleased())
    {
      Mouse.release(MOUSE_LEFT);  //Release the button
    }
  }

  //checkmovement functions return 0,1, or -1, then multiply by the RANGE
  xDistance = checkXmovement() * RANGE;
  yDistance = checkYmovement() * RANGE;

  //If not zero move
  if ((xDistance != 0) || (yDistance != 0))
  {
    //If not in scroll mode
    if (bolScroll == false)
    {
      Mouse.move(xDistance, yDistance, 0);
      xCurrent = xCurrent + xDistance;
      yCurrent = yCurrent + yDistance;
      /*Serial.print("xDistance = \t"); Serial.print(xDistance);
      Serial.print(" \tyDistance = \t");Serial.println(yDistance);
      Serial.print("xCurrent = \t"); Serial.print(xCurrent);
      Serial.print(" \tyCurrent= \t"); Serial.println(yCurrent);*/
    }
    //If in scroll mode
    else
    {
      Mouse.move(0,0, -yDistance);
      delay(150);
    }
  }
  
  delay(RESPONSEDELAY);
}

//Average the x movement to reduce jitters
int averageX ()
{
  int average = 0;
  long total = 0L;

  for(int i = 0; i < AVERAGEFACTOR; i++)
  {
    lis.read();  //Get new reading from acc
    total = total + lis.x;  //Sum the readings
  }
  
  average = total / AVERAGEFACTOR;  //Get the average
  return average;
}

//Average the y movement to reduce jitters
int averageY ()
{
  int average = 0;
  long total = 0L;

  for(int i = 0; i < AVERAGEFACTOR; i++)
  {
    lis.read();  //Get new reading from acc
    total = total + lis.y;  //Sum the readings
  }
  
  average = total / AVERAGEFACTOR;  //Get the average
  return average;  
}

//Calibrate resting position
void calibrate ()
{
  long xTotal = 0L;
  long yTotal = 0L;

  for (int i = 0; i < CALIBRATIONFACTOR; i++)  //Sample location CALIBRATIONFACTOR times
  {
    lis.read();  //Get new reading from acc
    xTotal = xTotal + lis.x;  //Total the x locations
    yTotal = yTotal + lis.y;  //Total the y locations
  }

  xCalibrated = xTotal / CALIBRATIONFACTOR; //Average the x location
  yCalibrated = yTotal / CALIBRATIONFACTOR;  //Average the y location
  
  /*Serial.println("xCurrent and yCurrent before move");
  Serial.print("xCurrent = \t"); Serial.print(xCurrent);
  Serial.print(" \tyCurrent = \t"); Serial.println(yCurrent);
  Mouse.move(-xCurrent, -yCurrent, 0);  //Mouse mouse opposite of its movement data
  Serial.println("xCurrent and yCurrent after move");
  Serial.print("xCurrent = \t"); Serial.print(xCurrent);
  Serial.print(" \tyCurrent = \t"); Serial.println(yCurrent);
  xCurrent = 0;  //Rest current x location
  yCurrent = 0;  //Reset current y location
  Serial.println("xCurrent and yCurrent after reset");
  Serial.print("xCurrent = \t"); Serial.print(xCurrent);
  Serial.print(" \tyCurrent = \t"); Serial.println(yCurrent);*/

}

//Check for x movement
int checkXmovement()
{
  long newX = averageX();    //Get current location
  long difference = newX - xCalibrated;  //Calculate movement from calibrated

  //check to see if movment
  if (abs(difference) > DEADZONE)  //Movement greater than deadzone
  {
    if(newX > xCalibrated) //Moved right
    {
      return 1;
    }
    else if(newX < xCalibrated) //Moved left
    {
      return -1;
    }
  }
  else //No movement
  {
    return 0;
  }
}

//Check for y movement
int checkYmovement()
{
  long newY = averageY();    //Get current location
  long difference = newY - yCalibrated;  //Calculate movement from calibrated

  //check to see if movment
  if (abs(difference) > DEADZONE)  //Movement greater than deadzon
  {
    if(newY > yCalibrated)  //Moved down
    {
      return -1;  
    }
    else if(newY < yCalibrated)  //Moved up
    {
      return 1;  
    }
  }
  else //No movement
  {
    return 0;
  }
}

//Convert to string for ble
String convertMovement(int x, int y)
{
  String movement = String(x);  //Convert xDistance to string
  movement += ",";  //Add the comma
  movement += String(y);  //Convert yDistance to string and add it to the string
  return movement;
}