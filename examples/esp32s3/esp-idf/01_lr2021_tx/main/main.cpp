/*
  RadioLib LR2021 Transmit with Interrupts Example

  This example transmits LoRa packets with one second delays
  between them. Each packet contains up to 256 bytes
  of data, in the form of:
  - Arduino String
  - null-terminated char array (C-string)
  - arbitrary binary data (byte array)

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
EspHal* hal = new EspHal(CLK_PIN, MISO_PIN, MOSI_PIN);

// now we can create the radio module
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

static const char *TAG = "lr2021_tx";

volatile bool transmittedFlag = false;
int transmissionState = RADIOLIB_ERR_NONE;

void setFlag(void) {
  transmittedFlag = true;
}

extern "C" void app_main(void) {
  radio.irqDioNum = 11;
  radio.XTAL = true;

  ESP_LOGI(TAG, "[LR2021] Initializing...");
  int state = radio.begin();

  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "[LR2021] Init failed, code: %d", state);
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  ESP_LOGI(TAG, "[LR2021] Init successful!");

  radio.setPacketSentAction(setFlag);

  // LoRa configuration
  radio.setFrequency(868.0);
  radio.setOutputPower(22);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setSyncWord(RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE);
  radio.setPreambleLength(8);

  int count = 0;
  char sendBuffer[64];

  // Send first packet

  transmissionState = radio.startTransmit("Hello World!");

  // Main loop
  while (1) {
    if (transmittedFlag) {
      transmittedFlag = false;

      if (transmissionState == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "[LR2021] Packet sent successfully");
      } else {
        ESP_LOGE(TAG, "[LR2021] Send failed, code: %d", transmissionState);
      }

      radio.finishTransmit();

      // 1-second delay
      vTaskDelay(pdMS_TO_TICKS(1000));

      // Send next packet
      snprintf(sendBuffer, sizeof(sendBuffer), "Hello World! #%d", count++);
      transmissionState = radio.startTransmit(sendBuffer);
      ESP_LOGI(TAG, "[LR2021] Sending next packet...");
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}