//Test from master online

#include "BluefruitRoutines.h"
#include <Wire.h>
#include <PushButton.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

#define range 7  //How far the mouse moves
#define responseDelay 5  //How often the loop runs
#define averageFactor 20  //How many captures per movement check
#define calibrationFactor 200  //How many captures for calibration
#define deadZone 200  //How far before movement registered

bool bolScroll = false;  //Is scroll mode active
PushButton scrollButton(5);  //Scroll button
PushButton mainButton(12);  //Main button

Adafruit_LIS3DH lis = Adafruit_LIS3DH();  //Create acc object
int xCalibrated = 0;  //Base value for x
int yCalibrated = 0;  //Base value for y

void setup()
{
  //Assign button pins
  pinMode(5, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);

  //Set buttons to active low
  scrollButton.setActiveLogic(LOW);
  mainButton.setActiveLogic(LOW);

  //Begin acc
  lis.begin(0x18);
  
  //Set Range for Acc
  lis.setRange(LIS3DH_RANGE_16_G);   // 2, 4, 8 or 16 G!

  //Calibrate X and Y axis
  calibrate();
  
  //Start bluetooth
  initializeBluefruit();
}

void loop()
{
  //Update buttons
  scrollButton.update();
  mainButton.update();


  //Check for button events
  if (scrollButton.isActive() && mainButton.isActive())  //Click both to calibrate
  {
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
      ble.println("AT+BleHidMouseButton=L");  //Press but don't release to allow for dragging
    }
    if (mainButton.isReleased())
    {
      ble.println("AT+BleHidMouseButton=0");  //Release the button
    }
  }

  //checkmovement functions return 0,1, or -1, then multiply by the range
  int xDistance = checkXmovement() * range;  
  int yDistance = checkYmovement() * range;

  //Convert movement to string
  String distance = convertMovement(xDistance,yDistance);  

  //If not zero move
  if ((xDistance != 0) || (yDistance != 0))
  {
    //If not in scroll mode
    if (bolScroll == false)
    {
      ble.print("AT+BleHidMouseMove=");
      ble.println(distance);
    }
    //If in scroll mode
    else
    {
      ble.print("AT+BleHidMouseMove=0,0,");
      ble.print(String(-yDistance/2));
      ble.print(",");
      ble.println(String(-xDistance));
      delay(150);
    }
  }
  
  delay(responseDelay);
}

//Average the x movement to reduce jitters
int averageX ()
{
  int average = 0;
  long total = 0L;

  for(int i = 0; i < averageFactor; i++)
  {
    lis.read();  //Get new reading from acc
    total = total + lis.x;  //Sum the readings
  }
  
  average = total / averageFactor;  //Get the average
  return average;
}

//Average the y movement to reduce jitters
int averageY ()
{
  int average = 0;
  long total = 0L;

  for(int i = 0; i < averageFactor; i++)
  {
    lis.read();  //Get new reading from acc
    total = total + lis.y;  //Sum the readings
  }
  
  average = total / averageFactor;  //Get the average
  return average;  
}

//Calibrate resting position
void calibrate ()
{
  long xTotal = 0L;
  long yTotal = 0L;

  for (int i = 0; i < calibrationFactor; i++)  //Sample location calibrationFactor times
  {
    lis.read();  //Get new reading from acc
    xTotal = xTotal + lis.x;  //Total the x locations
    yTotal = yTotal + lis.y;  //Total the y locations
  }

  xCalibrated = xTotal / calibrationFactor; //Average the x location
  yCalibrated = yTotal / calibrationFactor;  //Average the y location
}

//Check for x movement
int checkXmovement()
{
  long newX = averageX();    //Get current location
  long difference = newX - xCalibrated;  //Calculate movement from calibrated

  //check to see if movment
  if (abs(difference) > deadZone)  //Movement greater than deadzone
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
  if (abs(difference) > deadZone)  //Movement greater than deadzon
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
