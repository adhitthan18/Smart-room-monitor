const int PIN_LDR = 34;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("\nUji LDR di GPIO34");
}

void loop() {
  const int raw = analogRead(PIN_LDR);
  const int mv = analogReadMilliVolts(PIN_LDR);
  Serial.printf("raw:%d mV:%d persen:%d\n", raw, mv, raw * 100 / 4095);
  delay(500);
}
