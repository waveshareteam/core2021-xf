#ifndef _RADIOLIB_EX_LORAWAN_CONFIG_H
#define _RADIOLIB_EX_LORAWAN_CONFIG_H

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "hal/RPi/PiHal.h"

// Radio pin configuration
#define SPI_FREQ_HZ    8 * 1000 * 1000
#define NSS_PIN   25
#define IRQ_PIN   17
#define NRST_PIN  22
#define BUSY_PIN  24

// LoRaWAN parameters
const uint32_t uplinkIntervalSeconds = 5UL * 60UL;

// joinEUI - previous versions of LoRaWAN called this AppEUI
// for development purposes you can use all zeros - see wiki for details
#define RADIOLIB_LORAWAN_JOIN_EUI  0x2cd9bb5cb72b93d3

// the Device EUI & two keys can be generated on the TTN console 
#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0xe812acae83084796
#endif
#ifndef RADIOLIB_LORAWAN_APP_KEY   // Replace with your App Key 
#define RADIOLIB_LORAWAN_APP_KEY   0x09, 0x5e, 0x36, 0x2c, 0x3e, 0x75, 0xf0, 0xff, 0x81, 0xf1, 0x27, 0xf3, 0xd4, 0xbb, 0x17, 0xb0
#endif
#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0x09, 0x5e, 0x36, 0x2c, 0x3e, 0x75, 0xf0, 0xff, 0x81, 0xf1, 0x27, 0xf3, 0xd4, 0xbb, 0x17, 0xb0
#endif

const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;

uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

// Error code to string
const char* stateDecode(const int16_t result);

// Debug & helper
void debug(bool failed, const char* message, int state, bool halt);
void arrayDump(uint8_t *buffer, uint16_t len);

#endif