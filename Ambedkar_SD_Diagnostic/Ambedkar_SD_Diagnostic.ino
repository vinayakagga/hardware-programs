/*
============================================================
DR. AMBEDKAR NATIONAL MEMORIAL
SD CARD DIAGNOSTIC — COMPATIBLE VERSION

Arduino UNO R4 WiFi + MAR3501
Using the SD library currently selected by Arduino IDE.

SD:
D10 = CS
D11 = MOSI
D12 = MISO
D13 = SCK

Serial Monitor:
115200 baud

NON-DESTRUCTIVE:
This sketch does NOT format the card.
It only creates a temporary file, writes it,
reads it back, then deletes it.
============================================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS 10

const char TEST_FILE[] = "/danm_test.txt";
const char TEST_TEXT[] =
  "DR AMBEDKAR MEMORIAL SD TEST 123456789";

void setup() {

  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AMBEDKAR MEMORIAL SD DIAGNOSTIC");
  Serial.println("========================================");

  // -------------------------------------------------------
  // 1. Initialize SD
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[1] Initializing SD...");

  if (!SD.begin(SD_CS)) {
    Serial.println("RESULT: SD INIT FAILED");
    Serial.println();
    Serial.println("Check:");
    Serial.println("  - SD card seated correctly");
    Serial.println("  - D10 = SD CS");
    Serial.println("  - D11 = MOSI");
    Serial.println("  - D12 = MISO");
    Serial.println("  - D13 = SCK");
    return;
  }

  Serial.println("RESULT: SD INIT OK");

  // -------------------------------------------------------
  // 2. Open root directory
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[2] Opening root directory...");

  File root = SD.open("/");

  if (!root) {
    Serial.println("RESULT: ROOT OPEN FAILED");
    return;
  }

  Serial.println("RESULT: ROOT OPEN OK");

  // -------------------------------------------------------
  // 3. Show existing files
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[3] Existing files:");

  File entry = root.openNextFile();

  if (!entry) {
    Serial.println("  (no files)");
  }

  while (entry) {

    Serial.print("  ");
    Serial.print(entry.name());

    if (!entry.isDirectory()) {
      Serial.print("  ");
      Serial.print(entry.size());
      Serial.println(" bytes");
    } else {
      Serial.println("  <DIR>");
    }

    entry.close();
    entry = root.openNextFile();
  }

  root.close();

  // -------------------------------------------------------
  // 4. Create test file
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[4] Creating temporary file...");

  // Remove an old copy if one exists.
  SD.remove(TEST_FILE);

  File testFile = SD.open(
    TEST_FILE,
    FILE_WRITE
  );

  if (!testFile) {
    Serial.println("RESULT: FILE CREATE FAILED");
    Serial.println();
    Serial.println("SD initializes successfully, but the");
    Serial.println("Arduino SD library cannot create a file.");
    Serial.println();
    Serial.println("This is the important result to diagnose.");
    return;
  }

  Serial.println("RESULT: FILE CREATE OK");

  // -------------------------------------------------------
  // 5. Write
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[5] Writing test data...");

  size_t expected = strlen(TEST_TEXT);

  size_t written = testFile.print(TEST_TEXT);

  testFile.flush();
  testFile.close();

  Serial.print("Expected bytes: ");
  Serial.println(expected);

  Serial.print("Written bytes:  ");
  Serial.println(written);

  if (written != expected) {
    Serial.println("RESULT: WRITE FAILED");
    return;
  }

  Serial.println("RESULT: WRITE OK");

  // -------------------------------------------------------
  // 6. Read back
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[6] Reading test file...");

  File readFile = SD.open(TEST_FILE);

  if (!readFile) {
    Serial.println("RESULT: READ OPEN FAILED");
    return;
  }

  String received = readFile.readString();

  readFile.close();

  Serial.print("Read: ");
  Serial.println(received);

  if (received != TEST_TEXT) {
    Serial.println("RESULT: READ VERIFY FAILED");
    return;
  }

  Serial.println("RESULT: READ + VERIFY OK");

  // -------------------------------------------------------
  // 7. Check file size
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[7] Checking file size...");

  File sizeFile = SD.open(TEST_FILE);

  if (!sizeFile) {
    Serial.println("RESULT: SIZE OPEN FAILED");
    return;
  }

  Serial.print("File size: ");
  Serial.print(sizeFile.size());
  Serial.println(" bytes");

  sizeFile.close();

  // -------------------------------------------------------
  // 8. Delete
  // -------------------------------------------------------

  Serial.println();
  Serial.println("[8] Deleting temporary file...");

  if (SD.remove(TEST_FILE)) {
    Serial.println("RESULT: DELETE OK");
  } else {
    Serial.println("RESULT: DELETE FAILED");
  }

  // -------------------------------------------------------
  // Final result
  // -------------------------------------------------------

  Serial.println();
  Serial.println("========================================");
  Serial.println(" DIAGNOSTIC COMPLETE");
  Serial.println("========================================");
}

void loop() {
}
