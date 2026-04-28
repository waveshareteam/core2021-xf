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

#define SPI_Hz    8 * 1000 * 1000
#define NSS_PIN   25
#define IRQ_PIN   17
#define NRST_PIN  22
#define BUSY_PIN  24

// create a new instance of the HAL class
// use SPI channel 0
// The CS of LR2021 cannot use CE0; 
// it needs to be replaced with another option; 
// otherwise, communication will not be possible.
PiHal* hal = new PiHal(0, SPI_Hz);

// now we can create the radio module
// NSS pin:   25
// DIO1 pin:  17
// NRST pin:  22
// BUSY pin:  24
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

// the entry point for the program
int main(int argc, char** argv) {
  radio.irqDioNum = 11;
  radio.XTAL = true;
  // initialize just like with Arduino
  printf("[LR2021] Initializing ... \r\n");
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d\n", state);
    return(1);
  }
  printf("success!\n");

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  radio.setFrequency(868.0);
  radio.setOutputPower(22);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setSyncWord(RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE);
  radio.setPreambleLength(8);

  // start transmitting the first packet
  printf("[LR2021] Sending first packet ... \r\n");
  
  // you can transmit C-string or Arduino string up to
  // 256 characters long
  transmissionState = radio.startTransmit("Hello World!");

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    state = radio.startTransmit(byteArr, 8);
  */

  // loop forever
  int count = 0;
  char sendBuffer[64];
  
  for(;;) {
    // check if the previous transmission finished
    if(transmittedFlag) {
      // reset flag
      transmittedFlag = false;

      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        printf("transmission finished!\n");

      } else {
        printf("failed, code %d\n", transmissionState);

      }

      // clean up after transmission is finished
      // this will ensure transmitter is disabled,
      // RF switch is powered down etc.
      radio.finishTransmit();

      // wait a second before transmitting again
      hal->delay(1000);

      // send another one
      printf("[LR2021] Sending another packet ... \r\n");

      // you can transmit C-string to
      // 256 characters long
      snprintf(sendBuffer, sizeof(sendBuffer), "Hello World! #%d", count++);
      transmissionState = radio.startTransmit(sendBuffer);

      // you can also transmit byte array up to 256 bytes long
      /*
        byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                          0x89, 0xAB, 0xCD, 0xEF};
        transmissionState = radio.startTransmit(byteArr, 8);
      */
    }
  }

  return(0);
}
