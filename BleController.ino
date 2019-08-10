#include "BluefruitRoutines.h"
#include <Wire.h>
#include <PushButton.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

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
#define RADIOCEPIN 28  //Controller enable pin for radio
#define RADIOCSNPIN 29  //Controller select pin for radio

//Button setup
PushButton mainButton(MAINBUTTONPIN);  //Main button
PushButton scrollButton(SCROLLBUTTONPIN);  //Scroll button
bool bolScroll = false;  //Is scroll mode active

//Accelerometer data setup
int xCalibrated = 0;  //Base value for x
int yCalibrated = 0;  //Base value for y
int xDistance = 0;  //Movement of x axis
int yDistance = 0;  //Movement of y axis
int receivedLocation[] = {0,0}; //Data from foot

//Radio setup
RF24 radio(RADIOCEPIN, RADIOCSNPIN); // create radio object CE, CSN
const byte address[][6] = {"00001", "00002"};  //Address the radios will use
bool sendData = true;  //Tell foot to send data

void setup()
{
  DEBUG_BEGIN(115200);
  //Button setup
  pinMode(MAINBUTTONPIN, INPUT_PULLUP);  //Set button pin with pullup
  pinMode(SCROLLBUTTONPIN, INPUT_PULLUP);  //Set button pin with pullup
  scrollButton.setActiveLogic(LOW);  //Make button active low
  mainButton.setActiveLogic(LOW);  //Make button active low
  
  //Setup radio
  radio.begin();  //Start radio object
  radio.enableAckPayload();  //Enable the acknowledge response
  radio.enableDynamicPayloads();  //Acknowledge response is a daynamic payload
  radio.openWritingPipe(address[1]);  //Start the writing pipe
  radio.openReadingPipe(1, address[0]);  //Start the reading pipe
  radio.setPALevel(RF24_PA_MIN);  //How strong to send the signal
  radio.startListening();  //Start listening for acknowledge
  
  //Calibrate Accelerometer
  calibrate();
  DEBUG_PRINTLN("After calibrate setup");
  
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
      blehid.mouseButtonPress(MOUSE_BUTTON_LEFT);
    }
    if (mainButton.isReleased())
    {
      blehid.mouseButtonRelease();
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

//Average the x movement to reduce jitters
int averageX ()
{
  int average = 0;
  long total = 0L;
  int location[] = {0,0};

  for(int i = 0; i < AVERAGEFACTOR; i++)
  {
    readRadio();
    total = total + receivedLocation[0];
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
    readRadio();
    total = total + receivedLocation[1];
  }
  
  average = total / AVERAGEFACTOR;  //Get the average
  return average;  
}

//Calibrate resting position
void calibrate ()
{
  long xTotal = 0L;
  long yTotal = 0L;
  int location[] = {0,0};

  for (int i = 0; i < CALIBRATIONFACTOR; i++)  //Sample location CALIBRATIONFACTOR times
  {
    readRadio();
    xTotal = xTotal + receivedLocation[0];
    yTotal = yTotal + receivedLocation[1];
  }

  xCalibrated = xTotal / CALIBRATIONFACTOR; //Average the x location
  yCalibrated = yTotal / CALIBRATIONFACTOR;  //Average the y location
  DEBUG_PRINT("xCalibrated = ");DEBUG_PRINTLN(xCalibrated);
  DEBUG_PRINT("yCalibrated = ");DEBUG_PRINTLN(yCalibrated);
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

void readRadio()
{
  radio.stopListening();  //Stop listening to send command

  if (radio.write(&sendData, sizeof(sendData)))  //Send command boolean
  {
    if(!radio.available())
    {
      //Nothing there
      DEBUG_PRINTLN("Blank response");
    }
    else
    {
      while(radio.available())
      {
        radio.read(&receivedLocation, sizeof(receivedLocation));
      }
    }
  }
  else
  {
    DEBUG_PRINTLN("Sending failed");
  }
}