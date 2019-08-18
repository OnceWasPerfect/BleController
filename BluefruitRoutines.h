#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

#define MY_DEBUG 0

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

//Debug output routines
#if (MY_DEBUG)
  #define MESSAGE(m) Serial.println(m);
  #define FATAL(m) {MESSAGE(m); while (1);}
#else
  #define MESSAGE(m) {}
  #define FATAL(m) while (1);
#endif

void initializeBluefruit (void) {
  //Initialize Bluetooth
  if ( !ble.begin(MY_DEBUG))
  {
    FATAL(F("NO BLE?"));
  }
  delay(50);
  //Rename device
  if (! ble.sendCommandCheckOK("AT+GAPDEVNAME=Controller" ) ) {
    FATAL(F("err:rename fail"));
  }
  delay(50);
  //Enable HID keyboard
  if(!ble.sendCommandCheckOK("AT+BleHIDEn=On" )) {
    FATAL(F("err:enable Kb"));
  }
  delay(50);
  //Add or remove service requires a reset
  if (! ble.reset() ) {
    FATAL(F("err:SW reset"));
  }
  delay(50);

  //ble.sendCommandCheckOK("AT+GAPCONNECTABLE=1");

  //ble.factoryReset();

}


