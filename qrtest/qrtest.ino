#include <Arduino.h>
#include "esp_camera.h"
#include "quirc.h"

// ============================================================
// CAMERA PINS — ESP32-CAM + GC2145
// ============================================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============================================================
// CAMERA / QUIRC RESOLUTION
// ============================================================

#define CAM_W 640
#define CAM_H 480

#define QR_W 320
#define QR_H 240

#define GRAY_SIZE (QR_W * QR_H)

// ============================================================
// GLOBAL MEMORY
// ============================================================

struct quirc *qr = nullptr;

uint8_t *gray_buffer = nullptr;

// IMPORTANT:
// These are large (~4 KB and ~9 KB).
// Do NOT declare them inside loop().
struct quirc_code qr_code;
struct quirc_data qr_data;

// ============================================================
// RGB565 -> GRAYSCALE
// STANDARD RGB565 BYTE ORDER
// ============================================================

static inline uint8_t rgb565_to_gray(uint16_t p)
{
  uint8_t r = ((p >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((p >> 5)  & 0x3F) * 255 / 63;
  uint8_t b = (p & 0x1F) * 255 / 31;

  return (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
}

// ============================================================
// CREATE 320x240 GRAYSCALE IMAGE
//
// Camera is 640x480.
// We take every second pixel.
// ============================================================

void make_gray(camera_fb_t *fb)
{
  const uint8_t *src = fb->buf;

  for (int y = 0; y < QR_H; y++)
  {
    int sy = y * 2;

    for (int x = 0; x < QR_W; x++)
    {
      int sx = x * 2;

      int idx = (sy * CAM_W + sx) * 2;

      // STANDARD RGB565
      uint16_t p =
        ((uint16_t)src[idx] << 8) |
        src[idx + 1];

      gray_buffer[y * QR_W + x] =
        rgb565_to_gray(p);
    }
  }
}

// ============================================================
// RUN QUIRC
// ============================================================

int run_quirc()
{
  int w;
  int h;

  uint8_t *image = quirc_begin(qr, &w, &h);

  if (!image)
  {
    Serial.println("[ERROR] quirc_begin() returned NULL");
    return -1;
  }

  if (w != QR_W || h != QR_H)
  {
    Serial.printf(
      "[ERROR] Quirc image size is %dx%d\n",
      w, h
    );

    quirc_end(qr);
    return -1;
  }

  memcpy(image, gray_buffer, GRAY_SIZE);

  quirc_end(qr);

  return quirc_count(qr);
}

// ============================================================
// DECODE FIRST CANDIDATE
// ============================================================

void decode_candidates(int count)
{
  if (count <= 0)
  {
    return;
  }

  Serial.println();
  Serial.println("******** CANDIDATE FOUND ********");

  for (int i = 0; i < count; i++)
  {
    Serial.printf(
      "[QR] Candidate %d / %d\n",
      i + 1,
      count
    );

    // Extract candidate
    quirc_extract(qr, i, &qr_code);

    // Decode candidate
    int err = quirc_decode(&qr_code, &qr_data);

    if (err == QUIRC_SUCCESS)
    {
      Serial.println("[QR] *** DECODE SUCCESS ***");

      Serial.print("[QR] Payload: ");

      Serial.write(
        qr_data.payload,
        qr_data.payload_len
      );

      Serial.println();

      Serial.printf(
        "[QR] Payload length: %d\n",
        qr_data.payload_len
      );

      Serial.printf(
        "[QR] Version: %d\n",
        qr_data.version
      );

      Serial.printf(
        "[QR] ECC level: %d\n",
        qr_data.ecc_level
      );

      Serial.printf(
        "[QR] Mask: %d\n",
        qr_data.mask
      );
    }
    else
    {
      Serial.printf(
        "[QR] Decode FAILED: %d\n",
        err
      );

      // Useful diagnostic:
      // Quirc error codes distinguish different
      // decoding failures.
      switch (err)
      {
        case QUIRC_ERROR_INVALID_GRID_SIZE:
          Serial.println(
            "[QR] Reason: invalid grid size"
          );
          break;

        case QUIRC_ERROR_INVALID_VERSION:
          Serial.println(
            "[QR] Reason: invalid version"
          );
          break;

        case QUIRC_ERROR_FORMAT_ECC:
          Serial.println(
            "[QR] Reason: format ECC failure"
          );
          break;

        case QUIRC_ERROR_DATA_ECC:
          Serial.println(
            "[QR] Reason: data ECC failure"
          );
          break;

        case QUIRC_ERROR_UNKNOWN_DATA_TYPE:
          Serial.println(
            "[QR] Reason: unknown data type"
          );
          break;

        case QUIRC_ERROR_DATA_OVERFLOW:
          Serial.println(
            "[QR] Reason: data overflow"
          );
          break;

        case QUIRC_ERROR_DATA_UNDERFLOW:
          Serial.println(
            "[QR] Reason: data underflow"
          );
          break;

        default:
          Serial.println(
            "[QR] Reason: other Quirc error"
          );
          break;
      }
    }

    Serial.println();
  }

  Serial.println(
    "*********************************"
  );
}

// ============================================================
// CAMERA INITIALIZATION
// ============================================================

bool init_camera()
{
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_VGA;

  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK)
  {
    Serial.printf(
      "[ERROR] Camera init failed: 0x%x\n",
      err
    );

    return false;
  }

  sensor_t *s = esp_camera_sensor_get();

  if (s)
  {
    Serial.printf(
      "[OK] Sensor PID: 0x%04X\n",
      s->id.PID
    );
  }

  return true;
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("========================================");
  Serial.println("   QUIRC CANDIDATE VALIDATION TEST");
  Serial.println("========================================");

  Serial.printf(
    "PSRAM: %u bytes\n",
    ESP.getPsramSize()
  );

  // Camera
  if (!init_camera())
  {
    Serial.println(
      "[FATAL] Camera initialization failed"
    );

    while (true)
      delay(1000);
  }

  Serial.println("[OK] Camera initialized");

  // Grayscale buffer
  gray_buffer =
    (uint8_t *)ps_malloc(GRAY_SIZE);

  if (!gray_buffer)
  {
    Serial.println(
      "[FATAL] Gray buffer allocation failed"
    );

    while (true)
      delay(1000);
  }

  Serial.printf(
    "[OK] Gray buffer: %u bytes\n",
    GRAY_SIZE
  );

  // Quirc
  qr = quirc_new();

  if (!qr)
  {
    Serial.println(
      "[FATAL] quirc_new() failed"
    );

    while (true)
      delay(1000);
  }

  if (quirc_resize(qr, QR_W, QR_H) < 0)
  {
    Serial.println(
      "[FATAL] quirc_resize() failed"
    );

    while (true)
      delay(1000);
  }

  Serial.printf(
    "[OK] Quirc resized to %dx%d\n",
    QR_W,
    QR_H
  );

  Serial.println();
  Serial.println("========================================");
  Serial.println("READY");
  Serial.println("========================================");
  Serial.println();
  Serial.println(
    "Place a known QR code in view."
  );
  Serial.println(
    "Keep it stationary and well lit."
  );
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  static uint32_t frame_no = 0;

  frame_no++;

  Serial.println();
  Serial.printf(
    "========== FRAME %lu ==========\n",
    frame_no
  );

  // ----------------------------------------------------------
  // Capture
  // ----------------------------------------------------------

  uint32_t capture_start = millis();

  camera_fb_t *fb =
    esp_camera_fb_get();

  uint32_t capture_ms =
    millis() - capture_start;

  if (!fb)
  {
    Serial.println(
      "[ERROR] Frame capture failed"
    );

    delay(500);
    return;
  }

  Serial.printf(
    "Capture: %lu ms\n",
    capture_ms
  );

  Serial.printf(
    "Frame: %ux%u | len=%u | format=%d\n",
    fb->width,
    fb->height,
    fb->len,
    fb->format
  );

  // ----------------------------------------------------------
  // Validate framebuffer
  // ----------------------------------------------------------

  if (fb->width != CAM_W ||
      fb->height != CAM_H ||
      fb->len != CAM_W * CAM_H * 2)
  {
    Serial.println(
      "[WARNING] Unexpected framebuffer"
    );
  }

  // ----------------------------------------------------------
  // Grayscale
  // ----------------------------------------------------------

  uint32_t gray_start = millis();

  make_gray(fb);

  uint32_t gray_ms =
    millis() - gray_start;

  Serial.printf(
    "Grayscale: %lu ms\n",
    gray_ms
  );

  // ----------------------------------------------------------
  // Quirc
  // ----------------------------------------------------------

  uint32_t qr_start = millis();

  int candidates =
    run_quirc();

  uint32_t qr_ms =
    millis() - qr_start;

  Serial.printf(
    "Quirc: %lu ms\n",
    qr_ms
  );

  Serial.printf(
    "Candidates: %d\n",
    candidates
  );

  // ----------------------------------------------------------
  // Decode if candidate exists
  // ----------------------------------------------------------

  if (candidates > 0)
  {
    decode_candidates(candidates);
  }

  // ----------------------------------------------------------
  // Return framebuffer
  // ----------------------------------------------------------

  esp_camera_fb_return(fb);

  delay(300);
}