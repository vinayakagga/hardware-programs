AMBEDKAR MEMORIAL — SD-BACKED LIFE BUILD
==============================================

This build uses SdFat, which has been verified on your MAR3501:
- SDFAT INIT OK
- FILE CREATE OK
- WRITE OK
- READ + VERIFY OK
- DELETE OK

ARCHITECTURE
------------
UNO R4 flash:
  UI, fonts, code, touch

SD card:
  life1.rgb565
  life2.rgb565
  life3.rgb565
  life4.rgb565
  life5.rgb565

Images:
  160 x 170 pixels
  RGB565
  54,400 bytes each

No image arrays are stored in flash.

STEP 1 — FORMAT
---------------
Already done. Do not format again.

STEP 2 — UPLOAD THE SD LOADER
-----------------------------
Upload:
  Ambedkar_SD_Asset_Loader_SdFat.ino

Serial:
  460800 baud

Expected:
  AMBEDKAR_SD_LOADER_SDFAT
  SD_OK
  READY_FOR_COMMANDS

STEP 3 — CONVERT YOUR FIVE IMAGES
---------------------------------
Install:
  python -m pip install Pillow pyserial

Run:
  python convert_assets.py life1.png life1.rgb565
  python convert_assets.py life2.png life2.rgb565
  python convert_assets.py life3.jfif life3.rgb565
  python convert_assets.py life4.jfif life4.rgb565
  python convert_assets.py life5.jfif life5.rgb565

STEP 4 — UPLOAD TO SD THROUGH ARDUINO
-------------------------------------
Close Arduino Serial Monitor.

Replace COM3 with your actual port:

  python upload_assets_sdfat.py --port COM3 --file life1.rgb565 life1.rgb565
  python upload_assets_sdfat.py --port COM3 --file life2.rgb565 life2.rgb565
  python upload_assets_sdfat.py --port COM3 --file life3.rgb565 life3.rgb565
  python upload_assets_sdfat.py --port COM3 --file life4.rgb565 life4.rgb565
  python upload_assets_sdfat.py --port COM3 --file life5.rgb565 life5.rgb565

Verify:
  python upload_assets_sdfat.py --port COM3 --list

You should see all five files.

STEP 5 — UPLOAD THE SD LIFE FIRMWARE
------------------------------------
Upload:
  AmbedkarMemorial_SD_LIFE.ino

It reads each photo from SD when its LIFE card is displayed.

The photo is now approximately:
  160 x 170

instead of the previous:
  90 x 120

The UI remains fixed-card navigation:
  no vertical scrolling
  horizontal swipe for LIFE

IMPORTANT
---------
The image is read one scanline at a time into a 320-byte buffer.
This avoids consuming a large amount of UNO RAM.

The current UI still uses your existing touch.cpp/touch.h.

VIDEO
-----
Once the five images are working, we can use the same loader to store
a short video file on the SD card. We should add playback separately
after image rendering is confirmed, so we have one controlled variable
at a time.
