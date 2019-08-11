#include <Adafruit_LIS3DH.h>  //Accelerometer library
#include <Adafruit_Sensor.h>  //Accelerometer library
#include <SPI.h>  //SPI hardware library
#include <nRF24L01.h>  //Wireless tranceiver library
#include <RF24.h>  //Wireless tranceiver library

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

#define RADIOCEPIN 10  //Chip enable pin for radio
#define RADIOCSNPIN 9 //Chip select pin for radio

//Setup up a struct to pass the data
typedef struct data
{
  int x;  //Will hold the x axis info
  int y;  //Will hold the y axis info
  int test;  //future use
};
data location;  //create data object

//Accelerometer setup
Adafruit_LIS3DH lis = Adafruit_LIS3DH();  //Create acc object

//Radio setup
RF24 radio(RADIOCEPIN, RADIOCSNPIN); // Set up the radio object CE, CSN
const byte address[][6] = {"00001", "00002"};  //The address the radio will work on
bool sendData = false;  //Did feather ask for data?

void setup()
{
  DEBUG_BEGIN(115200);
  //Accelerometer setup 
  lis.begin(0x18);  //Start the accelerometer object
  lis.setRange(LIS3DH_RANGE_16_G);  //Set RANGE for accelerometer 2, 4, 8 or 16 G!

  //Radio setup
  radio.begin();  //Start the radio object
  radio.enableAckPayload();  //Enable the acknowledge response
  radio.enableDynamicPayloads();  //Acknowledge response is a daynamic payload
  radio.openWritingPipe(address[0]);  //Start the writing pipe
  radio.openReadingPipe(1, address[1]);  //Start the reading pipe
  radio.setPALevel(RF24_PA_LOW);  //How strong to send the signal
  radio.setDataRate(RF24_2MBPS);
  radio.startListening();  //Start listening for send command
}

void loop()
{
  while (radio.available())  //Look for send command
  {
      DEBUG_PRINTLN("Inside radio.available()");
    radio.read(&sendData, sizeof(sendData));  //Read in command
      DEBUG_PRINT("sendData = "); DEBUG_PRINTLN(sendData);
    if(sendData == true)  //If feather wants data
    {
      //Read from accelerometer
      lis.read();  //Get new location data
      location.x = lis.x;  //store x axis
      location.y = lis.y;  //store y axis
      DEBUG_PRINTLN("Before Radio");
      DEBUG_PRINT("x = ");DEBUG_PRINT(location.x); DEBUG_PRINT(" y = "); DEBUG_PRINTLN(location.y);
      location.x = 1;
      location.y = 2;
      radio.writeAckPayload(1, &location, sizeof(location));  //Write the data
      DEBUG_PRINTLN("After Radio");
      DEBUG_PRINT("x = ");DEBUG_PRINT(location.x); DEBUG_PRINT(" y = "); DEBUG_PRINTLN(location.y);
      sendData = false;  //Reset to look for next read command
    }
    delay(10);
  }   
}
  