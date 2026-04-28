/*
  RadioLib LoRaWAN Starter Example
  Optimized version (same functionality)
*/

#include "config.h"
#include <Preferences.h>

Preferences prefs;

// ===================== NVS Storage Keys =====================
static const char* NVS_NAMESPACE     = "lorawan";
static const char* NVS_KEY_DEV_NONCE = "nonce";
static const char* NVS_KEY_SESSION   = "sess";

// ===================== Helper Functions =====================
/**
 * @brief  Save LoRaWAN nonces and session data to NVS
 */
void saveLoRaWANState() {
  prefs.putBytes(
    NVS_KEY_DEV_NONCE,
    node.getBufferNonces(),
    RADIOLIB_LORAWAN_NONCES_BUF_SIZE
  );

  prefs.putBytes(
    NVS_KEY_SESSION,
    node.getBufferSession(),
    RADIOLIB_LORAWAN_SESSION_BUF_SIZE
  );
}

/**
 * @brief  Restore LoRaWAN state from NVS
 */
void restoreLoRaWANState() {
  uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];

  // Restore device nonces
  if (prefs.isKey(NVS_KEY_DEV_NONCE)) {
    prefs.getBytes(NVS_KEY_DEV_NONCE, buffer, sizeof(buffer));
    node.setBufferNonces(buffer);
    Serial.println(F("[NVS] Nonce restored"));
  }

  // Restore session data
  if (prefs.isKey(NVS_KEY_SESSION)) {
    prefs.getBytes(NVS_KEY_SESSION, buffer, sizeof(buffer));
    node.setBufferSession(buffer);
    Serial.println(F("[NVS] Session restored"));
  }
}

/**
 * @brief  Print byte array as HEX string
 * @param  data  Pointer to byte array
 * @param  len   Length of the array
 */
void printHex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}

/**
 * @brief  Print byte array as ASCII (non-printable = .)
 * @param  data  Pointer to byte array
 * @param  len   Length of the array
 */
void printAscii(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    Serial.write((data[i] >= 32 && data[i] <= 126) ? data[i] : '.');
  }
  Serial.println();
}

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(5000);

  Serial.println(F("\n[Setup] Starting..."));

  // Initialize NVS
  prefs.begin(NVS_NAMESPACE, false);

  // Initialize SPI bus
  SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, -1);
  radio.irqDioNum = 11;
  radio.XTAL = true;
  
  // Initialize radio module
  Serial.println(F("[Radio] Initializing..."));
  int state = radio.begin();

  debug(state != RADIOLIB_ERR_NONE, F("Radio init failed"), state, true);

  // Initialize OTAA node
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Node init failed"), state, true);

  // Restore previous session
  restoreLoRaWANState();

  // Join LoRaWAN network
  Serial.println(F("[LoRaWAN] Joining network..."));
  state = node.activateOTAA();

  // Check join status
  const bool joinFailed = (state != RADIOLIB_LORAWAN_NEW_SESSION) &&
                          (state != RADIOLIB_LORAWAN_SESSION_RESTORED);
  debug(joinFailed, F("Join failed"), state, true);

  // Save state after successful join
  saveLoRaWANState();
  Serial.println(F("[LoRaWAN] Ready!\n"));
}

// ===================== Main Loop =====================
void loop() {
  Serial.println(F("[LoRaWAN] Sending uplink..."));

  // Create uplink payload
  const uint8_t val1 = radio.random(100);
  const uint16_t val2 = radio.random(2000);
  const uint8_t uplinkPayload[3] = {val1, highByte(val2), lowByte(val2)};

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
  debug(state < RADIOLIB_ERR_NONE, F("sendReceive error"), state, false);

  // Save state on successful transmission
  if (state >= 0) {
    saveLoRaWANState();
  }

  // Process downlink if received
  if (state > 0) {
    Serial.println(F("[LoRaWAN] Downlink received"));
    Serial.print(F("RX Window: ")); Serial.println(state);
    Serial.print(F("Length: ")); Serial.println(downlinkLen);
    Serial.print(F("HEX: ")); printHex(downlink, downlinkLen);
    Serial.print(F("ASCII: ")); printAscii(downlink, downlinkLen);
  } else {
    Serial.println(F("[LoRaWAN] No downlink received"));
  }

  // Wait for next transmission interval
  Serial.print(F("[Timer] Next uplink in "));
  Serial.print(uplinkIntervalSeconds);
  Serial.println(F("s\n"));

  delay(uplinkIntervalSeconds * 1000UL);
}