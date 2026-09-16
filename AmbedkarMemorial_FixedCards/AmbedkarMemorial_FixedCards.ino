/*
============================================================================
Dr. B. R. Ambedkar National Memorial — Fixed Story Card UI
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
  const char* body;
  const char* tag;
};

const StoryCard lifeCards[] = {

  {
    "THE BEGINNING",
    "A LIFE OF PURPOSE",
    "Born in 1891 at Mhow, Bhimrao Ramji Ambedkar grew into one of "
    "India's most influential jurists, thinkers and social reformers.",
    "1891"
  },

  {
    "EDUCATION",
    "THE POWER OF KNOWLEDGE",
    "Ambedkar pursued higher education at Columbia University and the "
    "London School of Economics, building the intellectual foundation "
    "for his lifelong work for equality.",
    "COLUMBIA • LSE"
  },

  {
    "SOCIAL REFORM",
    "A FIGHT FOR DIGNITY",
    "He challenged caste discrimination and led movements demanding "
    "equal access to public spaces, education and social dignity.",
    "EQUALITY"
  },

  {
    "THE CONSTITUTION",
    "ARCHITECT OF DEMOCRACY",
    "As Chairman of the Constitution Drafting Committee, Ambedkar "
    "played a central role in shaping India's constitutional framework.",
    "1947"
  },

  {
    "FINAL CHAPTER",
    "A LASTING LEGACY",
    "In 1956, Ambedkar embraced Buddhism at Nagpur. His ideas on "
    "equality, liberty and justice continue to shape public life.",
    "1956"
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
      MUTED
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
    WHITE,
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
    GOLD,
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

  gfx->fillScreen(
    BG
  );

  drawHeader(
    "LIFE"
  );

  // Main card
  gfx->fillRoundRect(
    18,
    76,
    444,
    210,
    14,
    CARD
  );

  gfx->drawRoundRect(
    18,
    76,
    444,
    210,
    14,
    NAVY
  );

  // Image area / visual placeholder
  gfx->fillRoundRect(
    34,
    92,
    128,
    178,
    10,
    DARK
  );

  gfx->drawRoundRect(
    34,
    92,
    128,
    178,
    10,
    GOLD
  );

  centerText(
    "IMAGE",
    98,
    184,
    MUTED,
    &FreeSansBold9pt7b
  );

  // Text
  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    185,
    108
  );

  gfx->print(
    card.eyebrow
  );

  gfx->setFont(
    &FreeSansBold12pt7b
  );

  gfx->setTextColor(
    WHITE
  );

  gfx->setCursor(
    185,
    145
  );

  gfx->print(
    card.title
  );

  drawWrappedText(
    card.body,
    185,
    177,
    245,
    22,
    MUTED,
    &FreeSans9pt7b
  );

  // Tag
  gfx->fillRoundRect(
    185,
    242,
    125,
    26,
    7,
    NAVY
  );

  centerText(
    card.tag,
    247,
    260,
    GOLD,
    &FreeSansBold9pt7b
  );

  // Page indicators
  int dotsStart =
    SCR_W / 2 -
    ((LIFE_COUNT - 1) * 9);

  for (int i = 0; i < LIFE_COUNT; i++) {

    uint16_t c =
      (i == lifePage)
      ? GOLD
      : MUTED;

    gfx->fillCircle(
      dotsStart + i * 18,
      303,
      i == lifePage ? 4 : 3,
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

  gfx->setCursor(
    22,
    307
  );

  gfx->print(
    lifePage > 0 ? "<" : ""
  );

  gfx->setCursor(
    448,
    307
  );

  gfx->print(
    lifePage < LIFE_COUNT - 1 ? ">" : ""
  );
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
    GOLD,
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

  BG =
    gfx->color565(
      10, 17, 39
    );

  NAVY =
    gfx->color565(
      24, 37, 75
    );

  CARD =
    gfx->color565(
      31, 45, 87
    );

  GOLD =
    gfx->color565(
      214, 178, 96
    );

  WHITE =
    gfx->color565(
      240, 243, 250
    );

  MUTED =
    gfx->color565(
      165, 177, 207
    );

  DARK =
    gfx->color565(
      6, 11, 27
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
