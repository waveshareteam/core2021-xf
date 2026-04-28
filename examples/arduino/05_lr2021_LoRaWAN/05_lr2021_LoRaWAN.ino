/*
  RadioLib LoRaWAN Starter Example
  For Arduino UNO R4 - EEPROM Version
*/

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_UNO_R3_MINIMA) || defined(__AVR_ATmega328P__)
  #error "ERROR: This LoRaWAN firmware only supports Arduino UNO R4! Please switch to Arduino UNO R4 board."
#endif

#include "config.h"
#include <EEPROM.h>

// ===================== EEPROM Storage Addresses =====================
#define EEPROM_ADDR_DEV_NONCES  0
#define EEPROM_ADDR_SESSION     (EEPROM_ADDR_DEV_NONCES + RADIOLIB_LORAWAN_NONCES_BUF_SIZE)

// ===================== Helper Functions =====================
void saveLoRaWANState() {
  uint8_t* nonces = node.getBufferNonces();
  uint8_t* session = node.getBufferSession();

  // Save nonces
  for (size_t i = 0; i < RADIOLIB_LORAWAN_NONCES_BUF_SIZE; i++) {
    EEPROM.write(EEPROM_ADDR_DEV_NONCES + i, nonces[i]);
  }

  // Save session
  for (size_t i = 0; i < RADIOLIB_LORAWAN_SESSION_BUF_SIZE; i++) {
    EEPROM.write(EEPROM_ADDR_SESSION + i, session[i]);
  }

  Serial.println(F("[EEPROM] State saved"));
}

void restoreLoRaWANState() {
  // Restore nonces
  uint8_t nonceBuf[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
  for (size_t i = 0; i < RADIOLIB_LORAWAN_NONCES_BUF_SIZE; i++) {
    nonceBuf[i] = EEPROM.read(EEPROM_ADDR_DEV_NONCES + i);
  }
  node.setBufferNonces(nonceBuf);
  Serial.println(F("[EEPROM] Nonce restored"));

  // Restore session (use dedicated buffer to avoid overflow)
  uint8_t sessBuf[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
  for (size_t i = 0; i < RADIOLIB_LORAWAN_SESSION_BUF_SIZE; i++) {
    sessBuf[i] = EEPROM.read(EEPROM_ADDR_SESSION + i);
  }
  node.setBufferSession(sessBuf);
  Serial.println(F("[EEPROM] Session restored"));
}

void printHex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}

void printAscii(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    Serial.write((data[i] >= 32 && data[i] <= 126) ? data[i] : '.');
  }
  Serial.println();
}

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);
  delay(5000);

  Serial.println(F("\n[Setup] Starting Arduino UNO R4 LoRaWAN"));

  EEPROM.begin();
  SPI.begin();

  radio.irqDioNum = 11;
  radio.XTAL = true;
  
  Serial.println(F("[Radio] Initializing..."));
  int state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Radio init failed"), state, true);

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Node init failed"), state, true);

  restoreLoRaWANState();

  Serial.println(F("[LoRaWAN] Joining network..."));
  state = node.activateOTAA();

  const bool joinFailed = (state != RADIOLIB_LORAWAN_NEW_SESSION) &&
                          (state != RADIOLIB_LORAWAN_SESSION_RESTORED);
  debug(joinFailed, F("Join failed"), state, true);

  saveLoRaWANState();
  Serial.println(F("[LoRaWAN] Ready!\n"));
}

// ===================== Main Loop =====================
void loop() {
  Serial.println(F("[LoRaWAN] Sending uplink..."));

  const uint8_t val1 = radio.random(100);
  const uint16_t val2 = radio.random(2000);
  const uint8_t uplinkPayload[3] = {val1, highByte(val2), lowByte(val2)};

  uint8_t downlink[64];
  size_t downlinkLen = sizeof(downlink);

  const int16_t state = node.sendReceive(
    uplinkPayload,
    sizeof(uplinkPayload),
    1,
    downlink,
    &downlinkLen
  );

  debug(state < RADIOLIB_ERR_NONE, F("sendReceive error"), state, false);

  if (state >= 0) {
    saveLoRaWANState();
  }

  if (state > 0) {
    Serial.println(F("[LoRaWAN] Downlink received"));
    Serial.print(F("RX Window: ")); Serial.println(state);
    Serial.print(F("Length: ")); Serial.println(downlinkLen);
    Serial.print(F("HEX: ")); printHex(downlink, downlinkLen);
    Serial.print(F("ASCII: ")); printAscii(downlink, downlinkLen);
  } else {
    Serial.println(F("[LoRaWAN] No downlink received"));
  }

  Serial.print(F("[Timer] Next uplink in "));
  Serial.print(uplinkIntervalSeconds);
  Serial.println(F("s\n"));

  delay(uplinkIntervalSeconds * 1000UL);
}