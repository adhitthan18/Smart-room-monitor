#include "sensors.h"
#include <Arduino.h>
#include <DHT.h>
#include "../../config.h"
#include "../interval.h"

static DHT dht(PIN_DHT, DHT_TYPE);
static SensorReadings gReadings;
static Interval gDhtTimer(DHT_INTERVAL_MS);
static Interval gLdrTimer(LDR_INTERVAL_MS);

void sensorsBegin() {
  dht.begin();
  analogReadResolution(12);         
  pinMode(PIN_PIR, INPUT_PULLDOWN); 
}

static void readDht() {
  const float t = dht.readTemperature();
  const float h = dht.readHumidity();

  gReadings.dhtOk = !isnan(t) && !isnan(h) && t >= -40 && t <= 80 && h >= 0 && h <= 100;
  if (gReadings.dhtOk) {
    gReadings.temperature = t;
    gReadings.humidity = h;
    gReadings.dhtFailStreak = 0;
    return;
  }
  if (gReadings.dhtFailStreak < 255) gReadings.dhtFailStreak++;
  if (gReadings.dhtFailStreak == 1) Serial.println("[DHT] Tidak terbaca");
}

static void readLdr() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(PIN_LDR); 
  gReadings.lightPct = sum / 8 * 100 / 4095;
}

static void readPir(uint32_t now) {
  gReadings.pirReady = now >= PIR_WARMUP_MS;
  const bool motion = gReadings.pirReady && digitalRead(PIN_PIR) == HIGH;
  if (motion != gReadings.motion) Serial.println(motion ? "[PIR] gerakan terdeteksi" : "[PIR] tidak ada gerakan");
  gReadings.motion = motion;
}

bool sensorsTick(uint32_t now) {
  if (gLdrTimer.due(now)) readLdr();
  readPir(now);
  if (!gDhtTimer.due(now)) return false;
  readDht();
  return true;
}

const SensorReadings &sensors() { return gReadings; }
