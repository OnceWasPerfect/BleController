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

//Location data setup
int receivedLocation[] = {0,0}; //Data from foot
int restingPosition[] = {0,0};

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
  
  DEBUG_PRINTLN("Before first calibration");
  //Calibrate Accelerometer
  averageLocation(restingPosition);
  
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
    averageLocation(restingPosition);  //Reset resting position
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
  DEBUG_PRINTLN("Start of readRadio");
  radio.stopListening();  //Stop listening to send command

  if (radio.write(&sendData, sizeof(sendData)))  //Send command boolean
  {
    DEBUG_PRINTLN("Start of readRAdio if");
    if(!radio.available())
    {
      //Nothing there
      DEBUG_PRINTLN("Blank response");
      return false;  //Didn't receive data
    }
    else
    {
      while(radio.available())
      {
        radio.read(&receivedLocation, sizeof(receivedLocation)); //Get the location data from foot
        DEBUG_PRINTLN("Got reading from radio");
        DEBUG_PRINT("Data from radio: x = ");DEBUG_PRINT(receivedLocation[0]);DEBUG_PRINT(" y = ");DEBUG_PRINTLN(receivedLocation[1]);
        return true;  //Say we got good data
      }
      DEBUG_PRINTLN("In readRadio else loop but not while is available loop");
      DEBUG_PRINT("Data from radio in else loop: x = ");DEBUG_PRINT(receivedLocation[0]);DEBUG_PRINT(" y = ");DEBUG_PRINTLN(receivedLocation[1]);
    }
    DEBUG_PRINTLN("Bottom of radio read if");
    DEBUG_PRINT("Data from bottom of radioRead if: x = ");DEBUG_PRINT(receivedLocation[0]);DEBUG_PRINT(" y = ");DEBUG_PRINTLN(receivedLocation[1]);
  }
  else
  {
    DEBUG_PRINTLN("Sending failed");
    return false;  //Didn't receive data
  }

  radio.startListening();  //Start listening again
}

void averageLocation(int currentLocation[2])
{
  DEBUG_PRINTLN("Start of averageLocation");
  int total[] = {0,0};  //Place to store the totals
  bool goodRead = false;
  
  for (int i = 0; i < AVERAGEFACTOR;)
  {
    goodRead = readRadio();
    if(goodRead)  //If received a good value
    {
      total[0] += receivedLocation[0];  //add the x value to total
      total[1] += receivedLocation[1];  //add the y value to total
      i++;  //Only increment if good value (can cause infinite loop if never get good data)
      DEBUG_PRINT("Inside averageLocation for loop i = "); DEBUG_PRINTLN(i);
    }
    else
    {
      //didn't get a value so don't increment i
      DEBUG_PRINTLN("In else of averageLocation no good read");
    }
  }

  currentLocation[0] = total[0] / AVERAGEFACTOR;  //Average the x axis
  currentLocation[1] = total[1] / AVERAGEFACTOR;  //Average the y axis
}

void checkMovement(int &x, int &y)
{
  int location[] = {0,0};  //Place to store location data
  averageLocation(location);  //Get current location data
  long difference[] = {0,0};  //Place to store the difference between current location and resting location

  for(int i = 0; i < 2; i++)  //get the difference in x and y from resting
  {
    difference[i] = location[i] - restingPosition[i];
  }

  //Check x axis for movement
  if (abs(difference[0]) > DEADZONE)  //Movement greater than deadzone
  {
    if(location[0] > restingPosition[0]) //Moved right
    {
      x = 1;
    }
    else if(location[0] < restingPosition[0]) //Moved left
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
    if(location[1] > restingPosition[1])  //Moved down
    {
      y = -1;  
    }
    else if(location[1] < restingPosition[1])  //Moved up
    {
      y = 1;  
    }
  }
  else //No movement
  {
    y = 0;
  }
}