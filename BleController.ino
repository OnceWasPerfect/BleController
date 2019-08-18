#include "BluefruitRoutines.h"
#include "RadioSetup.h"
#include <Wire.h>
#include <PushButton.h>
#include <SPI.h>

//Debug setup
//#define DEBUG //comment out to disable debug
#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
 #define DEBUG_BEGIN(x)     Serial.begin(x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x) 
 #define DEBUG_BEGIN(x)
#endif

#define RANGE 7  //How far the mouse moves
#define RESPONSEDELAY 5  //How often the loop runs
#define DEADZONE 200  //How far before movement registered
#define MAINBUTTONPIN 5 //Pin for main button
#define SCROLLBUTTONPIN 6 //Pin for scroll button

//Setup up a struct to pass the data
typedef struct locationData
{
  int x;  //Will hold the x axis info
  int y;  //Will hold the y axis info
};
locationData *receivedLocation;  //create data object
locationData calibrated;  //hold resting position

//Button setup
PushButton mainButton(MAINBUTTONPIN);  //Main button
PushButton scrollButton(SCROLLBUTTONPIN);  //Scroll button
bool bolScroll = false;  //Is scroll mode active

//Stuff for radio
uint8_t rxbuf[RH_NRF24_MAX_MESSAGE_LEN];  //Set up the receive buffer
uint8_t rxbuflen = sizeof(rxbuf); //size of buffer

void setup()
{
  DEBUG_BEGIN(115200);
  
  //Button setup
  pinMode(MAINBUTTONPIN, INPUT_PULLUP);  //Set button pin with pullup
  pinMode(SCROLLBUTTONPIN, INPUT_PULLUP);  //Set button pin with pullup
  scrollButton.setActiveLogic(LOW);  //Make button active low
  mainButton.setActiveLogic(LOW);  //Make button active low
  
  //Start bluetooth
  initializeBluefruit();

  //Setup radio
  radioSetup();

  DEBUG_PRINTLN("Before first calibration");
  //Calibrate Accelerometer
  calibrate();
}

void loop()
{
  //Setup variables
  int xDistance = 0;  //Movement of x axis
  int yDistance = 0;  //Movement of y axis

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

  //checkmovement functions return 0,1, or -1, then multiply by the RANGE
  checkMovement(xDistance, yDistance);
  xDistance = xDistance * RANGE;
  yDistance = yDistance * RANGE;
  DEBUG_PRINT("Checked X = ");DEBUG_PRINT(xDistance);DEBUG_PRINT(" Checked Y = ");DEBUG_PRINTLN(yDistance);

  //If not zero move
  if ((xDistance != 0) || (yDistance != 0))
  {
    //If not in scroll mode
    if (bolScroll == false)
    {
      String distance = convertMovement(xDistance,yDistance);  //Convert movement to string
      ble.print("AT+BleHidMouseMove=");  //Move Mouse
      ble.println(distance);
      DEBUG_PRINTLN(distance);
    }
    //If in scroll mode
    else
    {
      String distance = convertMovement(-yDistance/7,-xDistance/7); //Convert to string reversed for scroll
      ble.print("AT+BleHidMouseMove=0,0,");  //Scroll mouse
      ble.println(distance);
    }
  }
  
  //ble.print("AT+BleHidMouseMove=7,7");
  delay(RESPONSEDELAY);
}

bool readRadio()
{
  if(nrf24.recv(rxbuf, &rxbuflen)) //Receive the radio payload
  {
    //memcpy(&receivedLocation, rxbuf, sizeof(receivedLocation));  //copy the payload to location 
    receivedLocation = (struct locationData *)rxbuf; 
    //DEBUG_PRINT("Received X = ");DEBUG_PRINT(receivedLocation->x);DEBUG_PRINT(" Received Y = ");DEBUG_PRINTLN(receivedLocation->y);
    return true;
  }  
  else
  {
    //DEBUG_PRINTLN("readRadio failed");
    return false;
  }  
  
}

void checkMovement(int &x, int &y)
{
  while(!readRadio())  //Don't read bad data
  {
    DEBUG_PRINTLN("Bad readRadio, exiting checkMovement");
  }

  locationData checkLocation;  //Place to store current location

  //Take the received location and stick it in check location
  checkLocation.x = receivedLocation->x;  
  checkLocation.y = receivedLocation->y;

  long difference[] = {0,0};  //Place to store the difference between current location and resting location

  //Compare the new location with the calibrated location
  DEBUG_PRINT("CheckMovement X = ");DEBUG_PRINT(checkLocation.x);DEBUG_PRINT(" CheckMovement Y = ");DEBUG_PRINTLN(checkLocation.y);
  difference[0] = checkLocation.x - calibrated.x;
  difference[1] = checkLocation.y - calibrated.y;
  DEBUG_PRINT("Difference X = ");DEBUG_PRINT(difference[0]);DEBUG_PRINT(" Difference Y = ");DEBUG_PRINTLN(difference[1]);

  //Check x axis for movement
  if (abs(difference[0]) > DEADZONE)  //Movement greater than deadzone
  {
    if(checkLocation.x > calibrated.x) //Moved right
    {
      x = 1;
    }
    else if(checkLocation.x < calibrated.x) //Moved left
    {
      x = -1;
    }
  }
  else //No movement
  {
    x = 0;
  }

  //Check y axis for movement
  if (abs(difference[1]) > DEADZONE)  //Movement greater than deadzon
  {
    if(checkLocation.y > calibrated.y)  //Moved down
    {
      y = -1;  
    }
    else if(checkLocation.y < calibrated.y)  //Moved up
    {
      y = 1;  
    }
  }
  else //No movement
  {
    y = 0;
  }
}

void calibrate()
{
  while(!readRadio())  //don't read bad data
  {
    DEBUG_PRINTLN("Bad initial calibration");
  }

  //Store location in calibrated
  calibrated.x = receivedLocation->x;
  calibrated.y = receivedLocation->y;
  DEBUG_PRINT("Calibrated X = ");DEBUG_PRINT(calibrated.x);DEBUG_PRINT(" Calibrated Y = ");DEBUG_PRINTLN(calibrated.y);
}

//Convert to string for ble
String convertMovement(int x, int y)
{
  String movement = String(x);  //Convert xDistance to string
  movement += ",";  //Add the comma
  movement += String(y);  //Convert yDistance to string and add it to the string
  return movement;
}