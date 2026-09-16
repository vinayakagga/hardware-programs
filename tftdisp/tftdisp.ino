#include <Arduino_GFX_Library.h>
#include <TouchScreen.h>

// ============================================================
// DISPLAY
// ============================================================

Arduino_DataBus *bus = new Arduino_UNOPAR8;

Arduino_GFX *gfx = new Arduino_ILI9486(
  bus,
  A4,
  0,
  false
);


// ============================================================
// TOUCH
// ============================================================

#define YP A3
#define XM A2
#define YM 9
#define XP 8

TouchScreen ts(
  XP,
  YP,
  XM,
  YM,
  300
);


// ============================================================
// COLORS
// ============================================================

#define BLACK       0x0000
#define OFFWHITE    0xEFEBE2
#define WHITE       0xFFFF
#define GOLD        0xC6A66A
#define BLUE        0x2F6FED
#define BLUE_LIGHT  0x5C8FF0
#define DARK        0x111216
#define PANEL       0x17191D
#define IMAGE_MID   0x30343B
#define MUTED       0x626771
#define LINE        0x383C44


// ============================================================
// TOUCH CALIBRATION
// ============================================================

const float TOUCH_AX = -0.58384084;
const float TOUCH_BX = 552.93869;

const float TOUCH_AY = -0.42020932;
const float TOUCH_BY = 374.01786;


// ============================================================
// TOUCH STRUCTURE
// ============================================================

struct TouchPoint
{
  int x;
  int y;
  int z;
  bool pressed;
};


// ============================================================
// READ TOUCH
// ============================================================

TouchPoint readTouch()
{
  TouchPoint t;

  TSPoint p = ts.getPoint();

  // Restore LCD pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);


  if (p.z < 200 || p.z > 1000)
  {
    t.pressed = false;
    return t;
  }


  t.x =
    (int)(
      TOUCH_AX * p.y +
      TOUCH_BX +
      0.5
    );


  t.y =
    (int)(
      TOUCH_AY * p.x +
      TOUCH_BY +
      0.5
    );


  t.x = constrain(t.x, 0, 479);
  t.y = constrain(t.y, 0, 319);

  t.z = p.z;
  t.pressed = true;

  return t;
}


// ============================================================
// TEXT HELPER
// ============================================================

void text(
  const char *s,
  int x,
  int y,
  int size,
  uint16_t color
)
{
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(x, y);
  gfx->print(s);
}


// ============================================================
// LEFT IMAGE PANEL
// ============================================================

void drawMemorialVisual()
{
  // Main image panel

  gfx->fillRect(
    0,
    0,
    195,
    320,
    PANEL
  );


  // ----------------------------------------------------------
  // Atmospheric background
  // ----------------------------------------------------------

  for (int y = 0; y < 320; y++)
  {
    int v =
      22 +
      (y * 13 / 320);

    uint16_t c =
      gfx->color565(
        v,
        v,
        v + 2
      );

    gfx->drawFastHLine(
      0,
      y,
      195,
      c
    );
  }


  // ----------------------------------------------------------
  // Architectural glow
  // ----------------------------------------------------------

  gfx->fillCircle(
    98,
    125,
    83,
    gfx->color565(
      39,
      41,
      45
    )
  );


  // ----------------------------------------------------------
  // Memorial building
  // ----------------------------------------------------------

  gfx->fillTriangle(
    42,
    122,
    98,
    78,
    153,
    122,
    IMAGE_MID
  );


  gfx->drawLine(
    42,
    122,
    98,
    78,
    GOLD
  );

  gfx->drawLine(
    98,
    78,
    153,
    122,
    GOLD
  );


  gfx->fillRect(
    53,
    122,
    90,
    13,
    IMAGE_MID
  );


  gfx->fillRect(
    62,
    135,
    72,
    45,
    IMAGE_MID
  );


  // ----------------------------------------------------------
  // Columns
  // ----------------------------------------------------------

  for (
    int x = 68;
    x <= 120;
    x += 16
  )
  {
    gfx->fillRect(
      x,
      140,
      5,
      39,
      0x7BEF
    );
  }


  // ----------------------------------------------------------
  // Central architectural spine
  // ----------------------------------------------------------

  gfx->fillRect(
    94,
    54,
    8,
    68,
    GOLD
  );

  gfx->fillCircle(
    98,
    47,
    11,
    GOLD
  );

  gfx->fillCircle(
    98,
    38,
    6,
    GOLD
  );


  // ----------------------------------------------------------
  // Open-book shape
  // ----------------------------------------------------------

  gfx->fillTriangle(
    18,
    240,
    98,
    196,
    98,
    270,
    IMAGE_MID
  );


  gfx->fillTriangle(
    98,
    196,
    175,
    240,
    98,
    270,
    IMAGE_MID
  );


  gfx->drawLine(
    18,
    240,
    98,
    196,
    GOLD
  );

  gfx->drawLine(
    98,
    196,
    175,
    240,
    GOLD
  );


  gfx->drawLine(
    18,
    240,
    98,
    279,
    GOLD
  );

  gfx->drawLine(
    98,
    279,
    175,
    240,
    GOLD
  );


  // ----------------------------------------------------------
  // Image typography
  // ----------------------------------------------------------

  text(
    "MAHAPARINIRVAN",
    17,
    18,
    1,
    GOLD
  );

  text(
    "BHOOMI",
    17,
    34,
    2,
    OFFWHITE
  );


  gfx->drawFastHLine(
    17,
    59,
    68,
    GOLD
  );


  text(
    "DELHI",
    17,
    292,
    1,
    MUTED
  );
}


// ============================================================
// FIXED RIGHT SIDE
// ============================================================

void drawFixedUI()
{
  // Right background

  gfx->fillRect(
    195,
    0,
    285,
    320,
    BLACK
  );


  // ----------------------------------------------------------
  // Memorial name
  // ----------------------------------------------------------

  text(
    "DR. AMBEDKAR NATIONAL",
    218,
    20,
    1,
    MUTED
  );


  text(
    "MEMORIAL",
    218,
    37,
    2,
    OFFWHITE
  );


  // ----------------------------------------------------------
  // Hero
  // ----------------------------------------------------------

  text(
    "DR. B. R.",
    218,
    72,
    3,
    OFFWHITE
  );


  text(
    "AMBEDKAR",
    218,
    106,
    3,
    OFFWHITE
  );


  // ----------------------------------------------------------
  // Divider
  // ----------------------------------------------------------

  gfx->drawFastHLine(
    218,
    143,
    75,
    GOLD
  );


  // ----------------------------------------------------------
  // Memorial identity
  // ----------------------------------------------------------

  text(
    "MAHAPARINIRVAN",
    218,
    158,
    2,
    GOLD
  );


  text(
    "BHOOMI",
    218,
    181,
    2,
    GOLD
  );


  // ----------------------------------------------------------
  // Location
  // ----------------------------------------------------------

  text(
    "26 ALIPUR ROAD",
    218,
    213,
    2,
    OFFWHITE
  );


  text(
    "DELHI",
    218,
    235,
    2,
    OFFWHITE
  );


  // ----------------------------------------------------------
  // Progress indicators
  // ----------------------------------------------------------

  gfx->fillCircle(
    218,
    298,
    3,
    GOLD
  );

  gfx->fillCircle(
    229,
    298,
    3,
    LINE
  );

  gfx->fillCircle(
    240,
    298,
    3,
    LINE
  );


  text(
    "EXPLORE",
    255,
    291,
    1,
    MUTED
  );
}


// ============================================================
// ANIMATED MESSAGE
// ============================================================

const char *messages[] =
{
  "A LIFE.",
  "A CONSTITUTION.",
  "A LEGACY."
};

const int MESSAGE_COUNT = 3;

int messageIndex = 0;

unsigned long lastMessageTime = 0;

const unsigned long MESSAGE_INTERVAL = 2300;


// ============================================================
// MESSAGE REGION
// ============================================================

void clearMessage()
{
  gfx->fillRect(
    310,
    155,
    165,
    40,
    BLACK
  );
}


void drawMessage()
{
  clearMessage();


  text(
    messages[messageIndex],
    310,
    164,
    2,
    OFFWHITE
  );
}


// ============================================================
// ANIMATE MESSAGE
// ============================================================

void animateMessage()
{
  int next =
    (messageIndex + 1)
    % MESSAGE_COUNT;


  // Slide current out

  for (
    int offset = 0;
    offset <= 35;
    offset += 5
  )
  {
    clearMessage();


    text(
      messages[messageIndex],
      310 - offset,
      164,
      2,
      OFFWHITE
    );


    delay(18);
  }


  // Slide next in

  for (
    int offset = 35;
    offset >= 0;
    offset -= 5
  )
  {
    clearMessage();


    text(
      messages[next],
      310 - offset,
      164,
      2,
      OFFWHITE
    );


    delay(18);
  }


  messageIndex = next;

  lastMessageTime = millis();
}


// ============================================================
// BEGIN BUTTON
// ============================================================

const int BUTTON_X = 305;
const int BUTTON_Y = 252;
const int BUTTON_W = 153;
const int BUTTON_H = 42;


void drawButton(
  bool pressed
)
{
  uint16_t c =
    pressed
      ? BLUE_LIGHT
      : BLUE;


  gfx->fillRoundRect(
    BUTTON_X,
    BUTTON_Y,
    BUTTON_W,
    BUTTON_H,
    10,
    c
  );


  text(
    "BEGIN JOURNEY",
    322,
    266,
    1,
    WHITE
  );


  // Arrow

  gfx->drawLine(
    432,
    273,
    447,
    273,
    WHITE
  );

  gfx->drawLine(
    441,
    267,
    448,
    273,
    WHITE
  );

  gfx->drawLine(
    441,
    279,
    448,
    273,
    WHITE
  );
}


// ============================================================
// BUTTON HITBOX
//
// Larger than visible button.
// ============================================================

bool buttonHit(
  int x,
  int y
)
{
  return (
    x >= 292 &&
    x <= 470 &&
    y >= 242 &&
    y <= 305
  );
}


// ============================================================
// ARRIVAL SCREEN
// ============================================================

void drawArrival()
{
  gfx->fillScreen(
    BLACK
  );


  drawMemorialVisual();

  drawFixedUI();

  drawMessage();

  drawButton(
    false
  );
}


// ============================================================
// NEXT SCREEN
// ============================================================

void drawMemorialScreen()
{
  gfx->fillScreen(
    BLACK
  );


  text(
    "THE MEMORIAL",
    35,
    48,
    4,
    OFFWHITE
  );


  gfx->drawFastHLine(
    37,
    102,
    90,
    GOLD
  );


  text(
    "A PLACE OF MEMORY",
    37,
    125,
    2,
    MUTED
  );


  text(
    "THE OPEN BOOK",
    37,
    165,
    2,
    GOLD
  );


  text(
    "THE CONSTITUTION",
    37,
    195,
    2,
    OFFWHITE
  );


  gfx->fillRoundRect(
    37,
    245,
    140,
    42,
    9,
    BLUE
  );


  text(
    "EXPLORE →",
    61,
    258,
    2,
    WHITE
  );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(
    115200
  );


  delay(500);


  if (!gfx->begin())
  {
    Serial.println(
      "TFT initialization FAILED!"
    );

    while (1);
  }


  gfx->setRotation(
    1
  );


  Serial.println();
  Serial.println(
    "=========================================="
  );

  Serial.println(
    " AMBEDKAR MEMORIAL POC"
  );

  Serial.println(
    " FIGMA → TFT IMPLEMENTATION"
  );

  Serial.println(
    "=========================================="
  );


  Serial.print(
    "Display: "
  );

  Serial.print(
    gfx->width()
  );

  Serial.print(
    " x "
  );

  Serial.println(
    gfx->height()
  );


  drawArrival();


  lastMessageTime =
    millis();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ----------------------------------------------------------
  // Secondary message animation
  // ----------------------------------------------------------

  if (
    millis() -
    lastMessageTime >=
    MESSAGE_INTERVAL
  )
  {
    animateMessage();
  }


  // ----------------------------------------------------------
  // Touch
  // ----------------------------------------------------------

  TouchPoint t =
    readTouch();


  if (t.pressed)
  {
    if (
      buttonHit(
        t.x,
        t.y
      )
    )
    {
      Serial.print(
        "BEGIN JOURNEY @ "
      );

      Serial.print(
        t.x
      );

      Serial.print(
        ","
      );

      Serial.println(
        t.y
      );


      // Press feedback

      drawButton(
        true
      );


      delay(120);


      drawMemorialScreen();


      // Wait for release

      while (true)
      {
        TouchPoint r =
          readTouch();

        if (!r.pressed)
          break;

        delay(20);
      }


      // Stay on Screen 2
      while (true)
      {
        delay(100);
      }
    }
  }


  delay(15);
}