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

volatile bool receivedFlag = false;

void setFlag(void) {
  receivedFlag = true;
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
  // when packet reception is finished
  radio.setPacketReceivedAction(setFlag);

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

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.scanChannel();

  // Main loop
  uint8_t rxBuf[256];
  size_t len;

  // loop forever
  for(;;) {
    if (receivedFlag) {
      receivedFlag = false;
      memset(rxBuf, 0, sizeof(rxBuf));
      len = radio.getPacketLength();
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
