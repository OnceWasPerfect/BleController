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

//Accelerometer setup
Adafruit_LIS3DH lis = Adafruit_LIS3DH();  //Create acc object
int location[] = {0, 0};

//Radio setup
RF24 radio(RADIOCEPIN, RADIOCSNPIN); // Set up the radio object CE, CSN
const byte address[6] = "00001";  //The address the radio will work on

void setup()
{
  DEBUG_BEGIN(115200);
  //Accelerometer setup 
  lis.begin(0x18);  //Start the accelerometer object
  lis.setRange(LIS3DH_RANGE_16_G);  //Set RANGE for accelerometer 2, 4, 8 or 16 G!

  //Radio setup
  radio.begin();  //Start the radio object
  radio.openWritingPipe(address);  //Start the writing pipe
  radio.setPALevel(RF24_PA_MIN);  //How strong to send the signal
  radio.stopListening();  //Turn off listening mode so it can transmit
}

void loop()
{
    //Read from accelerometer
    lis.read();  //Get new location data
    location[0] = lis.x;  //store x axis
    location[1] = lis.y;  //store y axis

    //Send data via radio
    radio.write(&location, sizeof(location)); 
    DEBUG_PRINT("x = "); DEBUG_PRINTLN(location[0]);
    DEBUG_PRINT("y = "); DEBUG_PRINTLN(location[1]);
    
    delay(10);  //small delay to let it finish
}
  