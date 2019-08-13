#include "headers\RadioSetup.h"  //Radio setup routine
#include <Adafruit_LIS3DH.h>  //Accelerometer library
#include <Adafruit_Sensor.h>  //Accelerometer library
#include <SPI.h>  //SPI hardware library

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

//Setup up a struct to pass the data
typedef struct data
{
  int x;  //Will hold the x axis info
  int y;  //Will hold the y axis info
};
data location;  //create data object

byte tx_buf[sizeof(location)] = {0};  //buffer to send location

//Accelerometer setup
Adafruit_LIS3DH lis = Adafruit_LIS3DH();  //Create acc object

void setup()
{
  DEBUG_BEGIN(115200);
  //Accelerometer setup 
  lis.begin(0x18);  //Start the accelerometer object
  lis.setRange(LIS3DH_RANGE_16_G);  //Set RANGE for accelerometer 2, 4, 8 or 16 G!

  //Radio setup
  radioSetup();
}

void loop()
{
  //Get new accelerometer reading
  lis.read();

  //Stick in location object
  location.x = lis.x;
  location.y = lis.y;

  memcpy(tx_buf, &location, sizeof(location));  //copy location to transmit buffer

  //Send location object
  nrf24.send((uint8_t *)tx_buf, sizeof(location));
  nrf24.waitPacketSent();
}
  