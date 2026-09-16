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
#include <SPI.h>
#include <SdFat.h>

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

// ============================================================================
// SD IMAGE STORAGE
// ============================================================================

#define SD_CS 10

bool homeScreen = true;
#define IMG_W 160
#define IMG_H 170
#define IMG_X 28
#define IMG_Y 92

SdFat sd;
bool sdOK = false;

// One scanline = 160 pixels = 320 bytes.
// Keeping only one row in RAM avoids a large framebuffer.
uint16_t imageRow[IMG_W];


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

bool showFPS = false;

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

  // ----------------------------------------------------------
  // Main content panel
  // ----------------------------------------------------------

  gfx->fillRoundRect(
    18,
    76,
    444,
    202,
    14,
    CARD
  );

  gfx->drawRoundRect(
    18,
    76,
    444,
    202,
    14,
    NAVY
  );

  // ----------------------------------------------------------
  // Image panel
  // ----------------------------------------------------------

  // ----------------------------------------------------------
  // SD-BACKED LARGE IMAGE
  // ----------------------------------------------------------

  gfx->fillRoundRect(
    IMG_X - 3,
    IMG_Y - 3,
    IMG_W + 6,
    IMG_H + 6,
    10,
    DARK
  );

  gfx->drawRoundRect(
    IMG_X - 3,
    IMG_Y - 3,
    IMG_W + 6,
    IMG_H + 6,
    10,
    GOLD
  );

  char path[24];
  snprintf(
    path,
    sizeof(path),
    "/life%d.rgb565",
    lifePage + 1
  );

  if (sdOK) {

    FsFile image = sd.open(
      path,
      O_RDONLY
    );

    if (image) {

      for (int row = 0; row < IMG_H; row++) {

        for (int col = 0; col < IMG_W; col++) {

          int hi = image.read();
          int lo = image.read();

          if (hi < 0 || lo < 0) {
            image.close();

            gfx->fillRoundRect(
              IMG_X,
              IMG_Y,
              IMG_W,
              IMG_H,
              7,
              DARK
            );

            centerText(
              "IMAGE ERROR",
              IMG_X + IMG_W / 2,
              IMG_Y + IMG_H / 2 + 5,
              MUTED,
              &FreeSansBold9pt7b
            );

            goto image_done;
          }

          imageRow[col] =
            ((uint16_t)hi << 8) |
            (uint16_t)lo;
        }

        gfx->draw16bitRGBBitmap(
          IMG_X,
          IMG_Y + row,
          imageRow,
          IMG_W,
          1
        );
      }

      image.close();

    } else {

      gfx->fillRoundRect(
        IMG_X,
        IMG_Y,
        IMG_W,
        IMG_H,
        7,
        DARK
      );

      centerText(
        "NO IMAGE",
        IMG_X + IMG_W / 2,
        IMG_Y + IMG_H / 2 + 5,
        MUTED,
        &FreeSansBold9pt7b
      );
    }

  } else {

    gfx->fillRoundRect(
      IMG_X,
      IMG_Y,
      IMG_W,
      IMG_H,
      7,
      DARK
    );

    centerText(
      "SD ERROR",
      IMG_X + IMG_W / 2,
      IMG_Y + IMG_H / 2 + 5,
      MUTED,
      &FreeSansBold9pt7b
    );
  }

image_done:
  ;

  // ----------------------------------------------------------
  // Right-side typography
  // ----------------------------------------------------------

  const int TX = 208;

  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    TX,
    108
  );

  gfx->print(
    card.eyebrow
  );

  // Two long titles use the smaller bold face so they remain
  // on one line in the available 244px text column.
  if (lifePage == 1 || lifePage == 3) {
    gfx->setFont(&FreeSansBold9pt7b);
  } else {
    gfx->setFont(&FreeSansBold12pt7b);
  }

  gfx->setTextColor(
    WHITE
  );

  gfx->setCursor(
    TX,
    143
  );

  gfx->print(
    card.title
  );

  // Date gets its own clear visual line.
  gfx->fillRoundRect(
    TX,
    153,
    190,
    28,
    7,
    NAVY
  );

  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    TX + 10,
    173
  );

  gfx->print(
    card.date
  );

  // Body intentionally limited to a small amount of text.
  drawWrappedText(
    card.body,
    TX,
    202,
    238,
    20,
    MUTED,
    &FreeSans9pt7b
  );

  // ----------------------------------------------------------
  // Tag
  // ----------------------------------------------------------

  gfx->fillRoundRect(
    TX,
    246,
    120,
    22,
    6,
    NAVY
  );

  centerText(
    card.tag,
    TX + 62,
    262,
    GOLD,
    &FreeSansBold9pt7b
  );

  // ----------------------------------------------------------
  // Page indicators
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
      300,
      i == lifePage ? 4 : 3,
      c
    );
  }

  // Small swipe affordances.
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

  if (homeScreen) {
    renderHome();
    return;
  }



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


// ============================================================================
// STARTUP ANIMATION
// ============================================================================
// Deliberately uses only a handful of full-screen redraws.
// This avoids the repeated-redraw problem we encountered during scrolling.

void startupScreenBase() {
  gfx->fillScreen(BG);

  // Thin heritage frame.
  gfx->drawRect(8, 8, 464, 304, 0x7BEF);
  gfx->drawRect(11, 11, 458, 298, 0x7BEF);

  // Central vertical accent.
  gfx->fillRect(
    239, 52,
    2, 216,
    GOLD
  );
}

void startupStage1() {
  startupScreenBase();

  // Small central gold point.
  gfx->fillCircle(
    240, 160,
    4,
    GOLD
  );
}

void startupStage2() {
  startupScreenBase();

  // Expanding horizontal line.
  gfx->fillRect(
    100, 159,
    280, 2,
    GOLD
  );

  gfx->fillCircle(240, 160, 5, GOLD);
}

void startupStage3() {
  startupScreenBase();

  centerText(
    "DR. B. R.",
    240,
    135,
    GOLD,
    &FreeSansBold12pt7b
  );

  centerText(
    "AMBEDKAR",
    240,
    169,
    WHITE,
    &FreeSansBold12pt7b
  );

  gfx->fillRect(
    155, 187,
    170, 2,
    GOLD
  );
}

void startupStage4() {
  startupScreenBase();

  centerText(
    "DR. B. R. AMBEDKAR",
    240,
    130,
    WHITE,
    &FreeSansBold12pt7b
  );

  centerText(
    "NATIONAL MEMORIAL",
    240,
    162,
    GOLD,
    &FreeSansBold9pt7b
  );

  centerText(
    "26 ALIPUR ROAD  •  DELHI",
    240,
    194,
    MUTED,
    &FreeSans9pt7b
  );
}

void startupStage5() {
  startupScreenBase();

  centerText(
    "DR. B. R. AMBEDKAR",
    240,
    112,
    WHITE,
    &FreeSansBold12pt7b
  );

  centerText(
    "NATIONAL MEMORIAL",
    240,
    144,
    GOLD,
    &FreeSansBold9pt7b
  );

  gfx->fillRect(
    145, 170,
    190, 2,
    GOLD
  );

  centerText(
    "EDUCATE  •  AGITATE  •  ORGANIZE",
    240,
    205,
    MUTED,
    &FreeSans9pt7b
  );
}

void playStartupAnimation() {

  startupStage1();
  delay(350);

  startupStage2();
  delay(350);

  startupStage3();
  delay(700);

  startupStage4();
  delay(750);

  startupStage5();
  delay(950);
}

// ============================================================================
// HOME SCREEN
// ============================================================================

void drawHomeButton(
  int x,
  int y,
  int w,
  int h,
  const char* title,
  const char* subtitle
) {
  gfx->fillRoundRect(
    x, y, w, h,
    10,
    CARD
  );

  gfx->drawRoundRect(
    x, y, w, h,
    10,
    0x7BEF
  );

  gfx->setFont(&FreeSansBold9pt7b);
  gfx->setTextColor(WHITE);
  gfx->setCursor(x + 14, y + 25);
  gfx->print(title);

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(MUTED);
  gfx->setCursor(x + 14, y + 47);
  gfx->print(subtitle);
}

void renderHome() {

  gfx->fillScreen(BG);

  // Header.
  gfx->fillRect(
    0, 0,
    480, 58,
    0x2104
  );

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(WHITE);
  gfx->setCursor(22, 28);
  gfx->print("DR. B. R. AMBEDKAR");

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(GOLD);
  gfx->setCursor(22, 48);
  gfx->print("NATIONAL MEMORIAL");

  // Small vertical divider.
  gfx->fillRect(
    330, 15,
    2, 30,
    GOLD
  );

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(MUTED);
  gfx->setCursor(348, 34);
  gfx->print("EXPLORE HIS STORY");

  // Six fixed cards.
  const int x1 = 18;
  const int x2 = 246;
  const int w = 216;
  const int h = 61;

  drawHomeButton(
    x1, 72, w, h,
    "LIFE",
    "A journey of purpose"
  );

  drawHomeButton(
    x2, 72, w, h,
    "TIMELINE",
    "1891 — 1956"
  );

  drawHomeButton(
    x1, 143, w, h,
    "CONSTITUTION",
    "Liberty • Equality • Justice"
  );

  drawHomeButton(
    x2, 143, w, h,
    "IDEAS",
    "Thoughts that shaped society"
  );

  drawHomeButton(
    x1, 214, w, h,
    "LEGACY",
    "Ideas that endure"
  );

  drawHomeButton(
    x2, 214, w, h,
    "VISIT",
    "26 Alipur Road • Delhi"
  );

  // Bottom status strip.
  gfx->fillRoundRect(
    18, 286,
    444, 23,
    7,
    CARD
  );

  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(MUTED);
  gfx->setCursor(31, 302);
  gfx->print("SELECT A SECTION TO BEGIN");
}


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

  Serial.println("Initializing SD...");

  if (sd.begin(
        SdSpiConfig(
          SD_CS,
          SHARED_SPI,
          SD_SCK_MHZ(4)
        )
      )) {
    sdOK = true;
    Serial.println("SD: OK");
  } else {
    sdOK = false;
    Serial.println("SD: FAILED");
    sd.printSdError(&Serial);
  }

  fpsStart = millis();

  // Show the startup sequence once after SD initialization.
  playStartupAnimation();

  homeScreen = true;
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
