AMBEDKAR MEMORIAL — COMPILE-SAFE REAL IMAGE BUILD

The previous image build failed at the final linking stage, not because of
a C++ syntax error. The UNO R4 WiFi linker reported:

  section `.text' will not fit in region `FLASH'
  region `FLASH' overflowed by 13512 bytes

Cause:
Five 110x145 RGB565 images were embedded directly in flash. Together they
consume about 160 KB before the rest of the application and libraries.

Fix:
- Images are now 90x120 RGB565.
- Five images consume about 108 KB.
- The images remain large enough for the 480x320 card.
- No SD card is required for this test.
- The existing touch calibration and UI are preserved.
- The long titles still use the smaller title font.

Put these four files in the same Arduino sketch folder:
  AmbedkarMemorial_RealImages.ino
  ambedkar_images.h
  touch.cpp
  touch.h

Select:
  Arduino UNO R4 WiFi

Then compile/upload again.

Later, when the SD card is added, we can remove the embedded image arrays
entirely and use higher-quality JPEG assets from the card.
