/*
============================================================
AMBEDKAR MEMORIAL — SD ASSET LOADER
Arduino UNO R4 WiFi + MAR3501
============================================================
Uploads binary assets from the PC over USB Serial to SD.

Protocol:
  PUT filename size
  -> READY
  -> raw bytes
  -> OK size

Commands:
  PING
  LIST
  DELETE filename

SD:
  CS   D10
  MOSI D11
  MISO D12
  SCK  D13

Uses SdFat directly because the tested SD.h wrapper could read
but could not create files on this MAR3501 SD interface.
============================================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

#define SD_CS 10
#define BAUD 460800
#define BLOCK 512

SdFat sd;
char lineBuffer[96];

bool validName(const char *s) {
  if (!s || !s[0]) return false;
  for (const char *p=s; *p; ++p) {
    char c=*p;
    if (!((c>='a'&&c<='z') || (c>='A'&&c<='Z') ||
          (c>='0'&&c<='9') || c=='_' || c=='-' || c=='.')) return false;
  }
  return true;
}

bool getLine(char *dst, size_t n) {
  static size_t pos=0;
  while (Serial.available()) {
    char c=(char)Serial.read();
    if (c=='\r') continue;
    if (c=='\n') {
      dst[pos]='\0';
      pos=0;
      return true;
    }
    if (pos<n-1) dst[pos++]=c;
    else {
      pos=0;
      Serial.println("ERR LINE_TOO_LONG");
      return false;
    }
  }
  return false;
}

void listFiles() {
  FsFile root=sd.open("/");
  if (!root) {
    Serial.println("ERR ROOT");
    return;
  }

  FsFile e;
  while (e.openNext(&root, O_RDONLY)) {
    char name[64];
    e.getName(name, sizeof(name));
    if (e.isFile()) {
      Serial.print(name);
      Serial.print(" ");
      Serial.println((uint32_t)e.size());
    }
    e.close();
  }
  root.close();
  Serial.println("LIST_END");
}

void putFile(const char *name, uint32_t total) {
  if (!validName(name)) {
    Serial.println("ERR BAD_FILENAME");
    return;
  }

  sd.remove(name);

  FsFile f=sd.open(
    name,
    O_WRONLY | O_CREAT | O_TRUNC
  );

  if (!f) {
    Serial.println("ERR OPEN_WRITE");
    sd.printSdError(&Serial);
    return;
  }

  Serial.println("READY");

  uint8_t buf[BLOCK];
  uint32_t got=0;
  uint32_t lastProgress=0;
  unsigned long lastData=millis();

  while (got<total) {
    if (Serial.available()) {
      size_t want=(size_t)min(
        (uint32_t)BLOCK,
        total-got
      );

      size_t n=Serial.readBytes(buf, want);

      if (n) {
        size_t w=f.write(buf,n);

        if (w!=n) {
          f.close();
          Serial.println("ERR WRITE");
          sd.printSdError(&Serial);
          return;
        }

        got += n;
        lastData=millis();
      }
    }

    if (millis()-lastData>15000UL) {
      f.close();
      Serial.println("ERR TIMEOUT");
      return;
    }

    if (got-lastProgress>=16384 || got==total) {
      Serial.print("PROGRESS ");
      Serial.print(got);
      Serial.print("/");
      Serial.println(total);
      lastProgress=got;
    }
  }

  f.flush();
  f.close();

  FsFile check=sd.open(name,O_RDONLY);

  if (!check) {
    Serial.println("ERR VERIFY_OPEN");
    return;
  }

  uint32_t actual=check.size();
  check.close();

  if (actual!=total) {
    Serial.print("ERR VERIFY_SIZE ");
    Serial.println(actual);
    return;
  }

  Serial.print("OK ");
  Serial.println(actual);
}

void command(char *cmd) {
  if (!strcmp(cmd,"PING")) {
    Serial.println("PONG");
    return;
  }

  if (!strcmp(cmd,"LIST")) {
    listFiles();
    return;
  }

  if (!strncmp(cmd,"DELETE ",7)) {
    const char *name=cmd+7;
    if (!validName(name)) {
      Serial.println("ERR BAD_FILENAME");
      return;
    }
    Serial.println(
      sd.remove(name) ? "OK DELETED" : "ERR DELETE"
    );
    return;
  }

  if (!strncmp(cmd,"PUT ",4)) {
    char name[64];
    unsigned long size=0;

    if (sscanf(cmd+4,"%63s %lu",name,&size)!=2 || size==0) {
      Serial.println("ERR PUT_SYNTAX");
      return;
    }

    putFile(name,(uint32_t)size);
    return;
  }

  Serial.println("ERR UNKNOWN_COMMAND");
}

void setup() {
  Serial.begin(BAUD);
  delay(1200);

  Serial.println();
  Serial.println("AMBEDKAR_SD_LOADER_SDFAT");
  Serial.print("BAUD ");
  Serial.println(BAUD);

  if (!sd.begin(SdSpiConfig(
        SD_CS,
        SHARED_SPI,
        SD_SCK_MHZ(4)
      ))) {
    Serial.println("SD_ERROR");
    sd.printSdError(&Serial);
    return;
  }

  Serial.println("SD_OK");
  Serial.println("READY_FOR_COMMANDS");
}

void loop() {
  if (getLine(lineBuffer,sizeof(lineBuffer))) {
    command(lineBuffer);
  }
}
