#include "wifi_link.h"

#include <Arduino.h>
#include <WiFi.h>

void wifiBegin(const WifiConfig &cfg) {
  WiFi.persistent(false);  
  WiFi.setHostname(cfg.hostname);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.ssid, cfg.password);
  Serial.printf("[WiFi] menyambung ke %s\n", cfg.ssid);
}

void wifiTick(uint32_t now) {
  static bool wasUp = false;
  static uint32_t downSince = 0;
  const bool up = WiFi.status() == WL_CONNECTED;
  if (up != wasUp) {
    wasUp = up;
    downSince = now;
    if (up) {
      Serial.printf("[WiFi] terhubung, IP %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[WiFi] terputus, menyambung ulang");
    }
  }
  if (!up && now - downSince > 30000) {
    WiFi.reconnect();
    downSince = now;
  }
}

bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }
