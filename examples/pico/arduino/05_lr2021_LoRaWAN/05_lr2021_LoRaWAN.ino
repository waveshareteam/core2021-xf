/*
  RadioLib LoRaWAN Starter Example
  RP2040 / RP2350 Compatible Version

  Changes:
  - Removed ESP32 Preferences.h
  - Uses EEPROM emulation (works on RP2040 / RP2350 Arduino core)
  - Same LoRaWAN session persistence logic
*/

#include "config.h"
#include <EEPROM.h>

// =======================================================
// EEPROM Layout
// =======================================================
#define EEPROM_SIZE           256
#define EEPROM_MAGIC_ADDR     0
#define EEPROM_DATA_ADDR      4
#define EEPROM_MAGIC_VALUE    0x5AA55AA5

// ===================== Helper Functions =====================
void saveLoRaWANState() {
  int addr = EEPROM_DATA_ADDR;

  // Save nonces
  const uint8_t* nonces = node.getBufferNonces();
  for (size_t i = 0; i < RADIOLIB_LORAWAN_NONCES_BUF_SIZE; i++) {
    EEPROM.write(addr++, nonces[i]);
  }

  // Save session
  const uint8_t* session = node.getBufferSession();
  for (size_t i = 0; i < RADIOLIB_LORAWAN_SESSION_BUF_SIZE; i++) {
    EEPROM.write(addr++, session[i]);
  }

  // Write magic flag
  uint32_t magic = EEPROM_MAGIC_VALUE;
  EEPROM.put(EEPROM_MAGIC_ADDR, magic);

  EEPROM.commit();

  Serial.println(F("[EEPROM] State saved"));
}

void restoreLoRaWANState() {
  uint32_t magic = 0;
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);

  if (magic != EEPROM_MAGIC_VALUE) {
    Serial.println(F("[EEPROM] No saved session"));
    return;
  }

  int addr = EEPROM_DATA_ADDR;

  // Restore nonces
  uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
  for (size_t i = 0; i < sizeof(nonces); i++) {
    nonces[i] = EEPROM.read(addr++);
  }
  node.setBufferNonces(nonces);

  // Restore session
  uint8_t session[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
  for (size_t i = 0; i < sizeof(session); i++) {
    session[i] = EEPROM.read(addr++);
  }
  node.setBufferSession(session);

  Serial.println(F("[EEPROM] Session restored"));
}

// =======================================================
// Print Helpers
// =======================================================
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

// =======================================================
// Setup
// =======================================================
void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println(F("\n[Setup] Starting..."));

  EEPROM.begin(EEPROM_SIZE);

  // Initialize SPI for RP2040
  SPI1.setSCK(CLK_PIN);
  SPI1.setRX(MISO_PIN);
  SPI1.setTX(MOSI_PIN);
  SPI1.begin();

  radio.irqDioNum = 11;
  radio.XTAL = true;

  Serial.println(F("[Radio] Initializing..."));
  int state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Radio init failed"), state, true);

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Node init failed"), state, true);

  // Restore previous session
  restoreLoRaWANState();

  // Join network
  Serial.println(F("[LoRaWAN] Joining network..."));
  state = node.activateOTAA();

  bool joinFailed =
    (state != RADIOLIB_LORAWAN_NEW_SESSION) &&
    (state != RADIOLIB_LORAWAN_SESSION_RESTORED);

  debug(joinFailed, F("Join failed"), state, true);

  saveLoRaWANState();

  Serial.println(F("[LoRaWAN] Ready!\n"));
}

// =======================================================
// Loop
// =======================================================
void loop() {
  Serial.println(F("[LoRaWAN] Sending uplink..."));

  uint8_t val1 = radio.random(100);
  uint16_t val2 = radio.random(2000);

  uint8_t uplinkPayload[3] = {
    val1,
    highByte(val2),
    lowByte(val2)
  };

  uint8_t downlink[64];
  size_t downlinkLen = sizeof(downlink);

  int16_t state = node.sendReceive(
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
    Serial.print(F("RX Window: "));
    Serial.println(state);

    Serial.print(F("Length: "));
    Serial.println(downlinkLen);

    Serial.print(F("HEX: "));
    printHex(downlink, downlinkLen);

    Serial.print(F("ASCII: "));
    printAscii(downlink, downlinkLen);

  } else {
    Serial.println(F("[LoRaWAN] No downlink received"));
  }

  Serial.print(F("[Timer] Next uplink in "));
  Serial.print(uplinkIntervalSeconds);
  Serial.println(F("s\n"));

  delay(uplinkIntervalSeconds * 1000UL);
}