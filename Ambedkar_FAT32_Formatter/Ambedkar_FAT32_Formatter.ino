/*
============================================================================
DR. B. R. AMBEDKAR NATIONAL MEMORIAL
MAR3501 SD CARD FAT32 FORMATTER
============================================================================

WARNING:
THIS WILL ERASE THE SD CARD.

Target:
  Arduino UNO R4 WiFi
  MAR3501 built-in SD slot

SD pins:
  CS   = D10
  MOSI = D11
  MISO = D12
  SCK  = D13

This sketch uses the SdFat library (v2.x).

For a 32 GB card, SdFat's FAT formatter will create a FAT32 filesystem.
The official SdFat formatter follows this same approach: cards larger
than 2 GiB and up to 32 GiB are formatted FAT32.

============================================================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

#define SD_CS 10

// Force FAT16/FAT32 rather than exFAT.
SdFat32 sd;

void setup() {

  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("================================================");
  Serial.println(" AMBEDKAR MEMORIAL FAT32 SD FORMATTER");
  Serial.println("================================================");
  Serial.println();

  Serial.println("WARNING:");
  Serial.println("ALL DATA ON THE SD CARD WILL BE ERASED.");
  Serial.println();
  Serial.println("Card target: MAR3501 SD slot");
  Serial.println("CS pin: D10");
  Serial.println();
  Serial.println("Starting in 5 seconds...");
  Serial.println();

  for (int i = 5; i >= 1; --i) {
    Serial.print(i);
    Serial.println("...");
    delay(1000);
  }

  Serial.println();
  Serial.println("Initializing SD card...");

  // 4 MHz is deliberately conservative for the formatter.
  if (!sd.begin(
        SdSpiConfig(
          SD_CS,
          SHARED_SPI,
          SD_SCK_MHZ(4)
        )
      )) {

    Serial.println();
    Serial.println("ERROR: SD INITIALIZATION FAILED");
    Serial.println();

    sd.printSdError(&Serial);

    Serial.println();
    Serial.println("STOP.");
    return;
  }

  Serial.println("SD CARD INITIALIZED.");
  Serial.println();

  Serial.println("Formatting FAT32...");
  Serial.println("DO NOT REMOVE POWER.");
  Serial.println("DO NOT REMOVE THE SD CARD.");
  Serial.println();

  if (!sd.format(&Serial)) {

    Serial.println();
    Serial.println("================================================");
    Serial.println(" FORMAT FAILED");
    Serial.println("================================================");
    Serial.println();

    sd.printSdError(&Serial);

    return;
  }

  Serial.println();
  Serial.println("================================================");
  Serial.println(" FAT32 FORMAT COMPLETE");
  Serial.println("================================================");
  Serial.println();
  Serial.println("The SD card is now ready.");
  Serial.println();
  Serial.println("Next:");
  Serial.println("1. Run the SD diagnostic again.");
  Serial.println("2. Confirm FILE CREATE OK.");
  Serial.println("3. Then we will upload the LIFE images.");
}

void loop() {
}
