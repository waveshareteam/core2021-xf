/*
  RadioLib LR2021 Receive with Interrupts Example
  Ported for ESP-IDF - Native Version

  This example listens for LoRa transmissions and tries to
  receive them. Once a packet is received, an interrupt is
  triggered. To successfully receive data, the following
  settings have to be the same on both transmitter
  and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr2021---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <RadioLib.h>
#include "EspHal.h"

// create a new instance of the HAL class
EspHal* hal = new EspHal(GPIO_SPI_CLK, GPIO_SPI_MISO, GPIO_SPI_MOSI);

// now we can create the radio module
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

static const char *TAG = "lr2021_rx";

volatile bool receivedFlag = false;

void setFlag(void) {
  receivedFlag = true;
}

extern "C" void app_main(void) {
  radio.irqDioNum = 11;
  radio.XTAL = true;

  ESP_LOGI(TAG, "[LR2021] Initializing...");
  int state = radio.begin();

  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "[LR2021] Init failed, code: %d", state);
    while (1) {
      hal->delay(10);
    }
  }
  ESP_LOGI(TAG, "[LR2021] Init successful!");

  // Set callback for received packet
  radio.setPacketReceivedAction(setFlag);

  // LoRa configuration (MUST match transmitter!)
  radio.setFrequency(868.0);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setSyncWord(RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE);
  radio.setPreambleLength(8);

  // Start receiving
  ESP_LOGI(TAG, "[LR2021] Starting to listen...");
  state = radio.startReceive();

  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "[LR2021] Listen failed, code: %d", state);
    while (1) {
      hal->delay(10);
    }
  }
  ESP_LOGI(TAG, "[LR2021] Listening for packets...");

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

  while (1) {
    if (receivedFlag) {
      receivedFlag = false;
      len = radio.getPacketLength();
      int state = radio.readData(rxBuf, len);

      if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "[LR2021] Packet received!");
        ESP_LOGI(TAG, "[LR2021] Length: %d bytes", len);
        ESP_LOGI(TAG, "[LR2021] RSSI: %.1f dBm", radio.getRSSI());
        ESP_LOGI(TAG, "[LR2021] SNR:  %.1f dB", radio.getSNR());

        // Print raw data as hex
        ESP_LOGI(TAG, "[LR2021] Data HEX: ");
        for (size_t i = 0; i < len; i++) {
          printf("%02X ", rxBuf[i]);
        }
        printf("\n");

        // Print as string
        char strBuf[257];
        memcpy(strBuf, rxBuf, len);
        strBuf[len] = 0;
        ESP_LOGI(TAG, "[LR2021] Data STR: %s", strBuf);

      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        ESP_LOGW(TAG, "[LR2021] CRC error!");

      } else {
        ESP_LOGE(TAG, "[LR2021] Receive error, code: %d", state);
      }

      // Resume listening
      radio.startReceive();
    }

    hal->delay(10);
  }
}