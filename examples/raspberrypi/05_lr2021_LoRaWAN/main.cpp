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

#include "config.h"
#include <iostream>
#include <fstream>

static const char* NVS_FILE = "lorawan_state.bin";
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

LoRaWANNode node(&radio, &Region, subBand);

// =====================================================
// Save LoRaWAN state
// =====================================================
void saveLoRaWANState() {

    std::ofstream file(NVS_FILE, std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        std::cout << "[NVS] Save failed\n";
        return;
    }

    // Save Nonces
    file.write(
        (char*)node.getBufferNonces(),
        RADIOLIB_LORAWAN_NONCES_BUF_SIZE
    );

    // Save Session
    file.write(
        (char*)node.getBufferSession(),
        RADIOLIB_LORAWAN_SESSION_BUF_SIZE
    );

    file.close();

    std::cout << "[NVS] State saved\n";
}

// =====================================================
// Restore LoRaWAN state
// =====================================================
void restoreLoRaWANState() {

    std::ifstream file(NVS_FILE, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "[NVS] No saved state\n";
        return;
    }

    uint8_t nonceBuf[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    uint8_t sessBuf[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

    file.read(
        (char*)nonceBuf,
        RADIOLIB_LORAWAN_NONCES_BUF_SIZE
    );

    if (file.gcount() == RADIOLIB_LORAWAN_NONCES_BUF_SIZE) {
        node.setBufferNonces(nonceBuf);
        std::cout << "[NVS] Nonce restored\n";
    }

    file.read(
        (char*)sessBuf,
        RADIOLIB_LORAWAN_SESSION_BUF_SIZE
    );

    if (file.gcount() == RADIOLIB_LORAWAN_SESSION_BUF_SIZE) {
        node.setBufferSession(sessBuf);
        std::cout << "[NVS] Session restored\n";
    }

    file.close();
}

/**
 * @brief  Print byte array as HEX string
 * @param  data  Pointer to byte array
 * @param  len   Length of the array
 */
void printHex(const uint8_t* data, size_t len) {
    printf("[HEX] ");
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
    printf("[ASCII] ");
    for (size_t i = 0; i < len; i++) {
        printf("%c", (data[i] >= 32 && data[i] <= 126) ? data[i] : '.');
    }
    printf("\n");
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
    printf("%s - %s (%d)\n", message, stateDecode(state), state);
    while (halt) { hal->delay(1); }
  }
}

void arrayDump(uint8_t *buffer, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    printf("%02X", buffer[i]);
  }
  printf("\n");
}

// the entry point for the program
int main(int argc, char** argv) {

  printf("[Setup] Starting...\n");

  // Radio IRQ & XTAL config
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


  // Initialize OTAA node
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        printf("Node init failed, code: %d\n", state);
        while (1) hal->delay(10);
    }

    // Restore previous session
    restoreLoRaWANState();

    // Join LoRaWAN network
    printf("[LoRaWAN] Joining network...\n");
    state = node.activateOTAA();

    // Check join status
    const bool joinFailed = (state != RADIOLIB_LORAWAN_NEW_SESSION) &&
                            (state != RADIOLIB_LORAWAN_SESSION_RESTORED);
    if (joinFailed) {
        printf("Join failed, code: %d\n", state);
        while (1) hal->delay(10);
    }

    // Save state after successful join
    saveLoRaWANState();
    printf("[LoRaWAN] Ready!\n");

    for (;; ) {
        printf("[LoRaWAN] Sending uplink...\n");

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
            printf("sendReceive error, code: %d\n", state);
        }

        // Save state on successful transmission
        if (state >= 0) {
            saveLoRaWANState();
        }

        // Process downlink if received
        if (state > 0) {
            printf("[LoRaWAN] Downlink received\n");
            printf("RX Window: %d\n", state);
            printf("Length: %d\n", downlinkLen);
            printf("HEX: ");
            printHex(downlink, downlinkLen);
            printf("ASCII: ");
            printAscii(downlink, downlinkLen);
        } else {
            printf("[LoRaWAN] No downlink received\n");
        }

        // Wait for next transmission interval
        printf("[Timer] Next uplink in %d s\n\n", uplinkIntervalSeconds);
        hal->delay(30 * 1000);
    }
  return(0);
}
