#pragma once
#include <stdint.h>

struct WifiConfig {
  const char *ssid;
  const char *password;
  const char *hostname;
};

void wifiBegin(const WifiConfig &cfg);
void wifiTick(uint32_t now);
bool wifiConnected();
