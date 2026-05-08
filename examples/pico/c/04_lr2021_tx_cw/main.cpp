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

#define OUT_HZ 868000000UL

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

  radio.setOutputPower(22);
  radio.transmitDirect(OUT_HZ);

  return(0);
}
