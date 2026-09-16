AMBEDKAR MEMORIAL UI — TEST FILES
====================================

Files:
  AmbedkarMemorial_UI.ino
  touch.cpp
  touch.h

Hardware:
  Arduino UNO R4 WiFi
  MAR3501 / ILI9486
  480x320 landscape
  Resistive touch

Touch pins:
  XP = D8
  XM = D9
  YP = A3
  YM = A2

Calibration:
  Screen X = -0.58384084 * Raw Y + 552.93869
  Screen Y = -0.42020932 * Raw X + 374.01786

IMPORTANT:
  Do not change the calibration coefficients.

This version has an FPS counter enabled.
Open Serial Monitor at 115200 baud.

The TFT also shows the measured FPS in the top-right corner.

Test sequence:
1. Upload all three source files together.
2. Open Serial Monitor at 115200.
3. Wait for the home screen.
4. Tap a card.
5. Watch the FPS value in Serial Monitor.
6. Open a content page and swipe vertically.
7. Check whether scrolling now feels smoother.
8. Report the FPS number and behavior.

If the FPS is low, the next optimization should be partial-region rendering,
not further increases to delay() values.
