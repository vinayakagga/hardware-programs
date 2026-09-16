/*
============================================================================
Dr. B. R. Ambedkar National Memorial — Kiosk UI
Target: Arduino UNO R4 WiFi + MAR3501 3.5" TFT / ILI9486
Resolution: 480x320 landscape

This version:
- fixes touch-release handling
- fixes swipe tracking
- accepts valid pressure readings up to 1000
- measures actual rendering FPS
- uses frame-timed scrolling/animation rather than fixed delays
- avoids changing touch calibration
============================================================================
*/

#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "touch.h"

// --------------------------------------------------------------------------
// DISPLAY
// --------------------------------------------------------------------------

Arduino_DataBus *bus =
  new Arduino_UNOPAR8();

Arduino_GFX *gfx =
  new Arduino_ILI9486(
    bus,
    A4,
    1,
    false
  );

#define SCR_W 480
#define SCR_H 320
#define HDR_H 66

// --------------------------------------------------------------------------
// COLORS
// --------------------------------------------------------------------------

uint16_t BG, PANEL, CARD, GOLD, INK, SUB, SHADOW;

// --------------------------------------------------------------------------
// SCREENS
// --------------------------------------------------------------------------

enum Screen {
  S_HOME,
  S_BIO,
  S_TIME,
  S_CONST,
  S_QUOTE,
  S_LEGACY,
  S_VISIT
};

Screen curScreen = S_HOME;

// --------------------------------------------------------------------------
// SCROLL
// --------------------------------------------------------------------------

int scrollY = 0;
int targetScrollY = 0;
int maxScroll = 0;

// --------------------------------------------------------------------------
// TOUCH STATE
// --------------------------------------------------------------------------

bool fingerDown = false;

int touchStartX = -1;
int touchStartY = -1;

int lastTouchX = -1;
int lastTouchY = -1;

// --------------------------------------------------------------------------
// TRANSITION
// --------------------------------------------------------------------------

bool isAnimating = false;
float animProgress = 0.0f;
unsigned long lastAnimFrame = 0;

// --------------------------------------------------------------------------
// FPS MEASUREMENT
// --------------------------------------------------------------------------

unsigned long fpsWindowStart = 0;
unsigned long fpsFrames = 0;
float measuredFPS = 0.0f;

bool showFPS = true;

// --------------------------------------------------------------------------
// CONTENT
// --------------------------------------------------------------------------

struct Section {
  const char* title;
  const char* text;
  const char* imgPath;
  int imgHeight;
};

const char BIO[] PROGMEM =
"Dr. Bhimrao Ramji Ambedkar (1891-1956) was an Indian jurist, economist, "
"social reformer and political leader. Born into a Mahar family treated as "
"untouchable, he overcame severe discrimination to earn doctorates from "
"Columbia University and the London School of Economics. He devoted his life "
"to dismantling caste oppression and securing dignity, equality and rights "
"for the marginalised. As independent India's first Law and Justice Minister "
"and Chairman of the Constitution Drafting Committee, he shaped the legal "
"foundation of the Republic. In 1956 he embraced Buddhism with hundreds of "
"thousands of followers at Nagpur.";

const char TIMELINE[] PROGMEM =
"1891   Born on 14 April at Mhow.\n"
"1913   Scholarship to Columbia University.\n"
"1916   Presented Castes in India.\n"
"1927   Led the Mahad Satyagraha.\n"
"1947   First Law Minister of India.\n"
"1947   Chairman of Constitution Drafting Committee.\n"
"1950   Constitution came into force.\n"
"1956   Embraced Buddhism at Nagpur.\n"
"1990   Conferred the Bharat Ratna.";

const char CONST_TXT[] PROGMEM =
"As Chairman of the Drafting Committee, Dr. Ambedkar guided the writing "
"of the Constitution of India. He championed a strong framework of "
"fundamental rights, equality before the law, and the abolition of "
"untouchability under Article 17. He argued that political democracy "
"could not endure without social and economic democracy.";

const char QUOTES[] PROGMEM =
"\"I measure the progress of a community by the degree of progress which "
"women have achieved.\"\n\n"
"\"Life should be great rather than long.\"\n\n"
"\"Be educated. Be organised. Be agitated.\"\n\n"
"\"Constitution is not a mere lawyers document; it is a vehicle of life.\"";

const char LEGACY[] PROGMEM =
"Dr. Ambedkar's influence endures across law, education and social justice. "
"Revered as Babasaheb, he remains a symbol of self-respect and emancipation. "
"In 1990 the nation honoured him with the Bharat Ratna.";

const char VISIT[] PROGMEM =
"Dr. Ambedkar National Memorial\n"
"26 Alipur Road, Civil Lines, Delhi.\n\n"
"This is the site where Babasaheb spent his final days. The memorial honours "
"his life and his contribution to the Constitution of India.";

const Section SECTIONS[6] = {
  {"Biography", BIO, "/bio.bmp", 120},
  {"Timeline", TIMELINE, "/timeline.bmp", 100},
  {"Constitution", CONST_TXT, "/const.bmp", 120},
  {"Quotes", QUOTES, "/quotes.bmp", 100},
  {"Legacy", LEGACY, "/legacy.bmp", 120},
  {"Visit", VISIT, "/visit.bmp", 100}
};

// --------------------------------------------------------------------------
// HELPERS
// --------------------------------------------------------------------------

static inline uint8_t chR(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t chG(uint32_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t chB(uint32_t c) { return c & 0xFF; }

void vGradient(
  int x, int y, int w, int h,
  uint32_t top, uint32_t bot
) {
  for (int i = 0; i < h; i++) {
    float t = (h > 1) ? (float)i / (h - 1) : 0.0f;

    uint8_t r = chR(top) + (int)((chR(bot) - chR(top)) * t);
    uint8_t g = chG(top) + (int)((chG(bot) - chG(top)) * t);
    uint8_t b = chB(top) + (int)((chB(bot) - chB(top)) * t);

    gfx->drawFastHLine(
      x,
      y + i,
      w,
      gfx->color565(r, g, b)
    );
  }
}

void textC(
  const char* s,
  int cx,
  int baseline,
  uint16_t color,
  const GFXfont* f
) {
  gfx->setFont(f);
  gfx->setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;

  gfx->getTextBounds(
    (char*)s,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  gfx->setCursor(
    cx - w / 2 - x1,
    baseline
  );

  gfx->print(s);
}

// --------------------------------------------------------------------------
// IMAGE PLACEHOLDER
// --------------------------------------------------------------------------

void drawSectionImage(
  const char* path,
  int x,
  int y,
  int maxW,
  int maxH
) {
  gfx->fillRoundRect(
    x, y, maxW, maxH, 8, PANEL
  );

  gfx->drawRoundRect(
    x, y, maxW, maxH, 8, GOLD
  );

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(SUB);

  int16_t x1, y1;
  uint16_t w, h;

  gfx->getTextBounds(
    "IMAGE",
    0, 0,
    &x1, &y1,
    &w, &h
  );

  gfx->setCursor(
    x + maxW / 2 - w / 2 - x1,
    y + maxH / 2 + 5
  );

  gfx->print("IMAGE");
}

// --------------------------------------------------------------------------
// CALCULATE CONTENT HEIGHT
// --------------------------------------------------------------------------

int calculateContentHeight(const Section& sec) {
  const int LH = 22;
  const int CHARS_PER_LINE = 70;

  int currentImgHeight =
    (sec.imgPath != NULL)
      ? sec.imgHeight + 20
      : 0;

  int totalLines = 0;
  int lineStart = 0;
  int len = strlen_P(sec.text);

  for (int i = 0; i <= len; i++) {
    char ch = pgm_read_byte(&sec.text[i]);

    bool isEnd = (i == len);
    bool isNewline = (ch == '\n');

    if (
      isNewline ||
      isEnd ||
      (i - lineStart >= CHARS_PER_LINE)
    ) {
      totalLines++;
      lineStart = i + 1;
    }
  }

  return
    HDR_H +
    20 +
    currentImgHeight +
    totalLines * LH +
    40;
}

// --------------------------------------------------------------------------
// CONTENT RENDER
// --------------------------------------------------------------------------

void renderScrollableContent(
  const Section& sec
) {
  const int X = 20;
  const int LH = 22;
  const int startY = HDR_H + 20;
  const int CHARS_PER_LINE = 70;

  int currentImgHeight = 0;

  if (sec.imgPath != NULL) {
    int imageY = startY - scrollY;

    if (
      imageY < SCR_H &&
      imageY + sec.imgHeight > HDR_H
    ) {
      drawSectionImage(
        sec.imgPath,
        (SCR_W - 200) / 2,
        imageY,
        200,
        sec.imgHeight
      );
    }

    currentImgHeight =
      sec.imgHeight + 20;
  }

  int textStartY =
    startY + currentImgHeight;

  int totalLines = 0;
  int lineStart = 0;
  int len = strlen_P(sec.text);

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(SUB);

  for (int i = 0; i <= len; i++) {
    char ch = pgm_read_byte(&sec.text[i]);

    bool isEnd = (i == len);
    bool isNewline = (ch == '\n');

    if (
      isNewline ||
      isEnd ||
      (i - lineStart >= CHARS_PER_LINE)
    ) {
      int drawY =
        textStartY +
        totalLines * LH -
        scrollY;

      if (
        drawY + LH > HDR_H &&
        drawY < SCR_H
      ) {
        char lineBuf[CHARS_PER_LINE + 1];

        int lineLen =
          i - lineStart;

        if (lineLen > CHARS_PER_LINE)
          lineLen = CHARS_PER_LINE;

        strncpy_P(
          lineBuf,
          &sec.text[lineStart],
          lineLen
        );

        lineBuf[lineLen] = 0;

        gfx->setCursor(
          X,
          drawY + 15
        );

        gfx->print(lineBuf);
      }

      totalLines++;
      lineStart = i + 1;
    }
  }
}

// --------------------------------------------------------------------------
// HOME
// --------------------------------------------------------------------------

void renderHome() {
  gfx->fillScreen(BG);

  vGradient(
    0, 0,
    SCR_W, HDR_H,
    0x213CB0,
    0x0E1E52
  );

  textC(
    "Dr. B. R. Ambedkar",
    SCR_W / 2,
    30,
    INK,
    &FreeSansBold12pt7b
  );

  textC(
    "N A T I O N A L   M E M O R I A L",
    SCR_W / 2,
    52,
    GOLD,
    &FreeSans9pt7b
  );

  gfx->fillRect(
    0,
    HDR_H,
    SCR_W,
    2,
    GOLD
  );

  const int cols = 3;
  const int cardW = 140;
  const int cardH = 90;

  const int x0[3] = {
    15, 170, 325
  };

  const int y0[2] = {
    90, 195
  };

  for (int i = 0; i < 6; i++) {
    int cx = x0[i % cols];
    int cy = y0[i / cols];

    gfx->fillRoundRect(
      cx + 3,
      cy + 4,
      cardW,
      cardH,
      10,
      SHADOW
    );

    gfx->fillRoundRect(
      cx,
      cy,
      cardW,
      cardH,
      10,
      CARD
    );

    gfx->drawRoundRect(
      cx,
      cy,
      cardW,
      cardH,
      10,
      PANEL
    );

    textC(
      SECTIONS[i].title,
      cx + cardW / 2,
      cy + cardH / 2 + 6,
      INK,
      &FreeSansBold9pt7b
    );
  }
}

// --------------------------------------------------------------------------
// CONTENT SCREEN
// --------------------------------------------------------------------------

void renderContentScreen() {
  gfx->fillScreen(BG);

  vGradient(
    0, 0,
    SCR_W, HDR_H,
    0x213CB0,
    0x0E1E52
  );

  // Back arrow
  gfx->fillTriangle(
    20, 33,
    35, 22,
    35, 44,
    GOLD
  );

  textC(
    SECTIONS[curScreen - 1].title,
    SCR_W / 2 + 15,
    42,
    INK,
    &FreeSansBold12pt7b
  );

  gfx->fillRect(
    0,
    HDR_H,
    SCR_W,
    2,
    GOLD
  );

  // Clip visually by clearing the viewport first.
  gfx->fillRect(
    0,
    HDR_H + 2,
    SCR_W,
    SCR_H - HDR_H - 2,
    BG
  );

  renderScrollableContent(
    SECTIONS[curScreen - 1]
  );

  maxScroll = max(
    0,
    calculateContentHeight(
      SECTIONS[curScreen - 1]
    ) -
    (SCR_H - 20)
  );

  targetScrollY =
    constrain(
      targetScrollY,
      0,
      maxScroll
    );

  scrollY =
    constrain(
      scrollY,
      0,
      maxScroll
    );

  if (maxScroll > 0) {
    int barH = 40;

    int trackH =
      SCR_H -
      HDR_H -
      20 -
      barH;

    int barY =
      HDR_H +
      10 +
      (int)(
        ((float)scrollY / maxScroll) *
        trackH
      );

    gfx->fillRoundRect(
      SCR_W - 8,
      barY,
      4,
      barH,
      2,
      GOLD
    );
  }
}

// --------------------------------------------------------------------------
// FPS
// --------------------------------------------------------------------------

void countFrame() {
  fpsFrames++;

  unsigned long now = millis();

  if (
    fpsWindowStart == 0
  ) {
    fpsWindowStart = now;
    return;
  }

  unsigned long elapsed =
    now - fpsWindowStart;

  if (elapsed >= 1000) {
    measuredFPS =
      (float)fpsFrames *
      1000.0f /
      (float)elapsed;

    Serial.print("FPS = ");
    Serial.println(measuredFPS, 1);

    fpsFrames = 0;
    fpsWindowStart = now;
  }
}

void drawFPS() {
  if (!showFPS)
    return;

  gfx->fillRect(
    400,
    2,
    76,
    18,
    BG
  );

  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    404,
    15
  );

  gfx->print(
    measuredFPS,
    1
  );

  gfx->print(
    " FPS"
  );
}

// --------------------------------------------------------------------------
// FULL RENDER WRAPPER
// --------------------------------------------------------------------------

void renderCurrentScreen() {
  if (curScreen == S_HOME)
    renderHome();
  else
    renderContentScreen();

  drawFPS();
  countFrame();
}

// --------------------------------------------------------------------------
// START TRANSITION
// --------------------------------------------------------------------------

void startTransition() {
  animProgress = 0.0f;
  isAnimating = true;
  lastAnimFrame = millis();
}

// --------------------------------------------------------------------------
// TRANSITION
// --------------------------------------------------------------------------

void handleAnimation() {
  unsigned long now = millis();

  // Run animation at roughly 60Hz without blocking the CPU.
  if (
    now - lastAnimFrame < 16
  ) {
    return;
  }

  lastAnimFrame = now;

  animProgress += 0.08f;

  if (animProgress >= 1.0f) {
    animProgress = 1.0f;
    isAnimating = false;

    renderCurrentScreen();
    return;
  }

  // We deliberately keep this simple:
  // render the final screen while measuring actual FPS.
  renderCurrentScreen();
}

// --------------------------------------------------------------------------
// TOUCH BEGIN
// --------------------------------------------------------------------------

void beginTouch(
  int x,
  int y
) {
  fingerDown = true;

  touchStartX = x;
  touchStartY = y;

  lastTouchX = x;
  lastTouchY = y;
}

// --------------------------------------------------------------------------
// TOUCH MOVE
// --------------------------------------------------------------------------

void moveTouch(
  int x,
  int y
) {
  if (!fingerDown) {
    beginTouch(x, y);
    return;
  }

  if (
    curScreen != S_HOME
  ) {
    int deltaY =
      lastTouchY - y;

    if (
      abs(deltaY) >= 2
    ) {
      targetScrollY += deltaY;

      targetScrollY =
        constrain(
          targetScrollY,
          0,
          maxScroll
        );
    }
  }

  lastTouchX = x;
  lastTouchY = y;
}

// --------------------------------------------------------------------------
// TOUCH END
// --------------------------------------------------------------------------

void endTouch() {
  if (!fingerDown)
    return;

  int movementX =
    abs(
      lastTouchX -
      touchStartX
    );

  int movementY =
    abs(
      lastTouchY -
      touchStartY
    );

  int movement =
    max(
      movementX,
      movementY
    );

  // ------------------------------------------------------------
  // TAP
  // ------------------------------------------------------------

  if (
    movement < 15
  ) {
    // CONTENT SCREEN
    if (
      curScreen != S_HOME
    ) {
      if (
        touchStartY < HDR_H
      ) {
        curScreen = S_HOME;

        scrollY = 0;
        targetScrollY = 0;
        maxScroll = 0;

        startTransition();
      }
    }

    // HOME
    else {
      if (
        touchStartY > HDR_H
      ) {
        const int cardW = 140;
        const int cardH = 90;

        const int x0[3] = {
          15, 170, 325
        };

        const int y0[2] = {
          90, 195
        };

        for (
          int i = 0;
          i < 6;
          i++
        ) {
          int cx =
            x0[i % 3];

          int cy =
            y0[i / 3];

          if (
            touchStartX >= cx &&
            touchStartX < cx + cardW &&
            touchStartY >= cy &&
            touchStartY < cy + cardH
          ) {
            curScreen =
              (Screen)(i + 1);

            scrollY = 0;
            targetScrollY = 0;
            maxScroll = 0;

            startTransition();

            break;
          }
        }
      }
    }
  }

  // Reset touch state
  fingerDown = false;

  touchStartX = -1;
  touchStartY = -1;

  lastTouchX = -1;
  lastTouchY = -1;
}

// --------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  delay(300);

  if (!gfx->begin()) {
    Serial.println(
      "TFT initialization FAILED"
    );

    while (1) {
      delay(100);
    }
  }

  // The constructor already uses rotation 1.
  // This is the verified 480x320 landscape configuration.

  BG =
    gfx->color565(
      11, 18, 42
    );

  PANEL =
    gfx->color565(
      23, 32, 64
    );

  CARD =
    gfx->color565(
      28, 42, 84
    );

  GOLD =
    gfx->color565(
      214, 178, 96
    );

  INK =
    gfx->color565(
      240, 243, 250
    );

  SUB =
    gfx->color565(
      176, 186, 214
    );

  SHADOW =
    gfx->color565(
      6, 10, 26
    );

  Serial.println();
  Serial.println(
    "=========================================="
  );
  Serial.println(
    "AMBEDKAR MEMORIAL UI"
  );
  Serial.println(
    "480 x 320 LANDSCAPE"
  );
  Serial.println(
    "FPS BENCHMARK ENABLED"
  );
  Serial.println(
    "=========================================="
  );

  fpsWindowStart = millis();

  renderCurrentScreen();
}

// --------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------

void loop() {
  // ------------------------------------------------------------
  // Animation
  // ------------------------------------------------------------

  if (isAnimating) {
    handleAnimation();
  }

  // ------------------------------------------------------------
  // Smooth scroll
  // ------------------------------------------------------------

  if (
    !isAnimating &&
    scrollY != targetScrollY &&
    curScreen != S_HOME
  ) {
    int diff =
      targetScrollY -
      scrollY;

    // Frame-rate independent-ish smoothing:
    // larger distance = larger step.
    int step =
      diff / 4;

    if (step == 0) {
      step =
        (diff > 0)
        ? 1
        : -1;
    }

    scrollY += step;

    if (
      abs(
        targetScrollY -
        scrollY
      ) <= 1
    ) {
      scrollY =
        targetScrollY;
    }

    renderCurrentScreen();
  }

  // ------------------------------------------------------------
  // Touch
  // ------------------------------------------------------------

  int x = 0;
  int y = 0;

  if (
    touchRead(
      x,
      y
    )
  ) {
    if (!fingerDown) {
      beginTouch(
        x,
        y
      );
    }
    else {
      moveTouch(
        x,
        y
      );
    }
  }
  else {
    if (fingerDown) {
      endTouch();
    }
  }

  // Very small cooperative delay.
  delay(1);
}
