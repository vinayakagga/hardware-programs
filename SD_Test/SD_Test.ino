#include "FS.h"
#include "SD_MMC.h"
#include <DHT11.h>

DHT11 dht11(16);
void setup() {
  Serial.begin(115200);

  if (!SD_MMC.begin()) {
    Serial.println("❌ SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("❌ No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %llu MB\n", cardSize);

  File file = SD_MMC.open("/test.txt", FILE_APPEND);
  if (file) {
    file.println("ESP32-CAM SD Card Working!");
    Serial.write(file.read());
    file.close();
    Serial.println("✅ File written successfully");
  } else {
    Serial.println("❌ Failed to write file");
  }
}

void loop() {
  
  int temperature = 0;
    int humidity = 0;

    // Attempt to read the temperature and humidity values from the DHT11 sensor.
    int result = dht11.readTemperatureHumidity(temperature, humidity);

    // Check the results of the readings.
    // If the reading is successful, print the temperature and humidity values.
    // If there are errors, print the appropriate error messages.
    if (result == 0) {
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" °C\tHumidity: ");
        Serial.print(humidity);
        Serial.println(" %");
        File file = SD_MMC.open("/test.txt", FILE_WRITE);
        if (file) {
          file.println(temperature);
          file.println(",");
          file.println(humidity);
          file.println("\n");
          file.close();
          Serial.println("✅ File written successfully");
  } else {
    Serial.println("❌ Failed to write file");
  }
}
  else {
        // Print error message based on the error code.
        Serial.println(DHT11::getErrorString(result));
    }

}