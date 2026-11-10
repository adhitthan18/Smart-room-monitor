const int PIN_PIR = 27;
const int PIN_LED_RED = 26;

int lastLevel = -1;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  pinMode(PIN_LED_RED, OUTPUT);
  Serial.println("\nUji PIR di GPIO27 - tunggu 30-60 detik");
}

void loop() {
  const int level = digitalRead(PIN_PIR);
  digitalWrite(PIN_LED_RED, level);
  if (level != lastLevel) {
    lastLevel = level;
    Serial.printf("[%lus] %s\n", millis() / 1000, level == HIGH ? "GERAKAN" : "diam");
  }
  delay(50);
}
