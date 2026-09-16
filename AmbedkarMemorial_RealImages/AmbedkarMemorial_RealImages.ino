/*
============================================================================
Dr. B. R. Ambedkar National Memorial — Fixed Story Card UI + Embedded Images
Arduino UNO R4 WiFi + MAR3501 / ILI9486
480 x 320 LANDSCAPE

Navigation:
HOME
  -> LIFE
       -> swipe left/right between fixed story cards
  -> other sections (placeholder cards for now)

Important:
- No vertical scrolling
- No continuous animation
- One fixed 480x320 screen is rendered at a time
- Existing verified touch calibration is preserved
============================================================================
*/

#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "touch.h"
#include "ambedkar_images.h"

// ============================================================================
// DISPLAY
// ============================================================================

Arduino_DataBus *bus = new Arduino_UNOPAR8();

Arduino_GFX *gfx =
  new Arduino_ILI9486(
    bus,
    A4,
    1,
    false
  );

#define SCR_W 480
#define SCR_H 320
#define HDR_H 58

// ============================================================================
// COLORS
// ============================================================================

uint16_t BG;
uint16_t NAVY;
uint16_t CARD;
uint16_t GOLD;
uint16_t WHITE;
uint16_t MUTED;
uint16_t DARK;

// ============================================================================
// SCREEN MODEL
// ============================================================================

enum Screen {
  HOME,
  LIFE,
  TIMELINE,
  CONSTITUTION,
  QUOTES,
  LEGACY,
  VISIT
};

Screen currentScreen = HOME;

// ============================================================================
// LIFE STORY CARDS
// ============================================================================

struct StoryCard {
  const char* eyebrow;
  const char* title;
  const char* date;
  const char* body;
  const char* tag;
};

const StoryCard lifeCards[] = {

  {
    "THE BEGINNING",
    "A LIFE OF PURPOSE",
    "14 APRIL 1891",
    "Bhimrao Ramji Ambedkar was born at Mhow in Madhya Pradesh. "
    "From these beginnings grew a life devoted to education, equality "
    "and social justice.",
    "MHOW"
  },

  {
    "EDUCATION",
    "THE POWER OF KNOWLEDGE",
    "1913 → 1916",
    "Ambedkar went to the United States for higher studies at Columbia "
    "University, where he earned his M.A. and Ph.D. He later continued "
    "his studies in London.",
    "COLUMBIA"
  },

  {
    "SOCIAL REFORM",
    "A FIGHT FOR DIGNITY",
    "1927",
    "Ambedkar led the Mahad Satyagraha, asserting the right of "
    "oppressed communities to access public water. His reform movement "
    "challenged caste discrimination and demanded dignity.",
    "MAHAD"
  },

  {
    "THE CONSTITUTION",
    "ARCHITECT OF DEMOCRACY",
    "1947",
    "After Independence, Ambedkar became India's first Law Minister. "
    "He was elected Chairman of the Constitution Drafting Committee.",
    "CONSTITUTION"
  },

  {
    "FINAL CHAPTER",
    "A LASTING LEGACY",
    "1956",
    "Ambedkar embraced Buddhism at Nagpur on 14 October 1956. "
    "He passed away on 6 December 1956 at 26 Alipur Road, Delhi.",
    "LEGACY"
  }
};

const int LIFE_COUNT =
  sizeof(lifeCards) / sizeof(lifeCards[0]);

int lifePage = 0;

// ============================================================================
// TOUCH STATE
// ============================================================================

bool fingerDown = false;

int touchStartX = -1;
int touchStartY = -1;

int lastTouchX = -1;
int lastTouchY = -1;

// ============================================================================
// FPS
// ============================================================================

unsigned long fpsStart = 0;
unsigned long fpsFrames = 0;
float fps = 0;

bool showFPS = true;

// ============================================================================
// TEXT HELPERS
// ============================================================================

void centerText(
  const char* text,
  int cx,
  int baseline,
  uint16_t color,
  const GFXfont* font
) {
  gfx->setFont(font);
  gfx->setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;

  gfx->getTextBounds(
    (char*)text,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  gfx->setCursor(
    cx - (w / 2) - x1,
    baseline
  );

  gfx->print(text);
}


// ============================================================================
// SIMPLE WRAPPED TEXT
// ============================================================================

void drawWrappedText(
  const char* text,
  int x,
  int y,
  int maxWidth,
  int lineHeight,
  uint16_t color,
  const GFXfont* font
) {
  gfx->setFont(font);
  gfx->setTextColor(color);

  String word = "";
  String line = "";

  int cursorY = y;

  for (int i = 0;; i++) {

    char c = text[i];

    if (c == ' ' || c == '\0') {

      String candidate =
        line.length() == 0
          ? word
          : line + " " + word;

      int16_t x1, y1;
      uint16_t w, h;

      gfx->getTextBounds(
        candidate.c_str(),
        0,
        0,
        &x1,
        &y1,
        &w,
        &h
      );

      if (
        line.length() > 0 &&
        w > maxWidth
      ) {
        gfx->setCursor(
          x,
          cursorY
        );

        gfx->print(line);

        cursorY += lineHeight;

        line = word;
      }
      else {
        line = candidate;
      }

      word = "";

      if (c == '\0')
        break;
    }
    else {
      word += c;
    }
  }
}


// ============================================================================
// HEADER
// ============================================================================

void drawHeader(
  const char* title
) {
  gfx->fillRect(
    0,
    0,
    SCR_W,
    HDR_H,
    NAVY
  );

  // Back button area
  if (currentScreen != HOME) {

    gfx->fillTriangle(
      18, 29,
      32, 19,
      32, 39,
      GOLD
    );

    gfx->setFont(
      &FreeSans9pt7b
    );

    gfx->setTextColor(
      CARD
    );

    gfx->setCursor(
      42,
      35
    );

    gfx->print(
      "BACK"
    );
  }

  centerText(
    title,
    SCR_W / 2 + (currentScreen == HOME ? 0 : 20),
    38,
    CARD,
    &FreeSansBold9pt7b
  );

  gfx->fillRect(
    0,
    HDR_H - 2,
    SCR_W,
    2,
    GOLD
  );
}


// ============================================================================
// HOME SCREEN
// ============================================================================

void drawHome() {

  gfx->fillScreen(
    BG
  );

  drawHeader(
    "DR. B. R. AMBEDKAR"
  );

  centerText(
    "NATIONAL MEMORIAL",
    SCR_W / 2,
    80,
    NAVY,
    &FreeSans9pt7b
  );

  const int cardW = 140;
  const int cardH = 82;

  const int x[3] = {
    15, 170, 325
  };

  const int y[2] = {
    105, 205
  };

  const char* labels[6] = {
    "LIFE",
    "TIMELINE",
    "CONSTITUTION",
    "QUOTES",
    "LEGACY",
    "VISIT"
  };

  for (int i = 0; i < 6; i++) {

    int cx = x[i % 3];
    int cy = y[i / 3];

    gfx->fillRoundRect(
      cx + 3,
      cy + 4,
      cardW,
      cardH,
      10,
      DARK
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
      NAVY
    );

    centerText(
      labels[i],
      cx + cardW / 2,
      cy + 48,
      WHITE,
      &FreeSansBold9pt7b
    );
  }
}


// ============================================================================
// LIFE CARD
// ============================================================================

void drawLifeCard() {

  const StoryCard &card =
    lifeCards[lifePage];

  gfx->fillScreen(BG);

  drawHeader("LIFE");

  // ----------------------------------------------------------
  // Main card
  // ----------------------------------------------------------

  gfx->fillRoundRect(
    18, 74,
    444, 204,
    14,
    CARD
  );

  gfx->drawRoundRect(
    18, 74,
    444, 204,
    14,
    NAVY
  );

  // ----------------------------------------------------------
  // REAL EMBEDDED IMAGE
  // ----------------------------------------------------------
  //
  // Images are pre-cropped to 90x120 RGB565 so the UNO R4 can
  // render them directly from flash without an SD card.
  //
  // The portrait image is centered inside a larger framed zone.
  // This avoids shrinking the typography just to accommodate
  // photographs with different original aspect ratios.

  const int IX = 53;
  const int IY = 116;

  gfx->fillRoundRect(
    32, 89,
    132, 174,
    10,
    DARK
  );

  gfx->drawRoundRect(
    32, 89,
    132, 174,
    10,
    GOLD
  );

  if (lifePage == 0) {
    gfx->draw16bitRGBBitmap(
      IX, IY,
      life_01,
      LIFE_IMG_W,
      LIFE_IMG_H
    );
  }
  else if (lifePage == 1) {
    gfx->draw16bitRGBBitmap(
      IX, IY,
      life_02,
      LIFE_IMG_W,
      LIFE_IMG_H
    );
  }
  else if (lifePage == 2) {
    gfx->draw16bitRGBBitmap(
      IX, IY,
      life_03,
      LIFE_IMG_W,
      LIFE_IMG_H
    );
  }
  else if (lifePage == 3) {
    gfx->draw16bitRGBBitmap(
      IX, IY,
      life_04,
      LIFE_IMG_W,
      LIFE_IMG_H
    );
  }
  else {
    gfx->draw16bitRGBBitmap(
      IX, IY,
      life_05,
      LIFE_IMG_W,
      LIFE_IMG_H
    );
  }

  // ----------------------------------------------------------
  // Text zone
  //
  // IMPORTANT:
  // Everything from TX onward stays inside a fixed 260px-wide
  // column. Body copy is intentionally short enough to occupy
  // only the reserved region and NEVER collide with the tag.
  // ----------------------------------------------------------

  const int TX = 184;
  const int TW = 244;

  // Eyebrow
  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    TX,
    106
  );

  gfx->print(
    card.eyebrow
  );

  // Main title
  // Keep the main title large, but use the 9pt bold face only
  // for the two titles that are too wide for the 244px column.
  if (
    lifePage == 1 ||
    lifePage == 3
  ) {
    gfx->setFont(
      &FreeSansBold9pt7b
    );
  }
  else {
    gfx->setFont(
      &FreeSansBold12pt7b
    );
  }

  gfx->setTextColor(
    WHITE
  );

  gfx->setCursor(
    TX,
    139
  );

  gfx->print(
    card.title
  );

  // Date
  gfx->fillRoundRect(
    TX,
    150,
    180,
    27,
    7,
    NAVY
  );

  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    CARD
  );

  gfx->setCursor(
    TX + 10,
    169
  );

  gfx->print(
    card.date
  );

  // ----------------------------------------------------------
  // Body region
  // ----------------------------------------------------------

  // Clear the body region explicitly before writing.
  gfx->fillRect(
    TX,
    187,
    TW,
    49,
    CARD
  );

  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    MUTED
  );

  // Draw only the first ~2-3 lines inside the bounded region.
  // The text has been deliberately shortened for the physical
  // 480x320 screen.

  if (lifePage == 0) {

    gfx->setCursor(TX, 202);
    gfx->print("Born at Mhow in 1891.");

    gfx->setCursor(TX, 221);
    gfx->print("A life shaped by education,");

    gfx->setCursor(TX, 240);
    gfx->print("equality and social justice.");
  }

  else if (lifePage == 1) {

    gfx->setCursor(TX, 202);
    gfx->print("Higher studies at");

    gfx->setCursor(TX, 221);
    gfx->print("Columbia University, followed");

    gfx->setCursor(TX, 240);
    gfx->print("by studies in London.");
  }

  else if (lifePage == 2) {

    gfx->setCursor(TX, 202);
    gfx->print("Mahad Satyagraha, 1927.");

    gfx->setCursor(TX, 221);
    gfx->print("A demand for equal access,");

    gfx->setCursor(TX, 240);
    gfx->print("dignity and social justice.");
  }

  else if (lifePage == 3) {

    gfx->setCursor(TX, 202);
    gfx->print("India's first Law Minister");

    gfx->setCursor(TX, 221);
    gfx->print("and Chairman of the");

    gfx->setCursor(TX, 240);
    gfx->print("Constitution Drafting Committee.");
  }

  else {

    gfx->setCursor(TX, 202);
    gfx->print("Embraced Buddhism in 1956.");

    gfx->setCursor(TX, 221);
    gfx->print("His ideas on liberty,");

    gfx->setCursor(TX, 240);
    gfx->print("equality and justice endure.");
  }

  // ----------------------------------------------------------
  // Tag
  // ----------------------------------------------------------

  gfx->fillRoundRect(
    TX,
    246,
    130,
    22,
    6,
    NAVY
  );

  centerText(
    card.tag,
    TX + 65,
    262,
    CARD,
    &FreeSansBold9pt7b
  );

  // ----------------------------------------------------------
  // Page indicator
  // ----------------------------------------------------------

  int dotsStart =
    SCR_W / 2 -
    ((LIFE_COUNT - 1) * 9);

  for (
    int i = 0;
    i < LIFE_COUNT;
    i++
  ) {

    uint16_t c =
      (i == lifePage)
        ? GOLD
        : MUTED;

    gfx->fillCircle(
      dotsStart + i * 18,
      299,
      (i == lifePage) ? 4 : 3,
      c
    );
  }

  // Swipe hints
  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    MUTED
  );

  if (lifePage > 0) {
    gfx->setCursor(
      22,
      305
    );
    gfx->print("<");
  }

  if (lifePage < LIFE_COUNT - 1) {
    gfx->setCursor(
      448,
      305
    );
    gfx->print(">");
  }
}


// ============================================================================
// PLACEHOLDER SECTION
// ============================================================================

void drawPlaceholder(
  const char* title
) {

  gfx->fillScreen(
    BG
  );

  drawHeader(
    title
  );

  gfx->fillRoundRect(
    35,
    90,
    410,
    175,
    14,
    CARD
  );

  centerText(
    "SECTION READY",
    SCR_W / 2,
    155,
    NAVY,
    &FreeSansBold12pt7b
  );

  centerText(
    "Content will be added next.",
    SCR_W / 2,
    190,
    MUTED,
    &FreeSans9pt7b
  );
}


// ============================================================================
// DRAW CURRENT SCREEN
// ============================================================================

void renderScreen() {

  switch (currentScreen) {

    case HOME:
      drawHome();
      break;

    case LIFE:
      drawLifeCard();
      break;

    case TIMELINE:
      drawPlaceholder("TIMELINE");
      break;

    case CONSTITUTION:
      drawPlaceholder("CONSTITUTION");
      break;

    case QUOTES:
      drawPlaceholder("QUOTES");
      break;

    case LEGACY:
      drawPlaceholder("LEGACY");
      break;

    case VISIT:
      drawPlaceholder("VISIT");
      break;
  }

  fpsFrames++;

  unsigned long now =
    millis();

  if (
    fpsStart == 0
  ) {
    fpsStart = now;
  }

  if (
    now - fpsStart >= 1000
  ) {

    fps =
      (float)fpsFrames *
      1000.0f /
      (float)(now - fpsStart);

    fpsFrames = 0;
    fpsStart = now;

    Serial.print(
      "Render FPS: "
    );

    Serial.println(
      fps,
      1
    );
  }

  if (showFPS) {

    gfx->setFont(
      &FreeSans9pt7b
    );

    gfx->setTextColor(
      MUTED
    );

    gfx->setCursor(
      408,
      17
    );

    gfx->print(
      fps,
      1
    );

    gfx->print(
      "F"
    );
  }
}


// ============================================================================
// TOUCH BEGIN
// ============================================================================

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


// ============================================================================
// TOUCH MOVE
// ============================================================================

void moveTouch(
  int x,
  int y
) {
  lastTouchX = x;
  lastTouchY = y;
}


// ============================================================================
// TOUCH END
// ============================================================================

void endTouch() {

  if (!fingerDown)
    return;

  int dx =
    lastTouchX -
    touchStartX;

  int dy =
    lastTouchY -
    touchStartY;

  // ----------------------------------------------------------
  // LIFE: HORIZONTAL SWIPE
  // ----------------------------------------------------------

  if (
    currentScreen == LIFE &&
    abs(dx) >= 45 &&
    abs(dx) > abs(dy)
  ) {

    // Swipe left -> next
    if (
      dx < 0 &&
      lifePage < LIFE_COUNT - 1
    ) {
      lifePage++;
      renderScreen();
    }

    // Swipe right -> previous
    else if (
      dx > 0 &&
      lifePage > 0
    ) {
      lifePage--;
      renderScreen();
    }
  }

  // ----------------------------------------------------------
  // TAP
  // ----------------------------------------------------------

  else if (
    abs(dx) < 15 &&
    abs(dy) < 15
  ) {

    // --------------------------------------------------------
    // Back
    // --------------------------------------------------------

    if (
      currentScreen != HOME &&
      touchStartY < HDR_H
    ) {

      currentScreen =
        HOME;

      lifePage = 0;

      renderScreen();
    }

    // --------------------------------------------------------
    // Home cards
    // --------------------------------------------------------

    else if (
      currentScreen == HOME &&
      touchStartY > 95
    ) {

      const int cardW = 140;
      const int cardH = 82;

      const int x[3] = {
        15, 170, 325
      };

      const int y[2] = {
        105, 205
      };

      for (
        int i = 0;
        i < 6;
        i++
      ) {

        int cx =
          x[i % 3];

        int cy =
          y[i / 3];

        if (
          touchStartX >= cx &&
          touchStartX < cx + cardW &&
          touchStartY >= cy &&
          touchStartY < cy + cardH
        ) {

          currentScreen =
            (Screen)(i + 1);

          lifePage = 0;

          renderScreen();

          break;
        }
      }
    }
  }

  fingerDown = false;

  touchStartX = -1;
  touchStartY = -1;

  lastTouchX = -1;
  lastTouchY = -1;
}


// ============================================================================
// SETUP
// ============================================================================

void setup() {

  Serial.begin(
    115200
  );

  delay(300);

  if (
    !gfx->begin()
  ) {

    Serial.println(
      "TFT initialization FAILED"
    );

    while (1)
      delay(100);
  }

  gfx->setRotation(
    1
  );

  // Heritage / museum palette
  // Warm ivory background + deep indigo + restrained antique gold.

  BG =
    gfx->color565(
      244, 239, 228
    );

  NAVY =
    gfx->color565(
      38, 53, 95
    );

  CARD =
    gfx->color565(
      255, 249, 238
    );

  GOLD =
    gfx->color565(
      185, 130, 50
    );

  WHITE =
    gfx->color565(
      36, 38, 43
    );

  MUTED =
    gfx->color565(
      101, 101, 106
    );

  DARK =
    gfx->color565(
      224, 216, 199
    );

  Serial.println();
  Serial.println(
    "=========================================="
  );
  Serial.println(
    "AMBEDKAR MEMORIAL"
  );
  Serial.println(
    "FIXED STORY CARD UI"
  );
  Serial.println(
    "480 x 320 LANDSCAPE"
  );
  Serial.println(
    "NO VERTICAL SCROLL"
  );
  Serial.println(
    "=========================================="
  );

  fpsStart = millis();

  renderScreen();
}


// ============================================================================
// LOOP
// ============================================================================

void loop() {

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

  // No artificial animation delay.
  // The loop is deliberately lightweight.
}
