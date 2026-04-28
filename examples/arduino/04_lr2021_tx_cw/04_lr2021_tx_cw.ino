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

// Transmit frequency: 868 MHz (ISM band)
#define OUT_HZ 868000000UL

// LR2021 pin configuration:
// NSS pin:   10
// IRQ pin:   2
// NRST pin:  3
// BUSY pin:  9
LR2021 radio = new Module(10, 2, 3, 9);

void setup() {
  Serial.begin(115200);
  delay(3000);

  // enable external crystal
  radio.XTAL = true;

  // initialize LR2021 with default settings
  Serial.print(F("[LR2021] Initializing... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("[LR2021] Init failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set transmit power to 22 dBm
  radio.setOutputPower(22);

  // start direct transmit at fixed frequency
  Serial.print(F("[LR2021] Starting direct transmit... "));
  state = radio.transmitDirect(OUT_HZ);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("started successfully!"));
  } else {
    Serial.print(F("[LR2021] Transmit failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void loop() {
  // direct transmit runs continuously, no loop action needed
  delay(1000);
}