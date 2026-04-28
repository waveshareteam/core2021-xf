/*
   RadioLib Non-Arduino Raspberry Pi Example

   This example shows how to use RadioLib without Arduino.
   In this case, a Raspberry Pi with WaveShare SX1302 LoRaWAN Hat
   using the lgpio library
   https://abyz.me.uk/lg/lgpio.html

   Can be used as a starting point to port RadioLib to any platform!
   See this API reference page for details on the RadioLib hardware abstraction
   https://jgromes.github.io/RadioLib/class_hal.html

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "hal/RPi/PiHal.h"

#define OUT_HZ 868000000UL

#define SPI_FREQ_HZ    8 * 1000 * 1000
#define NSS_PIN   25
#define IRQ_PIN   17
#define NRST_PIN  22
#define BUSY_PIN  24

// create a new instance of the HAL class
// use SPI channel 0
// The CS of LR2021 cannot use CE0; 
// it needs to be replaced with another option; 
// otherwise, communication will not be possible.
PiHal* hal = new PiHal(0, SPI_FREQ_HZ);

// now we can create the radio module
// NSS pin:   25
// DIO1 pin:  17
// NRST pin:  22
// BUSY pin:  24
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

// the entry point for the program
int main(int argc, char** argv) {
  radio.XTAL = true;
  // initialize just like with Arduino
  printf("[LR2021] Initializing ... \r\n");
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d\n", state);
    return(1);
  }
  printf("success!\n");

  radio.setOutputPower(22);
  radio.transmitDirect(OUT_HZ);

  return(0);
}
