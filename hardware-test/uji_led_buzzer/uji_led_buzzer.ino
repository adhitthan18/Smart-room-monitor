const int PIN_LED_GREEN = 25;
const int PIN_LED_RED = 26;
const int PIN_BUZZER = 33;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.println("\nUji LED dan buzzer");
}

void loop() {
  Serial.println("LED hijau");
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_GREEN, LOW);

  Serial.println("LED merah");
  digitalWrite(PIN_LED_RED, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_RED, LOW);

  Serial.println("Buzzer 3x");
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(200);
    digitalWrite(PIN_BUZZER, LOW);
    delay(200);
  }
  delay(1000);
}
