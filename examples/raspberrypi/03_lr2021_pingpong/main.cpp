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

// uncomment the following only on one
// of the nodes to initiate the pings
// #define INITIATING_NODE

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

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

void setFlag(void) {
  // we sent or received a packet, set the flag
  operationDone = true;
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

#if defined(INITIATING_NODE)
  // send the first packet on this node
  printf("[LR2021] Sending first packet ... \r\n");
  transmissionState = radio.startTransmit("Hello World!");
  transmitFlag = true;
#else
  // start listening for LoRa packets on this node
  printf("[LR2021] Starting to listen ... \r\n");
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    printf("success!\n");
  } else {
    printf("failed, code %d\n", state);
    while (true) { hal->delay(10); }
  }
#endif
  
  for(;;) {
    // check if the previous operation finished
    if(operationDone) {
      // reset flag
      operationDone = false;
      if(transmitFlag) {
        // the previous operation was transmission, listen for response
        // print the result
        if (transmissionState == RADIOLIB_ERR_NONE) {
          // packet was successfully sent
          printf("transmission finished!\n");
        } else {
          printf("failed, code %d\n", transmissionState);
        }

        // listen for response
        radio.startReceive();
        transmitFlag = false;

      } else {
        // the previous operation was reception
        // print data and send another packet
        uint8_t rxBuffer[256];
        size_t rxLen = sizeof(rxBuffer);
        int state = radio.readData(rxBuffer, rxLen);

        if (state == RADIOLIB_ERR_NONE) {
          // packet was successfully received
          printf("[LR2021] Received packet!\n");

          // print data of the packet
          char strBuf[257] = {0};
          memcpy(strBuf, rxBuffer, rxLen);
          printf("[LR2021] Data: %s\n", strBuf);

          // print RSSI (Received Signal Strength Indicator)
          printf("[LR2021] RSSI: %.1f dBm\n", radio.getRSSI());

          // print SNR (Signal-to-Noise Ratio)
          printf("[LR2021] SNR: %.1f dB\n\n", radio.getSNR());
        }

        // wait a second before transmitting again
        hal->delay(1000);

        // send another one
        printf("[LR2021] Sending another packet ... \r\n");
        transmissionState = radio.startTransmit("Hello World!");
        transmitFlag = true;
      }
    }

    hal->delay(5);
  }

  return(0);
}
