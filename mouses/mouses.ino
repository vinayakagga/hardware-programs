#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <HijelHID_BLEMouse.h>
#include <math.h>

HijelBLEMouse mouse("Motion Controller", "Vinayak");
MPU9250_asukiaaa mpu;

// ---------------- SETTINGS ----------------

const float sensitivity = 0.04;   // Cursor speed
const float deadzone = 4.0;       // Ignore small tilts
const float filter = 0.09;        // Smoothing (0.05-0.2)

// Button Pins
#define LEFT_BUTTON   4
#define RIGHT_BUTTON  5

// ------------------------------------------

float filteredPitch = 0;
float filteredRoll = 0;

bool lastLeftState = HIGH;
bool lastRightState = HIGH;

void setup() {

  Serial.begin(115200);

  Wire.begin(21,22);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);

  mpu.setWire(&Wire);
  mpu.beginAccel();

  mouse.begin();

  Serial.println("Motion Controller Started");
}

void loop() {

  if(!mouse.isPaired()){
    delay(100);
    return;
  }

  // ---------------- IMU ----------------

  mpu.accelUpdate();

  float ax = mpu.accelX();
  float ay = mpu.accelY();
  float az = mpu.accelZ();

  float pitch = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
  float roll  = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / PI;

  // Low-pass filter
  filteredPitch += (pitch - filteredPitch) * filter;
  filteredRoll  += (roll  - filteredRoll ) * filter;

  float p = filteredPitch;
  float r = filteredRoll;

  int dx = 0;
  int dy = 0;

  // Reverse X
  if(abs(r) > deadzone){
    float value = r - (r > 0 ? deadzone : -deadzone);
    dx = -(value * abs(value) * sensitivity);
  }

  if(abs(p) > deadzone){
    float value = p - (p > 0 ? deadzone : -deadzone);
    dy = -(value * abs(value) * sensitivity);
  }

  // Limit maximum speed
  dx = constrain(dx,-18,18);
  dy = constrain(dy,-18,18);

  mouse.move(dx,dy);

  // ---------------- LEFT CLICK ----------------

  bool leftState = digitalRead(LEFT_BUTTON);

  if(lastLeftState == HIGH && leftState == LOW){

    mouse.click(MouseButton::Left);
    Serial.println("Left Click");

    delay(40);
  }

  lastLeftState = leftState;

  // ---------------- RIGHT CLICK ----------------

  bool rightState = digitalRead(RIGHT_BUTTON);

  if(lastRightState == HIGH && rightState == LOW){

    mouse.click(MouseButton::Right);
    Serial.println("Right Click");

    delay(40);
  }

  lastRightState = rightState;

  delay(8);
}