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

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// uncomment the following only on one
// of the nodes to initiate the pings
// #define INITIATING_NODE

// create a new instance of the HAL class
EspHal* hal = new EspHal(GPIO_SPI_CLK, GPIO_SPI_MISO, GPIO_SPI_MOSI);

// now we can create the radio module
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

static const char *TAG = "lr2021_pingpong";

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
  ESP_LOGI(TAG, "[LR2021] Sending first packet ...");
  transmissionState = radio.startTransmit("Hello World!");
  transmitFlag = true;
#else
  // start listening for LoRa packets on this node
  ESP_LOGI(TAG, "[LR2021] Starting to listen ...");
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "success!");
  } else {
    ESP_LOGI(TAG, "failed, code %d", state);
    while (true) { hal->delay(10); }
  }
#endif

  while (1)
  {
    // check if the previous operation finished
    if(operationDone) {
      // reset flag
      operationDone = false;
      if(transmitFlag) {
        // the previous operation was transmission, listen for response
        // print the result
        if (transmissionState == RADIOLIB_ERR_NONE) {
          // packet was successfully sent
          ESP_LOGI(TAG, "transmission finished!");
        } else {
          ESP_LOGI(TAG, "failed, code %d", transmissionState);
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
          ESP_LOGI(TAG, "[LR2021] Received packet!");

          // print data of the packet
          char strBuf[257] = {0};
          memcpy(strBuf, rxBuffer, rxLen);
          ESP_LOGI(TAG, "[LR2021] Data: %s", strBuf);

          // print RSSI (Received Signal Strength Indicator)
          ESP_LOGI(TAG, "[LR2021] RSSI: %.1f dBm", radio.getRSSI());

          // print SNR (Signal-to-Noise Ratio)
          ESP_LOGI(TAG, "[LR2021] SNR: %.1f dB", radio.getSNR());
        }

        // wait a second before transmitting again
        hal->delay(1000);

        // send another one
        ESP_LOGI(TAG, "[LR2021] Sending another packet ...");
        transmissionState = radio.startTransmit("Hello World!");
        transmitFlag = true;
      }
    }

    hal->delay(5);
  }
}