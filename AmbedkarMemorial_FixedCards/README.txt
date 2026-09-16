AMBEDKAR MEMORIAL — FIXED STORY CARD TEST
=============================================

Hardware:
- Arduino UNO R4 WiFi
- MAR3501 / ILI9486
- 480x320 landscape
- 4-wire resistive touch

Touch:
XP = D8
XM = D9
YP = A3
YM = A2

Calibration:
Screen X = -0.58384084 * Raw Y + 552.93869
Screen Y = -0.42020932 * Raw X + 374.01786

UI:
- No vertical scrolling
- No continuous transition animation
- Home has 6 cards
- LIFE opens into 5 fixed story cards
- Swipe LEFT = next LIFE card
- Swipe RIGHT = previous LIFE card
- Tap BACK = home
- Other sections are placeholders for now

FPS:
Serial Monitor = 115200 baud.
A render FPS value is printed after rendering activity.
A small FPS indicator is also shown in the top-right.

TEST:
1. Upload the .ino and touch.cpp together.
2. Keep touch.h in the same sketch folder.
3. Tap LIFE.
4. Swipe left several times.
5. Swipe right several times.
6. Tap BACK.
7. Tap LIFE again.
8. Check that there is no 10-13 frame animation loop.
9. Report how responsive the swipe feels and the FPS shown.
