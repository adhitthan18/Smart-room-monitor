#include "payload.h"
#include <ArduinoJson.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "../../config.h"

static double roundTo(float v, double scale) { return round(v * scale) / scale; }

size_t buildTelemetry(const Telemetry &t, char *out, size_t outSize) {
  StaticJsonDocument<384> doc;
  doc["device_id"] = t.deviceId;
  doc["uptime_s"] = t.uptimeS;
  if (t.dhtOk) {
    doc["temperature"] = roundTo(t.temperature, 10);
    doc["humidity"] = roundTo(t.humidity, 10);
  } else {
    doc["temperature"] = nullptr; 
    doc["humidity"] = nullptr;
  }
  doc["light_pct"] = t.lightPct;
  doc["motion"] = t.motion;
  doc["alarm"] = t.alarm;
  doc["threshold"] = roundTo(t.threshold, 100);
  doc["rssi"] = t.rssi;
  if (t.firmware[0]) doc["fw"] = t.firmware;
  if (t.ota[0]) doc["ota"] = t.ota;
  if (doc.overflowed() || measureJson(doc) >= outSize) return 0;
  return serializeJson(doc, out, outSize);
}

bool parseThreshold(const char *data, size_t len, float &threshold) {
  StaticJsonDocument<128> doc;
  if (len == 0 || deserializeJson(doc, data, len)) return false;
  JsonVariantConst root = doc.as<JsonVariantConst>();
  JsonVariantConst value = root.is<JsonObjectConst>() ? root["threshold"] : root;
  if (!value.is<float>()) return false; 
  const float t = value.as<float>();
  if (!isfinite(t) || t < THRESHOLD_MIN_C || t > THRESHOLD_MAX_C) return false;
  threshold = t;
  return true;
}

bool parseOtaCommand(const char *data, size_t len, OtaCommand &out) {
  StaticJsonDocument<384> doc;
  if (len == 0 || deserializeJson(doc, data, len)) return false;
  JsonObjectConst ota = doc["ota"];
  if (ota.isNull()) return false;
  const char *sha = ota["sha256"] | "";
  const char *version = ota["version"] | "";
  if (strlen(sha) != 64 || !ota["size"].is<uint32_t>()) return false;
  const uint32_t size = ota["size"];
  const size_t versionLen = strlen(version);
  if (size == 0 || size > 4u * 1024 * 1024 || versionLen == 0 || versionLen >= sizeof out.version) return false;
  for (size_t i = 0; i < 64; i++) {
    const char c = sha[i];
    if (!isxdigit((unsigned char)c)) return false;
    out.sha256[i] = (char)tolower((unsigned char)c);
  }
  out.sha256[64] = '\0';
  for (size_t i = 0; i < versionLen; i++) {
    if (!isprint((unsigned char)version[i])) return false;
  }
  memcpy(out.version, version, versionLen + 1);
  out.size = size;
  return true;
}
