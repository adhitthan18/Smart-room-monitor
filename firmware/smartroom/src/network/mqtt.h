#pragma once
#include <stddef.h>
#include <stdint.h>

// MQTT berjalan di task sendiri, tidak melalui loop()
//   smartroom/<MAC>/telemetry  ->  JSON tiap 5 detik
//   smartroom/<MAC>/command    <-  {"threshold": 28.5}
//   smartroom/<MAC>/status     ->  "online" / "offline" 

struct MqttConfig {
  const char *url;  // mqtt://, mqtts://, ws://, wss://
  const char *username;
  const char *password;
  const char *deviceId;
  const char *clientId;
};

void mqttBegin(const MqttConfig &cfg);  
void mqttTick(bool wifiUp);            

bool mqttConnected();
uint32_t mqttDownForMs(uint32_t now); 

bool mqttPublishTelemetry(const char *json);   
bool mqttTakeCommand(char *buf, size_t size); 
