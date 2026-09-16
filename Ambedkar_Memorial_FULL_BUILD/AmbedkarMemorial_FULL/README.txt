AMBEDKAR MEMORIAL — FULL BUILD
=================================

This is the rapid full-content build.

SCREEN FLOW
-----------
STATIC SPLASH
     ↓
HOME
 ├── LIFE
 ├── TIMELINE
 ├── CONSTITUTION
 ├── IDEAS
 ├── LEGACY
 └── VISIT

There is NO video and NO continuous animation.
The TFT only redraws after a navigation action.

WHAT IS IN THIS ZIP
-------------------
AmbedkarMemorial_FULL.ino
touch.cpp
touch.h
source_images/
assets/
prepare_assets.py
upload_assets_sdfat.py
SOURCES.txt

HOW TO USE THE MASTER IMAGE PACK
--------------------------------
You do NOT copy the .rgb565 files into Arduino flash.

1. Extract this ZIP.
2. The source_images folder contains the original images already collected
   for the project.
3. The assets folder contains the correctly sized RGB565 files.
4. Upload those .rgb565 files to the SD card through the Arduino.

If you need to regenerate them:
    python -m pip install Pillow
    python prepare_assets.py

This recreates the assets folder.

SD UPLOAD
---------
1. Upload Ambedkar_SD_Asset_Loader_SdFat.ino from the previous loader package
   to the UNO R4.
2. Close Arduino Serial Monitor.
3. Use 460800 baud for the Python uploader.
4. Run the uploader for every asset, or use the batch command below.

PowerShell from this folder:

    Get-ChildItem .\assets\*.rgb565 | ForEach-Object {
        python .\upload_assets_sdfat.py --port COM3 --file $_.FullName $_.Name
    }

Replace COM3 if the Arduino uses another port.

Then verify:
    python .\upload_assets_sdfat.py --port COM3 --list

You should see 23 files.

IMPORTANT:
- Close Arduino Serial Monitor before running Python.
- The Arduino must be running the SdFat asset loader during transfer.
- Do NOT format the SD card again.
- After transferring, upload AmbedkarMemorial_FULL.ino.

ARDUINO
-------
Open:
    AmbedkarMemorial_FULL.ino

Board:
    Arduino UNO R4 WiFi

Libraries:
    Arduino_GFX_Library
    Adafruit GFX Library
    SdFat
    TouchScreen

The touch.cpp/touch.h in this package are the previously verified
soft-touch calibration.

SD ASSET SIZES
--------------
splash      480x320
LIFE        160x170
Other pages 190x140
VISIT       220x150

The largest image is read one scanline at a time.

CONTENT
-------
LIFE:
  5 cards

TIMELINE:
  Birth / Education / Mahad / public life / Constitution

CONSTITUTION:
  Justice & Liberty / Equality / Fundamental Rights

IDEAS:
  Educate / Agitate / Organize

LEGACY:
  Constitution / Social Reform / Continuing Legacy

VISIT:
  26 Alipur Road / Museum / Open Book Memorial

The memorial-specific descriptions are based on Dr. Ambedkar Foundation
and Delhi Tourism information. See SOURCES.txt.

TROUBLESHOOTING
---------------
If the screen says IMAGE:
  The corresponding .rgb565 file is missing from SD.

If SD says FAILED:
  Recheck the SdFat diagnostic that previously passed.

If Python says COM3 access denied:
  Close Arduino Serial Monitor / Serial Plotter and retry.

If the splash is missing:
  Upload splash.rgb565. The firmware has a text fallback.
