const int PIN_BUTTON = 14;
const int PIN_LED_GREEN = 25;

int lastLevel = HIGH;
int count = 0;
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED_GREEN, OUTPUT);
  Serial.println("\nUji button: tekan button di GPIO14");
  Serial.printf("Level awal: %s (harus HIGH)\n", digitalRead(PIN_BUTTON) == HIGH ? "HIGH" : "LOW");
}

void loop() {
  const int level = digitalRead(PIN_BUTTON);
  if (level != lastLevel) {
    delay(30);  
    if (digitalRead(PIN_BUTTON) == level) {
      lastLevel = level;
      if (level == LOW) {
        count++;
        ledOn = !ledOn;
        digitalWrite(PIN_LED_GREEN, ledOn ? HIGH : LOW);
        Serial.printf("DITEKAN #%d\n", count);
      } else {
        Serial.println("dilepas");
      }
    }
  }
}
