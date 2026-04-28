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

volatile bool receivedFlag = false;

void setFlag(void) {
  receivedFlag = true;
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

  // Start receiving
  printf("[LR2021] Starting to listen...\r\n");
  state = radio.startReceive();

  if (state != RADIOLIB_ERR_NONE) {
    printf("[LR2021] Listen failed, code: %d\n", state);
    while (1) {
      hal->delay(10);
    }
  }
  printf("[LR2021] Listening for packets...\r\n");

  // Main loop
  uint8_t rxBuf[256];
  size_t len;
  
  for(;;) {
    if (receivedFlag) {
      receivedFlag = false;
      len = sizeof(rxBuf);
      int state = radio.readData(rxBuf, len);

      if (state == RADIOLIB_ERR_NONE) {
        printf("[LR2021] Packet received!\r\n");
        printf("[LR2021] Length: %d bytes\n", len);
        printf("[LR2021] RSSI: %.1f dBm\n", radio.getRSSI());
        printf("[LR2021] SNR:  %.1f dB\n", radio.getSNR());

        // Print raw data as hex
        printf("[LR2021] Data HEX: ");
        for (size_t i = 0; i < len; i++) {
          printf("%02X ", rxBuf[i]);
        }
        printf("\n");

        // Print as string
        char strBuf[257];
        memcpy(strBuf, rxBuf, len);
        strBuf[len] = 0;
        printf("[LR2021] Data STR: %s\n\n", strBuf);

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        printf("[LR2021] CRC error!\r\n");

      } else {
        printf("[LR2021] Receive error, code: %d\n", state);
      }

      // Resume listening
      radio.startReceive();
    }

    hal->delay(10);
  }

  return(0);
}
