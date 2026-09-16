/*
============================================================================
touch.cpp
Verified landscape touch mapping for UNO R4 + MAR3501 resistive touch.
============================================================================
*/

#include <Arduino.h>
#include <TouchScreen.h>

#include "touch.h"

// --------------------------------------------------------------------------
// VERIFIED TOUCH PINS
// --------------------------------------------------------------------------

#define YP A3
#define XM A2
#define YM 9
#define XP 8

static TouchScreen ts =
  TouchScreen(
    XP,
    YP,
    XM,
    YM,
    300
  );

// --------------------------------------------------------------------------
// VERIFIED LANDSCAPE CALIBRATION
// --------------------------------------------------------------------------

// Screen X = -0.58384084 * Raw Y + 552.93869
// Screen Y = -0.42020932 * Raw X + 374.01786

static const float TOUCH_AX =
  -0.58384084f;

static const float TOUCH_BX =
  552.93869f;

static const float TOUCH_AY =
  -0.42020932f;

static const float TOUCH_BY =
  374.01786f;

// --------------------------------------------------------------------------
// PRESSURE
// --------------------------------------------------------------------------

// Valid calibration readings included values above 900,
// e.g. 932, 976, 985 and 972.
// Therefore do not reject those touches.

static const int TS_MINP = 200;
static const int TS_MAXP = 1000;

static const int SCR_W = 480;
static const int SCR_H = 320;

// --------------------------------------------------------------------------
// READ TOUCH
// --------------------------------------------------------------------------

bool touchRead(
  int &sx,
  int &sy
) {
  TSPoint p =
    ts.getPoint();

  // Restore shared LCD pins after reading resistive touch.
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);

  if (
    p.z < TS_MINP ||
    p.z > TS_MAXP
  ) {
    return false;
  }

  sx =
    (int)(
      TOUCH_AX * p.y +
      TOUCH_BX
    );

  sy =
    (int)(
      TOUCH_AY * p.x +
      TOUCH_BY
    );

  sx =
    constrain(
      sx,
      0,
      SCR_W - 1
    );

  sy =
    constrain(
      sy,
      0,
      SCR_H - 1
    );

  return true;
}
