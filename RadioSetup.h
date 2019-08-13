#include <SPI.h>
#include <RH_NRF24.h>

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

RH_NRF24 nrf24(RADIOCEPIN, RADIOCSNPIN); // create radio object

bool radioSetup()
{
  if (!nrf24.init())
  {
      DEBUG_PRINTLN("Failed init");
      return false;
  }
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
  {
      DEBUG_PRINTLN("Failed setChannel");
      return false;
  }
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
  {
      DEBUG_PRINTLN("Failed setRF");
      return false;
  }

  DEBUG_PRINTLN("Radio setup successful");
  return true;   
}