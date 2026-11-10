#pragma once
#include <stddef.h>
#include <stdint.h>

struct Telemetry {
  const char *deviceId = "";
  uint32_t uptimeS = 0;
  bool dhtOk = false;
  float temperature = 0;
  float humidity = 0;
  uint8_t lightPct = 0;
  bool motion = false;
  const char *alarm = "normal";
  float threshold = 0;
  int rssi = 0;
  const char *firmware = ""; 
  const char *ota = "";      
};

// {"ota":{"sha256":"<64 hex>","size":N,"version":"..."}}
struct OtaCommand {
  char sha256[65];  
  uint32_t size;
  char version[32];
};

size_t buildTelemetry(const Telemetry &t, char *out, size_t outSize);

bool parseThreshold(const char *data, size_t len, float &threshold);

bool parseOtaCommand(const char *data, size_t len, OtaCommand &out);
