#ifndef _RADIOLIB_EX_LORAWAN_CONFIG_H
#define _RADIOLIB_EX_LORAWAN_CONFIG_H

#include <RadioLib.h>
// include the hardware abstraction layer
#include "hal/RPiPico/PicoHal.h"

// define pins to be used
#define SPI_PORT spi1
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_SCK 10

#define RFM_NSS 13
#define RFM_RST 5
#define RFM_IRQ 15
#define RFM_BUSY 14

// LoRaWAN parameters
const uint32_t uplinkIntervalSeconds = 5UL * 60UL;

// joinEUI - previous versions of LoRaWAN called this AppEUI
// for development purposes you can use all zeros - see wiki for details
#define RADIOLIB_LORAWAN_JOIN_EUI  0x0000000000000000

// the Device EUI & two keys can be generated on the TTN console
#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0x---------------
#endif
#ifndef RADIOLIB_LORAWAN_APP_KEY   // Replace with your App Key
#define RADIOLIB_LORAWAN_APP_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--
#endif
#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--
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
