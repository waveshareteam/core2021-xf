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
#define SPI_FREQ_HZ    8 * 1000 * 1000
#define GPIO_SPI_MISO  46
#define GPIO_SPI_MOSI  45
#define GPIO_SPI_CLK   40
#define NSS_PIN   42
#define IRQ_PIN   38
#define NRST_PIN  39
#define BUSY_PIN  41
LR2021 radio = new Module(NSS_PIN, IRQ_PIN, NRST_PIN, BUSY_PIN, SPI, SPISettings(SPI_FREQ_HZ, MSBFIRST, SPI_MODE0));

void setup() {
  Serial.begin(115200);
  
  SPI.begin(GPIO_SPI_CLK, GPIO_SPI_MISO, GPIO_SPI_MOSI, -1);
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