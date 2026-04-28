/*
  RadioLib LR2021 Direct Transmit Example

  This example enables LR2021 LoRa module in continuous direct transmit mode at fixed frequency.
  Direct mode allows constant carrier wave transmission without packetized data.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr2021---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "EspHal.h"

#define OUT_HZ 868000000UL

// create a new instance of the HAL class
EspHal* hal = new EspHal(GPIO_SPI_CLK, GPIO_SPI_MISO, GPIO_SPI_MOSI);

// now we can create the radio module
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

static const char *TAG = "lr2021_tx_cw";

// the entry point for the program
// it must be declared as "extern C" because the compiler assumes this will be a C function
extern "C" void app_main(void) {

  // LR2021 allows to use any DIO pin as the interrupt
  // as an example, we set DIO10 to be the IRQ
  // this has to be done prior to calling begin()!
  radio.irqDioNum = 11;
  radio.XTAL = true;

  // initialize LR2021 with default settings
  ESP_LOGI(TAG, "[LR2021] Initializing ... ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "success!");
  } else {
    ESP_LOGI(TAG, "failed, code %d", state);
    while (true) { hal->delay(10); }
  }

  radio.setOutputPower(22);
  radio.transmitDirect(OUT_HZ);

  while (1)
  {
    hal->delay(1000);
  }
}
