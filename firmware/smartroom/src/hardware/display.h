#pragma once
#include <stdint.h>
#include "alarm_logic.h"
#include "sensors.h"

struct DisplayView {
  const SensorReadings *s;
  AlarmState alarm;
  float threshold;
  bool wifiOk;
  const char *link;  // jalur data: "MQTT", "HTTP"
  int otaPct;              // 0-100 saat download firmware
  const char *otaVersion;  // versi yang sedang didownload
  bool newFirmware;        // firmware baru terpasang lewat OTA
  const char *firmware;    // FIRMWARE_VERSION
};

void displayBegin();
void displayTick(uint32_t now, const DisplayView &v);  
