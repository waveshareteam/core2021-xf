/*
  RadioLib LR2021 Direct Transmit Example

  This example enables LR2021 LoRa module in continuous direct transmit mode at fixed frequency.
  Direct mode allows constant carrier wave transmission without packetized data.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr2021---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the hardware abstraction layer
#include "EspHal.h"

// create a new instance of the HAL class
EspHal* hal = new EspHal(CLK_PIN, MISO_PIN, MOSI_PIN);

// now we can create the radio module
LR2021 radio = new Module(hal, NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN);

LoRaWANNode node(&radio, &Region, subBand);

static const char *TAG = "main";

nvs_handle_t nvsHandle;

// ===================== NVS Storage Keys =====================
static const char* NVS_NAMESPACE     = "lorawan";
static const char* NVS_KEY_DEV_NONCE = "nonce";
static const char* NVS_KEY_SESSION   = "sess";

// ===================== Helper Functions =====================
/**
 * @brief  Save LoRaWAN nonces and session data to NVS
 */
void saveLoRaWANState() {
    nvs_set_blob(nvsHandle, NVS_KEY_DEV_NONCE,
                 node.getBufferNonces(),
                 RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

    nvs_set_blob(nvsHandle, NVS_KEY_SESSION,
                 node.getBufferSession(),
                 RADIOLIB_LORAWAN_SESSION_BUF_SIZE);

    nvs_commit(nvsHandle);
    ESP_LOGI(TAG, "[NVS] State saved");
}

/**
 * @brief  Restore LoRaWAN state from NVS
 */
void restoreLoRaWANState() {
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    size_t bufSize = sizeof(buffer);

    // Restore device nonces
    esp_err_t ret = nvs_get_blob(nvsHandle, NVS_KEY_DEV_NONCE, buffer, &bufSize);
    if (ret == ESP_OK) {
        node.setBufferNonces(buffer);
        ESP_LOGI(TAG, "[NVS] Nonce restored");
    }

    // Restore session data
    bufSize = sizeof(buffer);
    ret = nvs_get_blob(nvsHandle, NVS_KEY_SESSION, buffer, &bufSize);
    if (ret == ESP_OK) {
        node.setBufferSession(buffer);
        ESP_LOGI(TAG, "[NVS] Session restored");
    }
}

/**
 * @brief  Print byte array as HEX string
 * @param  data  Pointer to byte array
 * @param  len   Length of the array
 */
void printHex(const uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "[HEX] ");
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

/**
 * @brief  Print byte array as ASCII (non-printable = .)
 * @param  data  Pointer to byte array
 * @param  len   Length of the array
 */
void printAscii(const uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "[ASCII] ");
    for (size_t i = 0; i < len; i++) {
        putchar((data[i] >= 32 && data[i] <= 126) ? data[i] : '.');
    }
    putchar('\n');
}

// ===================== config.h helper functions =====================
const char* stateDecode(const int16_t result) {
  switch (result) {
    case RADIOLIB_ERR_NONE:                  return "ERR_NONE";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:        return "ERR_CHIP_NOT_FOUND";
    case RADIOLIB_ERR_PACKET_TOO_LONG:       return "ERR_PACKET_TOO_LONG";
    case RADIOLIB_ERR_RX_TIMEOUT:            return "ERR_RX_TIMEOUT";
    case RADIOLIB_ERR_MIC_MISMATCH:          return "ERR_MIC_MISMATCH";
    case RADIOLIB_ERR_INVALID_BANDWIDTH:     return "ERR_INVALID_BANDWIDTH";
    case RADIOLIB_ERR_INVALID_SPREADING_FACTOR: return "ERR_INVALID_SPREADING_FACTOR";
    case RADIOLIB_ERR_INVALID_CODING_RATE:   return "ERR_INVALID_CODING_RATE";
    case RADIOLIB_ERR_INVALID_FREQUENCY:     return "ERR_INVALID_FREQUENCY";
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:  return "ERR_INVALID_OUTPUT_POWER";
    case RADIOLIB_ERR_NETWORK_NOT_JOINED:    return "ERR_NETWORK_NOT_JOINED";
    case RADIOLIB_ERR_DOWNLINK_MALFORMED:    return "ERR_DOWNLINK_MALFORMED";
    case RADIOLIB_LORAWAN_SESSION_RESTORED:  return "SESSION_RESTORED";
    case RADIOLIB_LORAWAN_NEW_SESSION:       return "NEW_SESSION";
    default: return "See RadioLib status codes";
  }
}

void debug(bool failed, const char* message, int state, bool halt) {
  if (failed) {
    ESP_LOGE(TAG, "%s - %s (%d)", message, stateDecode(state), state);
    while (halt) { vTaskDelay(pdMS_TO_TICKS(1)); }
  }
}

void arrayDump(uint8_t *buffer, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    printf("%02X", buffer[i]);
  }
  printf("\n");
}

// ===================== Main Entry (app_main) =====================
extern "C" void app_main(void) {

    // Initialize NVS
    nvs_flash_init();
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle);

    ESP_LOGI(TAG, "\n[Setup] Starting...");

    // Radio IRQ & XTAL config
    radio.irqDioNum = 11;
    radio.XTAL = true;

    // Initialize radio module
    ESP_LOGI(TAG, "[Radio] Initializing...");
    int state = radio.begin();

    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Radio init failed, code: %d", state);
        while (1) vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Initialize OTAA node
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Node init failed, code: %d", state);
        while (1) vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Restore previous session
    restoreLoRaWANState();

    // Join LoRaWAN network
    ESP_LOGI(TAG, "[LoRaWAN] Joining network...");
    state = node.activateOTAA();

    // Check join status
    const bool joinFailed = (state != RADIOLIB_LORAWAN_NEW_SESSION) &&
                            (state != RADIOLIB_LORAWAN_SESSION_RESTORED);
    if (joinFailed) {
        ESP_LOGE(TAG, "Join failed, code: %d", state);
        while (1) vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Save state after successful join
    saveLoRaWANState();
    ESP_LOGI(TAG, "[LoRaWAN] Ready!\n");

    // ===================== Main Loop =====================
    while (1) {
        ESP_LOGI(TAG, "[LoRaWAN] Sending uplink...");

        // Create uplink payload
        const uint8_t val1 = radio.random(100);
        const uint16_t val2 = radio.random(2000);
        const uint8_t uplinkPayload[3] = {val1, (uint8_t)(val2 >> 8), (uint8_t)(val2 & 0xFF)};

        // Downlink receive buffer
        uint8_t downlink[64];
        size_t downlinkLen = sizeof(downlink);

        // Send uplink and wait for downlink
        const int16_t state = node.sendReceive(
            uplinkPayload,
            sizeof(uplinkPayload),
            1,
            downlink,
            &downlinkLen
        );

        // Handle errors
        if (state < RADIOLIB_ERR_NONE) {
            ESP_LOGE(TAG, "sendReceive error, code: %d", state);
        }

        // Save state on successful transmission
        if (state >= 0) {
            saveLoRaWANState();
        }

        // Process downlink if received
        if (state > 0) {
            ESP_LOGI(TAG, "[LoRaWAN] Downlink received");
            ESP_LOGI(TAG, "RX Window: %d", state);
            ESP_LOGI(TAG, "Length: %d", downlinkLen);
            ESP_LOGI(TAG, "HEX: ");
            printHex(downlink, downlinkLen);
            ESP_LOGI(TAG, "ASCII: ");
            printAscii(downlink, downlinkLen);
        } else {
            ESP_LOGI(TAG, "[LoRaWAN] No downlink received");
        }

        // Wait for next transmission interval
        ESP_LOGI(TAG, "[Timer] Next uplink in %d s\n", uplinkIntervalSeconds);
        vTaskDelay(pdMS_TO_TICKS(30 * 1000UL));
    }
}