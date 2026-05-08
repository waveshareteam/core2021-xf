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
#define INITIATING_NODE

// create a new instance of the HAL class
PicoHal* hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);

// now we can create the radio module
// NSS pin:  13
// DIO0 pin:  15
// RESET pin:  5
// DIO1 pin:  14
LR2021 radio = new Module(hal, RFM_NSS, RFM_IRQ, RFM_RST, RFM_BUSY);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
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
