#include <BleMouse.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>

BleMouse bleMouse("Vinayak Air Mouse", "ESP32", 100);

MPU9250_asukiaaa mySensor;

float gyroX, gyroY;

void setup() {

  Serial.begin(115200);

  Wire.begin(21,22);          // CHANGE ONLY IF USING DIFFERENT SDA/SCL

  mySensor.setWire(&Wire);

  mySensor.beginGyro();

  bleMouse.begin();

  Serial.println("Air Mouse Started");
}

void loop() {

  if (!bleMouse.isConnected()) {
    delay(100);
    return;
  }

  mySensor.gyroUpdate();

  gyroX = mySensor.gyroX();
  gyroY = mySensor.gyroY();

  // -------- DEAD ZONE --------
  if (abs(gyroX) < 0.05) gyroX = 0;
  if (abs(gyroY) < 0.05) gyroY = 0;

  // -------- SENSITIVITY --------
  // Increase multiplier if cursor is slow
  // Decrease if cursor is too fast

  int moveX = gyroY * 12;
  int moveY = -gyroX * 12;

  bleMouse.move(moveX, moveY);

  delay(10);      // CHANGE IF MOVEMENT IS JITTERY
}