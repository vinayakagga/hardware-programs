/*
============================================================================
DR. B. R. AMBEDKAR NATIONAL MEMORIAL — FULL TFT EXPERIENCE
Arduino UNO R4 WiFi + MAR3501 / ILI9486
480 x 320 LANDSCAPE

STATIC SPLASH -> HOME
HOME -> LIFE / TIMELINE / CONSTITUTION / IDEAS / LEGACY / VISIT

Performance:
- No continuous animation
- No vertical scrolling
- One complete screen rendered per navigation action
- Images read from SD one scanline at a time
- Touch is event-based
- SdFat used directly
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


#define SCR_W 480
#define SCR_H 320

#define SD_CS 10


SdFat sd;
bool sdOK = false;


// One scanline.
uint16_t imageRow[480];


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
uint16_t BORDER;


// ============================================================================
// SCREEN MODEL
// ============================================================================

enum Screen {

  HOME,
  LIFE,
  TIMELINE,
  CONSTITUTION,
  IDEAS,
  LEGACY,
  VISIT

};

Screen currentScreen = HOME;

int pageIndex = 0;


// ============================================================================
// CONTENT MODEL
// ============================================================================

struct Page {

  const char* image;

  uint16_t iw;
  uint16_t ih;

  const char* eyebrow;
  const char* title;
  const char* date;
  const char* body;
  const char* tag;

};


// ============================================================================
// LIFE
// ============================================================================

const Page lifePages[] = {

  {
    "life1.rgb565",
    160,170,
    "THE BEGINNING",
    "A LIFE OF PURPOSE",
    "14 APRIL 1891",
    "From humble beginnings, he rose through education and determination.",
    "MHOW"
  },

  {
    "life2.rgb565",
    160,170,
    "EDUCATION",
    "POWER OF KNOWLEDGE",
    "1913 - 1916",
    "Education became his path to freedom and equality.",
    "COLUMBIA"
  },

  {
    "life3.rgb565",
    160,170,
    "SOCIAL REFORM",
    "FIGHT FOR DIGNITY",
    "1927",
    "He challenged caste injustice and fought for equal rights.",
    "MAHAD"
  },

  {
    "life4.rgb565",
    160,170,
    "THE CONSTITUTION",
    "DEMOCRACY'S ARCHITECT",
    "1947",
    "He helped shape India's Constitution around equality and justice.",
    "CONSTITUTION"
  },

  {
    "life5.rgb565",
    160,170,
    "FINAL CHAPTER",
    "A LASTING LEGACY",
    "1956",
    "His ideas continue to inspire equality and justice.",
    "LEGACY"
  }

};

const int LIFE_COUNT =
  sizeof(lifePages) /
  sizeof(lifePages[0]);


// ============================================================================
// TIMELINE
// ============================================================================

const Page timelinePages[] = {

  {
    "timeline1.rgb565",
    190,140,
    "1891",
    "A BEGINNING",
    "14 APRIL 1891",
    "Bhimrao Ramji Ambedkar was born at Mhow.",
    "BIRTH"
  },

  {
    "timeline2.rgb565",
    190,140,
    "EDUCATION",
    "THE POWER OF STUDY",
    "1913",
    "He began higher studies at Columbia University.",
    "COLUMBIA"
  },

  {
    "timeline3.rgb565",
    190,140,
    "SOCIAL REFORM",
    "MAHAD SATYAGRAHA",
    "1927",
    "The Mahad movement asserted equal access and dignity.",
    "MAHAD"
  },

  {
    "timeline4.rgb565",
    190,140,
    "PUBLIC LIFE",
    "A VOICE FOR RIGHTS",
    "1930 - 32",
    "He represented oppressed communities in major political negotiations.",
    "LONDON"
  },

  {
    "timeline5.rgb565",
    190,140,
    "CONSTITUTION",
    "A NEW NATION",
    "1947 - 49",
    "He became Law Minister and chaired the Drafting Committee.",
    "DELHI"
  }

};

const int TIMELINE_COUNT =
  sizeof(timelinePages) /
  sizeof(timelinePages[0]);


// ============================================================================
// CONSTITUTION
// ============================================================================

const Page constitutionPages[] = {

  {
    "constitution1.rgb565",
    190,140,
    "THE CONSTITUTION",
    "JUSTICE • LIBERTY",
    "1949",
    "A democratic framework built around justice, liberty and equality.",
    "PREAMBLE"
  },

  {
    "constitution2.rgb565",
    190,140,
    "DRAFTING",
    "EQUALITY",
    "1947 - 49",
    "Equality and dignity became central constitutional commitments.",
    "DIGNITY"
  },

  {
    "constitution3.rgb565",
    190,140,
    "DEMOCRACY",
    "FUNDAMENTAL RIGHTS",
    "1950",
    "Rights protect individual freedom and dignity.",
    "RIGHTS"
  }

};

const int CONSTITUTION_COUNT =
  sizeof(constitutionPages) /
  sizeof(constitutionPages[0]);


// ============================================================================
// IDEAS
// ============================================================================

const Page ideasPages[] = {

  {
    "ideas1.rgb565",
    190,140,
    "EDUCATE",
    "KNOWLEDGE",
    "",
    "Knowledge creates the power to challenge inequality.",
    "EDUCATE"
  },

  {
    "ideas2.rgb565",
    190,140,
    "AGITATE",
    "QUESTION INJUSTICE",
    "",
    "Question injustice. Demand change.",
    "AGITATE"
  },

  {
    "ideas3.rgb565",
    190,140,
    "ORGANIZE",
    "COLLECTIVE ACTION",
    "",
    "Organized action can transform society.",
    "ORGANIZE"
  }

};

const int IDEAS_COUNT =
  sizeof(ideasPages) /
  sizeof(ideasPages[0]);


// ============================================================================
// LEGACY
// ============================================================================

const Page legacyPages[] = {

  {
    "legacy1.rgb565",
    190,140,
    "THE CONSTITUTION",
    "A DEMOCRATIC LEGACY",
    "1949",
    "His constitutional work shaped India's democratic framework.",
    "CONSTITUTION"
  },

  {
    "legacy2.rgb565",
    190,140,
    "SOCIAL REFORM",
    "A LEGACY OF DIGNITY",
    "",
    "He challenged caste discrimination and fought for equality.",
    "REFORM"
  },

  {
    "legacy3.rgb565",
    190,140,
    "A CONTINUING LEGACY",
    "IDEAS THAT ENDURE",
    "1956 →",
    "His ideas remain part of India's conversation on equality.",
    "LEGACY"
  }

};

const int LEGACY_COUNT =
  sizeof(legacyPages) /
  sizeof(legacyPages[0]);


// ============================================================================
// VISIT
// ============================================================================

const Page visitPages[] = {

  {
    "visit1.rgb565",
    220,150,
    "26 ALIPUR ROAD",
    "MAHAPARINIRVAN BHOOMI",
    "DELHI",
    "Final home of Dr. Ambedkar.",
    "MEMORIAL"
  },

  {
    "visit2.rgb565",
    220,150,
    "THE MUSEUM",
    "A LIVING ARCHIVE",
    "27 EXHIBITS",
    "Explore his life and ideas.",
    "MUSEUM"
  },

  {
    "visit3.rgb565",
    220,150,
    "THE MEMORIAL",
    "OPEN BOOK",
    "",
    "A symbol of knowledge and the Constitution.",
    "ARCHITECTURE"
  }

};

const int VISIT_COUNT =
  sizeof(visitPages) /
  sizeof(visitPages[0]);


// ============================================================================
// TOUCH STATE
// ============================================================================

bool fingerDown = false;

int touchStartX = -1;
int touchStartY = -1;

int lastTouchX = -1;
int lastTouchY = -1;


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

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;

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


int textWidth(
  const char* text,
  const GFXfont* font
) {

  gfx->setFont(font);

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;

  gfx->getTextBounds(
    (char*)text,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  return (int)w;
}


// ============================================================================
// CONTROLLED TEXT WRAPPING
// ============================================================================

void drawWrappedText(
  const char* text,
  int x,
  int y,
  int maxWidth,
  int lineHeight,
  uint16_t color,
  const GFXfont* font,
  int maxLines
) {

  gfx->setFont(font);
  gfx->setTextColor(color);

  String word = "";
  String line = "";

  int cursorY = y;
  int lines = 0;

  for (int i = 0;; i++) {

    char c = text[i];

    if (c == ' ' || c == '\0') {

      String candidate =
        line.length() == 0
        ? word
        : line + " " + word;

      int16_t x1;
      int16_t y1;

      uint16_t w;
      uint16_t h;

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

        lines++;

        if (lines >= maxLines)
          return;

        line = word;

      } else {

        line = candidate;

      }

      word = "";

      if (c == '\0')
        break;

    } else {

      word += c;

    }
  }

  if (
    line.length() > 0 &&
    lines < maxLines
  ) {

    gfx->setCursor(
      x,
      cursorY
    );

    gfx->print(line);
  }
}


// ============================================================================
// IMAGE READER
// ============================================================================

bool drawRGB565(
  const char* path,
  int x,
  int y,
  int w,
  int h
) {

  if (!sdOK)
    return false;

  FsFile image =
    sd.open(
      path,
      O_RDONLY
    );

  if (!image)
    return false;


  for (int row = 0; row < h; row++) {

    for (int col = 0; col < w; col++) {

      int hi = image.read();
      int lo = image.read();

      if (
        hi < 0 ||
        lo < 0
      ) {

        image.close();

        return false;
      }

      imageRow[col] =
        ((uint16_t)hi << 8) |
        (uint16_t)lo;
    }


    gfx->draw16bitRGBBitmap(
      x,
      y + row,
      imageRow,
      w,
      1
    );
  }


  image.close();

  return true;
}


// ============================================================================
// SPLASH
// ============================================================================

void drawSplash() {

  gfx->fillScreen(BG);

  if (
    !drawRGB565(
      "splash.rgb565",
      0,
      0,
      480,
      320
    )
  ) {

    gfx->fillScreen(DARK);

    gfx->drawRect(
      10,
      10,
      460,
      300,
      GOLD
    );

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
      164,
      GOLD,
      &FreeSansBold9pt7b
    );

    centerText(
      "EDUCATE • AGITATE • ORGANIZE",
      240,
      204,
      MUTED,
      &FreeSans9pt7b
    );
  }

  delay(1500);
}


// ============================================================================
// HOME CARD
// ============================================================================

void homeCard(
  int x,
  int y,
  int w,
  int h,
  const char* label,
  const char* sub,
  int iconType
) {

  gfx->fillRoundRect(
    x + 3,
    y + 4,
    w,
    h,
    10,
    DARK
  );

  gfx->fillRoundRect(
    x,
    y,
    w,
    h,
    10,
    CARD
  );

  gfx->drawRoundRect(
    x,
    y,
    w,
    h,
    10,
    BORDER
  );


  int cx = x + 22;
  int cy = y + 28;


  gfx->drawCircle(
    cx,
    cy,
    8,
    GOLD
  );


  if (iconType == 0) {

    gfx->drawLine(
      cx - 9,
      cy + 13,
      cx + 9,
      cy + 13,
      GOLD
    );

    gfx->drawLine(
      cx,
      cy + 7,
      cx,
      cy + 17,
      GOLD
    );

  }

  else if (iconType == 1) {

    gfx->drawRect(
      cx - 8,
      cy - 8,
      16,
      16,
      GOLD
    );

    gfx->drawLine(
      cx - 5,
      cy - 13,
      cx - 5,
      cy - 7,
      GOLD
    );

    gfx->drawLine(
      cx + 5,
      cy - 13,
      cx + 5,
      cy - 7,
      GOLD
    );

  }

  else if (iconType == 2) {

    gfx->drawRect(
      cx - 10,
      cy - 7,
      20,
      15,
      GOLD
    );

    gfx->drawLine(
      cx,
      cy - 7,
      cx,
      cy + 8,
      GOLD
    );

  }

  else if (iconType == 3) {

    gfx->drawLine(
      cx - 9,
      cy + 7,
      cx + 9,
      cy - 7,
      GOLD
    );

    gfx->drawLine(
      cx - 9,
      cy - 7,
      cx + 9,
      cy + 7,
      GOLD
    );

  }

  else if (iconType == 4) {

    gfx->drawCircle(
      cx - 6,
      cy,
      5,
      GOLD
    );

    gfx->drawCircle(
      cx + 6,
      cy,
      5,
      GOLD
    );

    gfx->drawLine(
      cx - 12,
      cy + 10,
      cx + 12,
      cy + 10,
      GOLD
    );

  }

  else {

    gfx->drawRect(
      cx - 10,
      cy - 8,
      20,
      16,
      GOLD
    );

    gfx->drawLine(
      cx - 6,
      cy - 13,
      cx + 6,
      cy - 13,
      GOLD
    );
  }


  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    WHITE
  );

  gfx->setCursor(
    x + 48,
    y + 28
  );

  gfx->print(label);


  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    MUTED
  );

  gfx->setCursor(
    x + 48,
    y + 50
  );

  gfx->print(sub);
}


// ============================================================================
// HOME
// ============================================================================

void drawHome() {

  gfx->fillScreen(BG);


  // Header
  gfx->fillRect(
    0,
    0,
    480,
    58,
    NAVY
  );


  gfx->setFont(
    &FreeSansBold12pt7b
  );

  gfx->setTextColor(
    WHITE
  );

  gfx->setCursor(
    20,
    29
  );

  gfx->print(
    "DR. B. R. AMBEDKAR"
  );


  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    20,
    49
  );

  gfx->print(
    "NATIONAL MEMORIAL"
  );


  gfx->fillRect(
    0,
    56,
    480,
    2,
    GOLD
  );


  // FIXED: shorter header text
  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    MUTED
  );

  gfx->setCursor(
    350,
    33
  );

  gfx->print(
    "EXPLORE"
  );


  const int w = 220;
  const int h = 60;


  homeCard(
    18,
    72,
    w,
    h,
    "LIFE",
    "A journey of purpose",
    0
  );


  homeCard(
    242,
    72,
    w,
    h,
    "TIMELINE",
    "1891 — 1956",
    1
  );


  // FIXED: shorter subtitle
  homeCard(
    18,
    140,
    w,
    h,
    "CONSTITUTION",
    "JUSTICE• EQUALITY",
    2
  );


  // FIXED: shorter subtitle
  homeCard(
    242,
    140,
    w,
    h,
    "IDEAS",
    "educate organise",
    2
  );


  homeCard(
    18,
    208,
    w,
    h,
    "LEGACY",
    "Ideas that endure",
    4
  );


  // FIXED: shorter subtitle
  homeCard(
    242,
    208,
    w,
    h,
    "VISIT",
    "26 alipur rd delhi",
    4
  );


  gfx->fillRoundRect(
    18,
    280,
    444,
    25,
    7,
    CARD
  );


  centerText(
    "SELECT A SECTION TO BEGIN",
    240,
    298,
    MUTED,
    &FreeSans9pt7b
  );
}


// ============================================================================
// SECTION HEADER
// ============================================================================

void drawSectionHeader(
  const char* title
) {

  gfx->fillRect(
    0,
    0,
    480,
    55,
    NAVY
  );


  gfx->fillTriangle(
    17,
    27,
    31,
    17,
    31,
    37,
    GOLD
  );


  gfx->setFont(
    &FreeSans9pt7b
  );

  gfx->setTextColor(
    MUTED
  );

  gfx->setCursor(
    41,
    34
  );

  gfx->print(
    "HOME"
  );


  gfx->fillRect(
    0,
    53,
    480,
    2,
    GOLD
  );


  centerText(
    title,
    270,
    35,
    WHITE,
    &FreeSansBold12pt7b
  );
}


// ============================================================================
// DOTS
// ============================================================================

void drawDots(
  int count,
  int selected
) {

  int spacing = 18;

  int start =
    240 -
    ((count - 1) * spacing / 2);


  for (
    int i = 0;
    i < count;
    i++
  ) {

    gfx->fillCircle(
      start + i * spacing,
      303,
      i == selected ? 4 : 3,
      i == selected ? GOLD : MUTED
    );
  }
}


// ============================================================================
// ARROWS
// ============================================================================

void drawArrows(
  bool left,
  bool right
) {

  if (left) {

    gfx->fillTriangle(
      12,
      154,
      28,
      144,
      28,
      164,
      GOLD
    );
  }


  if (right) {

    gfx->fillTriangle(
      468,
      154,
      452,
      144,
      452,
      164,
      GOLD
    );
  }
}


// ============================================================================
// STANDARD STORY PAGE
// ============================================================================

void drawStoryPage(
  const char* section,
  const Page& p,
  int count,
  int index
) {

  gfx->fillScreen(BG);

  drawSectionHeader(section);


  // Main panel
  gfx->fillRoundRect(
    15,
    67,
    450,
    220,
    14,
    CARD
  );

  gfx->drawRoundRect(
    15,
    67,
    450,
    220,
    14,
    BORDER
  );


  // Image
  gfx->fillRoundRect(
    24,
    78,
    p.iw + 6,
    p.ih + 6,
    10,
    DARK
  );

  gfx->drawRoundRect(
    24,
    78,
    p.iw + 6,
    p.ih + 6,
    10,
    GOLD
  );


  if (
    !drawRGB565(
      p.image,
      27,
      81,
      p.iw,
      p.ih
    )
  ) {

    gfx->fillRoundRect(
      27,
      81,
      p.iw,
      p.ih,
      7,
      DARK
    );

    centerText(
      "IMAGE",
      27 + p.iw / 2,
      81 + p.ih / 2 + 5,
      MUTED,
      &FreeSansBold9pt7b
    );
  }


  // Text column
  const int tx = 225;


  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    tx,
    94
  );

  gfx->print(
    p.eyebrow
  );


  // ------------------------------------------------------------
  // TITLE
  // ------------------------------------------------------------

  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    WHITE
  );


  if (
    textWidth(
      p.title,
      &FreeSansBold9pt7b
    ) > 230
  ) {

    drawWrappedText(
      p.title,
      tx,
      122,
      230,
      20,
      WHITE,
      &FreeSansBold9pt7b,
      2
    );

  } else {

    gfx->setCursor(
      tx,
      125
    );

    gfx->print(
      p.title
    );
  }


  // ------------------------------------------------------------
  // DATE
  // ------------------------------------------------------------

  if (
    strlen(p.date) > 0
  ) {

    gfx->fillRoundRect(
      tx,
      136,
      210,
      25,
      6,
      NAVY
    );

    gfx->setFont(
      &FreeSans9pt7b
    );

    gfx->setTextColor(
      GOLD
    );

    gfx->setCursor(
      tx + 9,
      154
    );

    gfx->print(
      p.date
    );
  }


  // ------------------------------------------------------------
  // BODY
  // ------------------------------------------------------------

  drawWrappedText(
    p.body,
    tx,
    183,
    225,
    20,
    MUTED,
    &FreeSans9pt7b,
    2
  );


  // Tag
  gfx->fillRoundRect(
    tx,
    248,
    115,
    22,
    6,
    NAVY
  );

  centerText(
    p.tag,
    tx + 57,
    264,
    GOLD,
    &FreeSansBold9pt7b
  );


  drawArrows(
    index > 0,
    index < count - 1
  );

  drawDots(
    count,
    index
  );
}


// ============================================================================
// VISIT PAGE
//
// Dedicated layout.
// The 220px image gets its own space.
// Text is kept SHORT.
// Nothing is allowed to sit on the image.
// ============================================================================

void drawVisitPage(
  const Page& p,
  int count,
  int index
) {

  gfx->fillScreen(BG);

  drawSectionHeader(
    "VISIT"
  );


  // Main panel
  gfx->fillRoundRect(
    15,
    67,
    450,
    220,
    14,
    CARD
  );

  gfx->drawRoundRect(
    15,
    67,
    450,
    220,
    14,
    BORDER
  );


  // ------------------------------------------------------------
  // IMAGE
  // ------------------------------------------------------------

  const int imageX = 24;
  const int imageY = 78;

  const int imageW = 220;
  const int imageH = 150;


  gfx->fillRoundRect(
    imageX,
    imageY,
    imageW + 6,
    imageH + 6,
    10,
    DARK
  );

  gfx->drawRoundRect(
    imageX,
    imageY,
    imageW + 6,
    imageH + 6,
    10,
    GOLD
  );


  if (
    !drawRGB565(
      p.image,
      imageX + 3,
      imageY + 3,
      imageW,
      imageH
    )
  ) {

    gfx->fillRoundRect(
      imageX + 3,
      imageY + 3,
      imageW,
      imageH,
      7,
      DARK
    );

    centerText(
      "IMAGE",
      imageX + imageW / 2 + 3,
      imageY + imageH / 2 + 5,
      MUTED,
      &FreeSansBold9pt7b
    );
  }


  // ------------------------------------------------------------
  // TEXT
  //
  // Starts AFTER the image.
  // Available width = approximately 200px.
  // ------------------------------------------------------------

  const int tx = 258;
  const int textW = 190;


  // Eyebrow
  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    GOLD
  );

  gfx->setCursor(
    tx,
    95
  );

  gfx->print(
    p.eyebrow
  );


  // Title
  gfx->setFont(
    &FreeSansBold9pt7b
  );

  gfx->setTextColor(
    WHITE
  );


  drawWrappedText(
    p.title,
    tx,
    122,
    textW,
    20,
    WHITE,
    &FreeSansBold9pt7b,
    2
  );


  // Date / location
  if (
    strlen(p.date) > 0
  ) {

    gfx->fillRoundRect(
      tx,
      166,
      180,
      24,
      6,
      NAVY
    );

    gfx->setFont(
      &FreeSans9pt7b
    );

    gfx->setTextColor(
      GOLD
    );

    gfx->setCursor(
      tx + 8,
      183
    );

    gfx->print(
      p.date
    );
  }


  // Short body
  drawWrappedText(
    p.body,
    tx,
    211,
    textW,
    20,
    MUTED,
    &FreeSans9pt7b,
    2
  );


  // Small tag
  gfx->fillRoundRect(
    tx,
    250,
    125,
    22,
    6,
    NAVY
  );

  centerText(
    p.tag,
    tx + 62,
    266,
    GOLD,
    &FreeSansBold9pt7b
  );


  drawArrows(
    index > 0,
    index < count - 1
  );

  drawDots(
    count,
    index
  );
}


// ============================================================================
// RENDER
// ============================================================================

void renderScreen() {

  switch (currentScreen) {

    case HOME:

      drawHome();

      break;


    case LIFE:

      drawStoryPage(
        "LIFE",
        lifePages[pageIndex],
        LIFE_COUNT,
        pageIndex
      );

      break;


    case TIMELINE:

      drawStoryPage(
        "TIMELINE",
        timelinePages[pageIndex],
        TIMELINE_COUNT,
        pageIndex
      );

      break;


    case CONSTITUTION:

      drawStoryPage(
        "CONSTITUTION",
        constitutionPages[pageIndex],
        CONSTITUTION_COUNT,
        pageIndex
      );

      break;


    case IDEAS:

      drawStoryPage(
        "IDEAS",
        ideasPages[pageIndex],
        IDEAS_COUNT,
        pageIndex
      );

      break;


    case LEGACY:

      drawStoryPage(
        "LEGACY",
        legacyPages[pageIndex],
        LEGACY_COUNT,
        pageIndex
      );

      break;


    case VISIT:

      // IMPORTANT:
      // VISIT no longer uses the generic layout.
      drawVisitPage(
        visitPages[pageIndex],
        VISIT_COUNT,
        pageIndex
      );

      break;
  }
}


// ============================================================================
// PAGE COUNT
// ============================================================================

int pageCountForScreen() {

  switch (currentScreen) {

    case LIFE:
      return LIFE_COUNT;

    case TIMELINE:
      return TIMELINE_COUNT;

    case CONSTITUTION:
      return CONSTITUTION_COUNT;

    case IDEAS:
      return IDEAS_COUNT;

    case LEGACY:
      return LEGACY_COUNT;

    case VISIT:
      return VISIT_COUNT;

    default:
      return 1;
  }
}


// ============================================================================
// TOUCH
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


void moveTouch(
  int x,
  int y
) {

  lastTouchX = x;
  lastTouchY = y;
}


void goHome() {

  currentScreen = HOME;

  pageIndex = 0;

  renderScreen();
}


void openSection(
  int i
) {

  currentScreen =
    (Screen)(i + 1);

  pageIndex = 0;

  renderScreen();
}


void endTouch() {

  if (!fingerDown)
    return;


  int dx =
    lastTouchX -
    touchStartX;

  int dy =
    lastTouchY -
    touchStartY;


  // ------------------------------------------------------------
  // SWIPE
  // ------------------------------------------------------------

  if (
    currentScreen != HOME &&
    abs(dx) >= 45 &&
    abs(dx) > abs(dy)
  ) {

    int count =
      pageCountForScreen();


    if (
      dx < 0 &&
      pageIndex < count - 1
    ) {

      pageIndex++;

      renderScreen();

    }

    else if (
      dx > 0 &&
      pageIndex > 0
    ) {

      pageIndex--;

      renderScreen();
    }
  }


  // ------------------------------------------------------------
  // TAP
  // ------------------------------------------------------------

  else if (
    abs(dx) < 18 &&
    abs(dy) < 18
  ) {


    // ----------------------------------------------------------
    // HOME BUTTON
    // ----------------------------------------------------------

    if (
      currentScreen != HOME &&
      touchStartY < 58
    ) {

      goHome();
    }


    // ----------------------------------------------------------
    // HOME GRID
    // ----------------------------------------------------------

    else if (
      currentScreen == HOME
    ) {

      const int w = 220;
      const int h = 60;

      const int xs[2] = {
        18,
        242
      };

      const int ys[3] = {
        72,
        140,
        208
      };


      for (
        int row = 0;
        row < 3;
        row++
      ) {

        for (
          int col = 0;
          col < 2;
          col++
        ) {

          int x =
            xs[col];

          int y =
            ys[row];


          if (
            touchStartX >= x &&
            touchStartX < x + w &&
            touchStartY >= y &&
            touchStartY < y + h
          ) {

            int index =
              row * 2 + col;

            openSection(index);

            goto touch_done;
          }
        }
      }
    }


    // ----------------------------------------------------------
    // STORY ARROWS
    // ----------------------------------------------------------

    else {

      int count =
        pageCountForScreen();


      if (
        touchStartX < 60 &&
        pageIndex > 0
      ) {

        pageIndex--;

        renderScreen();
      }


      else if (
        touchStartX > 420 &&
        pageIndex < count - 1
      ) {

        pageIndex++;

        renderScreen();
      }
    }
  }


touch_done:

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

  Serial.begin(115200);

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


  gfx->setRotation(1);


  // ------------------------------------------------------------
  // COLORS
  // ------------------------------------------------------------

  BG =
    gfx->color565(
      8,
      15,
      33
    );

  NAVY =
    gfx->color565(
      20,
      33,
      66
    );

  CARD =
    gfx->color565(
      29,
      44,
      82
    );

  GOLD =
    gfx->color565(
      214,
      178,
      96
    );

  WHITE =
    gfx->color565(
      244,
      246,
      250
    );

  MUTED =
    gfx->color565(
      171,
      181,
      204
    );

  DARK =
    gfx->color565(
      5,
      10,
      24
    );

  BORDER =
    gfx->color565(
      76,
      91,
      121
    );


  // ------------------------------------------------------------
  // SERIAL
  // ------------------------------------------------------------

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    " AMBEDKAR MEMORIAL FULL UI"
  );

  Serial.println(
    " 480 x 320 LANDSCAPE"
  );

  Serial.println(
    " SdFat + STATIC SPLASH + FIXED CARDS"
  );

  Serial.println(
    "========================================"
  );


  // ------------------------------------------------------------
  // SD
  // ------------------------------------------------------------

  Serial.println(
    "Initializing SD..."
  );


  if (
    sd.begin(
      SdSpiConfig(
        SD_CS,
        SHARED_SPI,
        SD_SCK_MHZ(4)
      )
    )
  ) {

    sdOK = true;

    Serial.println(
      "SD: OK"
    );

  }

  else {

    sdOK = false;

    Serial.println(
      "SD: FAILED"
    );

    sd.printSdError(
      &Serial
    );
  }


  // ------------------------------------------------------------
  // START
  // ------------------------------------------------------------

  drawSplash();


  currentScreen = HOME;

  pageIndex = 0;

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

    if (!fingerDown)
      beginTouch(x, y);

    else
      moveTouch(x, y);

  }

  else {

    if (fingerDown)
      endTouch();
  }
}