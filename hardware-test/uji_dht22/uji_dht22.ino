#include <DHT.h>

const int PIN_DHT = 4;
const uint8_t DHT_TYPE = DHT11;  // atau DHT22
DHT dht(PIN_DHT, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.printf("\nUji DHT%d di GPIO4\n", DHT_TYPE);
}

void loop() {
  delay(2500); 
  const float t = dht.readTemperature();
  const float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.printf("GAGAL membaca DHT%d\n", DHT_TYPE);
    return;
  }
  Serial.printf("Suhu %.1f C | Kelembapan %.1f %%\n", t, h);
}
