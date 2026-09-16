/*
============================================================================
Verified resistive touch driver
Landscape calibration preserved.
============================================================================
*/

#include <Arduino.h>
#include <TouchScreen.h>

#include "touch.h"

#define YP A3
#define XM A2
#define YM 9
#define XP 8

static TouchScreen ts(
  XP,
  YP,
  XM,
  YM,
  300
);

// VERIFIED 9-POINT CALIBRATION
static const float TOUCH_AX = -0.58384084f;
static const float TOUCH_BX =  552.93869f;

static const float TOUCH_AY = -0.42020932f;
static const float TOUCH_BY =  374.01786f;

// Valid observed readings included 932, 976, 985, 972.
static const int TS_MINP = 80;
static const int TS_MAXP = 1000;

static const int SCR_W = 480;
static const int SCR_H = 320;

bool touchRead(
  int &sx,
  int &sy
) {

  TSPoint p =
    ts.getPoint();

  // Restore shared display pins.
  pinMode(
    XM,
    OUTPUT
  );

  pinMode(
    YP,
    OUTPUT
  );

  pinMode(
    XP,
    OUTPUT
  );

  pinMode(
    YM,
    OUTPUT
  );

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
