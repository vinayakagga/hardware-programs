#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

#define SD_CS 10
#define BAUD 460800

// Keep this small enough that the Arduino can consume
// the serial stream without overflowing.
#define CHUNK_SIZE 512

SdFat sd;

char lineBuffer[96];

bool validName(const char *s) {
  if (!s || !s[0]) return false;

  for (const char *p = s; *p; ++p) {
    char c = *p;

    if (!(
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '_' ||
      c == '-' ||
      c == '.'
    )) {
      return false;
    }
  }

  return true;
}

bool getLine(char *dst, size_t n) {

  static size_t pos = 0;

  while (Serial.available()) {

    char c = (char)Serial.read();

    if (c == '\r')
      continue;

    if (c == '\n') {

      dst[pos] = '\0';
      pos = 0;

      return true;
    }

    if (pos < n - 1) {
      dst[pos++] = c;
    }
    else {

      pos = 0;

      Serial.println("ERR LINE_TOO_LONG");

      return false;
    }
  }

  return false;
}


// ============================================================
// LIST
// ============================================================

void listFiles() {

  FsFile root = sd.open("/");

  if (!root) {
    Serial.println("ERR ROOT");
    return;
  }

  FsFile entry;

  while (entry.openNext(&root, O_RDONLY)) {

    char name[64];

    entry.getName(
      name,
      sizeof(name)
    );

    if (entry.isFile()) {

      Serial.print(name);
      Serial.print(" ");
      Serial.println(
        (uint32_t)entry.size()
      );
    }

    entry.close();
  }

  root.close();

  Serial.println("LIST_END");
}


// ============================================================
// UPLOAD
// ============================================================

void putFile(
  const char *name,
  uint32_t total
) {

  if (!validName(name)) {

    Serial.println(
      "ERR BAD_FILENAME"
    );

    return;
  }

  // Remove previous version.
  sd.remove(name);

  FsFile file = sd.open(
    name,
    O_WRONLY | O_CREAT | O_TRUNC
  );

  if (!file) {

    Serial.println(
      "ERR OPEN_WRITE"
    );

    sd.printSdError(&Serial);

    return;
  }

  Serial.println("READY");

  uint8_t buffer[CHUNK_SIZE];

  uint32_t received = 0;

  while (received < total) {

    uint32_t remaining =
      total - received;

    uint32_t wanted =
      remaining < CHUNK_SIZE
        ? remaining
        : CHUNK_SIZE;

    // Wait until the complete chunk arrives.
    size_t got = Serial.readBytes(
      (char *)buffer,
      wanted
    );

    if (got != wanted) {

      file.close();

      Serial.println(
        "ERR RECEIVE_TIMEOUT"
      );

      return;
    }

    size_t written =
      file.write(
        buffer,
        got
      );

    if (written != got) {

      file.close();

      Serial.println(
        "ERR WRITE"
      );

      sd.printSdError(&Serial);

      return;
    }

    received += got;

    // CRITICAL:
    // Don't let the PC send the next chunk until
    // the Arduino has consumed/written this one.
    Serial.println("ACK");

    if (
      received % 16384 == 0 ||
      received == total
    ) {

      Serial.print(
        "PROGRESS "
      );

      Serial.print(received);

      Serial.print("/");

      Serial.println(total);
    }
  }

  file.flush();
  file.close();

  // Verify final size.
  FsFile check =
    sd.open(
      name,
      O_RDONLY
    );

  if (!check) {

    Serial.println(
      "ERR VERIFY_OPEN"
    );

    return;
  }

  uint32_t actual =
    check.size();

  check.close();

  if (actual != total) {

    Serial.print(
      "ERR VERIFY_SIZE "
    );

    Serial.println(actual);

    return;
  }

  Serial.print("OK ");
  Serial.println(actual);
}


// ============================================================
// COMMANDS
// ============================================================

void command(char *cmd) {

  if (!strcmp(cmd, "PING")) {

    Serial.println("PONG");

    return;
  }


  if (!strcmp(cmd, "LIST")) {

    listFiles();

    return;
  }


  if (!strncmp(cmd, "DELETE ", 7)) {

    const char *name =
      cmd + 7;

    if (!validName(name)) {

      Serial.println(
        "ERR BAD_FILENAME"
      );

      return;
    }

    if (sd.remove(name))
      Serial.println("OK DELETED");
    else
      Serial.println("ERR DELETE");

    return;
  }


  if (!strncmp(cmd, "PUT ", 4)) {

    char name[64];

    unsigned long size = 0;

    if (
      sscanf(
        cmd + 4,
        "%63s %lu",
        name,
        &size
      ) != 2
      ||
      size == 0
    ) {

      Serial.println(
        "ERR PUT_SYNTAX"
      );

      return;
    }

    putFile(
      name,
      (uint32_t)size
    );

    return;
  }


  Serial.println(
    "ERR UNKNOWN_COMMAND"
  );
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(BAUD);

  delay(1500);

  Serial.println();
  Serial.println(
    "AMBEDKAR_SD_LOADER_SDFAT"
  );

  Serial.print("BAUD ");
  Serial.println(BAUD);


  if (!sd.begin(
        SdSpiConfig(
          SD_CS,
          SHARED_SPI,
          SD_SCK_MHZ(4)
        )
      )) {

    Serial.println(
      "SD_ERROR"
    );

    sd.printSdError(&Serial);

    return;
  }

  Serial.println("SD_OK");
  Serial.println(
    "READY_FOR_COMMANDS"
  );
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  if (
    getLine(
      lineBuffer,
      sizeof(lineBuffer)
    )
  ) {

    command(lineBuffer);
  }
}