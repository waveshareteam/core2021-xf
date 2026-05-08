/*
  RadioLib Non-Arduino Raspberry Pi Pico library example

  Licensed under the MIT License

  Copyright (c) 2024 Cameron Goddard

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// define pins to be used
#define SPI_PORT spi1
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_SCK 10

#define RFM_NSS 13
#define RFM_RST 5
#define RFM_IRQ 15
#define RFM_BUSY 14

#include <pico/stdlib.h>

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "hal/RPiPico/PicoHal.h"

// uncomment the following only on one
// of the nodes to initiate the pings
// #define INITIATING_NODE

// create a new instance of the HAL class
PicoHal* hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);

// now we can create the radio module
// NSS pin:  13
// DIO0 pin:  15
// RESET pin:  5
// DIO1 pin:  14
LR2021 radio = new Module(hal, RFM_NSS, RFM_IRQ, RFM_RST, RFM_BUSY);

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

int main() {
  // initialize just like with Arduino
  printf("[LR2021] Initializing ... ");

  radio.irqDioNum = 11;
  radio.XTAL = true;

  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d\n", state);
    return(1);
  }
  printf("success!\n");

  // set the function that will be called
  // when new packet is received
  radio.setIrqAction(setFlag);

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

  // loop forever
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
        memset(rxBuffer, 0, sizeof(rxBuffer));

        size_t rxLen = radio.getPacketLength();
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
