int pirPin = 19;

void setup() {
  Serial.begin(115200);
  pinMode(pirPin, INPUT);
}

void loop() {
  int val = digitalRead(pirPin);
  Serial.println(val);
  delay(500);
}