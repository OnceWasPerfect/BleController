#include "BluefruitRoutines.h"
#include "RadioSetup.h"
#include <Wire.h>
#include <PushButton.h>
#include <SPI.h>

//Debug setup
#define DEBUG //comment out to disable debug
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
#define AVERAGEFACTOR 20  //How many captures per movement check
#define CALIBRATIONFACTOR 200  //How many captures for calibration
#define DEADZONE 200  //How far before movement registered
#define MAINBUTTONPIN 27 //Pin for main button
#define SCROLLBUTTONPIN 30 //Pin for scroll button

//Setup up a struct to pass the data
typedef struct data
{
  int x;  //Will hold the x axis info
  int y;  //Will hold the y axis info
};
data *receivedLocation;  //create data object
data calibrated;  //hold resting position

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
  
  //Setup radio
  radioSetup();

  DEBUG_PRINTLN("Before first calibration");
  //Calibrate Accelerometer
  calibrated = averageLocation();
  
  //Start bluetooth
  initializeBluefruit();
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
    calibrated = averageLocation();  //Reset resting position
  }
  else
  {
    if (scrollButton.isClicked())  //Check if scroll mode change
    {
      bolScroll = !bolScroll;
    }
    if (mainButton.isActive())  //Check for main button
    {
      blehid.mouseButtonPress(MOUSE_BUTTON_LEFT);
    }
    if (mainButton.isReleased())
    {
      blehid.mouseButtonRelease();
    }
  }

  //checkmovement functions return 0,1, or -1, then multiply by the RANGE
  checkMovement(xDistance, yDistance);
  xDistance = xDistance * RANGE;
  yDistance = yDistance * RANGE;

  //If not zero move
  if ((xDistance != 0) || (yDistance != 0))
  {
    //If not in scroll mode
    if (bolScroll == false)
    {
      blehid.mouseMove(xDistance, yDistance);
    }
    //If in scroll mode
    else
    {
      blehid.mouseScroll(-yDistance/7);
      blehid.mousePan(-xDistance/7);
      //delay(200);
    }
  }
  
  delay(RESPONSEDELAY);
}

bool readRadio()
{
  if(nrf24.recv(rxbuf, &rxbuflen)) //Receive the radio payload
  {
    //memcpy(&receivedLocation, rxbuf, sizeof(receivedLocation));  //copy the payload to location 
    receivedLocation = (struct data *)rxbuf; 
    DEBUG_PRINT("Received X = ");DEBUG_PRINT(receivedLocation->x);DEBUG_PRINT(" Received Y = ");DEBUG_PRINTLN(receivedLocation->y);
    return true;
  }  
  else
  {
    DEBUG_PRINTLN("readRadio failed");
    return false;
  }  
  
}

data averageLocation()
{
  DEBUG_PRINTLN("Start of averageLocation");
  long total[] = {0,0};  //Place to store the totals
  data average; 
  
  for (int i = 0; i < AVERAGEFACTOR;)
  {
    if(readRadio())
    {
      total[0] = receivedLocation->x;
      total[1] = receivedLocation->y;
      i++;
    }
    
  }

  average.x = total[0] / AVERAGEFACTOR;  //Average the x axis
  average.y = total[1] / AVERAGEFACTOR;  //Average the y axis

  return average;
}

void checkMovement(int &x, int &y)
{
  data checkLocation = averageLocation();
  long difference[] = {0,0};  //Place to store the difference between current location and resting location

  difference[0] = checkLocation.x - calibrated.x;
  difference[1] = checkLocation.y - calibrated.y;

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