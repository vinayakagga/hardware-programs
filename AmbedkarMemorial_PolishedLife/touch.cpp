/*
============================================================================
touch.cpp
Verified resistive touch driver
Soft-touch + stability filtering
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

// --------------------------------------------------------------------------
// VERIFIED LANDSCAPE CALIBRATION
// --------------------------------------------------------------------------

static const float TOUCH_AX = -0.58384084f;
static const float TOUCH_BX =  552.93869f;

static const float TOUCH_AY = -0.42020932f;
static const float TOUCH_BY =  374.01786f;

// --------------------------------------------------------------------------
// PRESSURE
// --------------------------------------------------------------------------

// Lower threshold makes the panel respond to a lighter touch.
// The previous verified calibration produced valid readings well above 900,
// so the upper limit remains 1000.
static const int TS_MINP = 80;
static const int TS_MAXP = 1000;

// --------------------------------------------------------------------------
// STABILITY FILTER
// --------------------------------------------------------------------------
//
// A single noisy sample should not create a touch.
// We accept a light touch when several consecutive samples are valid.
// The position is averaged to make taps/swipes steadier.
//
// This is deliberately small so the UI still feels responsive.

static const int REQUIRED_SAMPLES = 2;
static const int MAX_SAMPLE_JUMP = 35;

static const int SCR_W = 480;
static const int SCR_H = 320;

// --------------------------------------------------------------------------
// RAW -> SCREEN
// --------------------------------------------------------------------------

static void rawToScreen(
  int rawX,
  int rawY,
  int &sx,
  int &sy
) {
  sx = (int)(
    TOUCH_AX * rawY +
    TOUCH_BX
  );

  sy = (int)(
    TOUCH_AY * rawX +
    TOUCH_BY
  );

  sx = constrain(
    sx,
    0,
    SCR_W - 1
  );

  sy = constrain(
    sy,
    0,
    SCR_H - 1
  );
}

// --------------------------------------------------------------------------
// READ TOUCH
// --------------------------------------------------------------------------

bool touchRead(
  int &sx,
  int &sy
) {
  long sumX = 0;
  long sumY = 0;

  int accepted = 0;

  int lastX = 0;
  int lastY = 0;

  for (
    int i = 0;
    i < REQUIRED_SAMPLES;
    i++
  ) {
    TSPoint p = ts.getPoint();

    // Restore shared display pins after reading the resistive panel.
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    pinMode(XP, OUTPUT);
    pinMode(YM, OUTPUT);

    if (
      p.z < TS_MINP ||
      p.z > TS_MAXP
    ) {
      continue;
    }

    int tx;
    int ty;

    rawToScreen(
      p.x,
      p.y,
      tx,
      ty
    );

    // Reject a wildly different second sample.
    if (
      accepted > 0 &&
      (
        abs(tx - lastX) > MAX_SAMPLE_JUMP ||
        abs(ty - lastY) > MAX_SAMPLE_JUMP
      )
    ) {
      continue;
    }

    sumX += tx;
    sumY += ty;

    lastX = tx;
    lastY = ty;

    accepted++;
  }

  if (
    accepted < REQUIRED_SAMPLES
  ) {
    return false;
  }

  sx = sumX / accepted;
  sy = sumY / accepted;

  sx = constrain(
    sx,
    0,
    SCR_W - 1
  );

  sy = constrain(
    sy,
    0,
    SCR_H - 1
  );

  return true;
}
