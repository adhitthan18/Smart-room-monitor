#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_mac.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "secrets.h"
#endif

#include "config.h"
#include "src/data/payload.h"
#include "src/hardware/alarm.h"
#include "src/hardware/display.h"
#include "src/hardware/sensors.h"
#include "src/interval.h"
#include "src/network/http_backup.h"
#include "src/network/mqtt.h"
#include "src/network/ota_http.h"
#include "src/network/wifi_link.h"

static char gMac[18];        // AA:BB:CC:DD:EE:FF device ID di topic
static char gClientId[32];   
static char gHostname[24];
static char gMqttUser[40];
static Interval gTelemetryTimer(TELEMETRY_INTERVAL_MS);
static Preferences gPrefs;   // NVS

static void makeIds() {
  uint8_t m[6];
  esp_read_mac(m, ESP_MAC_WIFI_STA);
  snprintf(gMac, sizeof gMac, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  snprintf(gClientId, sizeof gClientId, "smartroom-%02X%02X%02X%02X%02X%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  snprintf(gHostname, sizeof gHostname, "smartroom-%02x%02x%02x", m[3], m[4], m[5]);
  const bool macLogin = !MQTT_USERNAME[0] && MQTT_PASSWORD[0];
  strlcpy(gMqttUser, macLogin ? gMac : MQTT_USERNAME, sizeof gMqttUser);
}

static float loadThreshold() {
  const float t = gPrefs.getFloat("thr", DEFAULT_THRESHOLD_C);
  return isfinite(t) && t >= THRESHOLD_MIN_C && t <= THRESHOLD_MAX_C ? t : DEFAULT_THRESHOLD_C;
}

static bool httpMode(uint32_t now) {
  return !mqttConnected() && httpEnabled() && wifiConnected() && mqttDownForMs(now) >= HTTP_AFTER_MS;
}

static void sendTelemetry(uint32_t now) {
  const SensorReadings &s = sensors();
  Telemetry t;
  t.deviceId = gMac;
  t.uptimeS = now / 1000;
  t.dhtOk = s.dhtOk;
  t.temperature = s.temperature;
  t.humidity = s.humidity;
  t.lightPct = s.lightPct;
  t.motion = s.motion;
  t.alarm = alarmStateName(alarmState());
  t.threshold = alarmThreshold();
  t.rssi = WiFi.RSSI();
  t.firmware = FIRMWARE_VERSION;
  char otaState[80];
  otaHttpState(otaState, sizeof otaState);
  t.ota = otaState;

  char json[384];
  if (!buildTelemetry(t, json, sizeof json)) return;
  if (mqttPublishTelemetry(json)) {
    Serial.printf("[MQTT] %s\n", json);
  } else if (httpMode(now) && httpSend(json)) { 
    Serial.printf("[HTTP] %s\n", json);
  } else {
    Serial.printf("[DATA] belum terkirim: %s\n", json);
  }
}

static void handleCommands() {
  char cmd[MQTT_COMMAND_MAX];
  while (mqttTakeCommand(cmd, sizeof cmd)) {
    float threshold;
    OtaCommand ota;
    if (parseOtaCommand(cmd, strlen(cmd), ota)) {
      if (!otaHttpStart(ota.sha256, ota.size, ota.version)) Serial.println("[CMD] OTA ditolak: sedang download");
      gTelemetryTimer.trigger();  
      continue;
    }
    if (!parseThreshold(cmd, strlen(cmd), threshold)) {
      continue;
    }
    alarmSetThreshold(threshold);
    if (gPrefs.getFloat("thr", NAN) != threshold) gPrefs.putFloat("thr", threshold); 
    Serial.printf("[CMD] ambang baru %.1f C\n", threshold);
    gTelemetryTimer.trigger(); 
  }
}

void setup() {
  Serial.begin(115200);
  makeIds();
  gPrefs.begin("smartroom", false);
  const float threshold = loadThreshold();

  alarmBegin(threshold);
  sensorsBegin();
  displayBegin();
  Serial.printf("\n=== %s ===\nDevice ID : %s\nAmbang    : %.1f C\n", FIRMWARE_VERSION, gMac, threshold);

  wifiBegin({WIFI_SSID, WIFI_PASSWORD, gHostname});
  mqttBegin({MQTT_URL, gMqttUser, MQTT_PASSWORD, gMac, gClientId});
  httpBegin(HTTP_URL, gMac, MQTT_USERNAME, MQTT_PASSWORD);
  otaHttpBegin({HTTP_URL, gMac, MQTT_USERNAME, MQTT_PASSWORD});
}

void loop() {
  const uint32_t now = millis();

  // Tombol, LED, buzzer
  if (alarmTick(now)) {
    if (alarmAcknowledge()) {
      Serial.println("[button] alarm dimatikan sampai suhu turun lalu");
      gTelemetryTimer.trigger();
    } else {
      Serial.println("[button] tidak ada alarm aktif");
    }
  }

  // Sensor, alarm
  if (sensorsTick(now)) {
    const SensorReadings &s = sensors();
    alarmOnTemperature(s.temperature, s.dhtOk);
    alarmSetSensorFault(s.dhtFailStreak >= DHT_FAULT_STREAK);
  }

  // Internet dan server
  wifiTick(now);
  mqttTick(wifiConnected());
  otaHttpVerify(mqttConnected() || httpSucceeded(), now); 
  handleCommands();

  // Telemetri tiap 5 detik: MQTT, atau HTTP bila MQTT disconnect
  if (gTelemetryTimer.due(now)) sendTelemetry(now);

  // OLED
  const char *link = mqttConnected() ? "MQTT" : httpMode(now) ? "HTTP" : "----";
  displayTick(now, {&sensors(), alarmState(), alarmThreshold(), wifiConnected(), link, otaHttpProgress(),
            otaHttpTarget(), otaHttpJustInstalled(now), FIRMWARE_VERSION});
}
