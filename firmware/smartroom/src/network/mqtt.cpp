#include "mqtt.h"
#include <Arduino.h>
#include "../../config.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"

struct CommandMsg {
  char text[MQTT_COMMAND_MAX];
};

static MqttConfig gCfg;
static char gTopicTelemetry[64];
static char gTopicCommand[64];
static char gTopicStatus[64];
static esp_mqtt_client_handle_t gClient = nullptr;
static QueueHandle_t gCommands = nullptr;
static volatile bool gMqttUp = false;
static volatile uint32_t gMqttDownSince = 0;

static bool topicIs(esp_mqtt_event_handle_t e, const char *topic) {
  return e->topic_len == (int)strlen(topic) && strncmp(e->topic, topic, e->topic_len) == 0;
}

static void onMqttEvent(void *, esp_event_base_t, int32_t id, void *data) {
  const auto e = static_cast<esp_mqtt_event_handle_t>(data);
  switch (id) {
    case MQTT_EVENT_CONNECTED:
      gMqttUp = true;
      Serial.println("[MQTT] terhubung");
      esp_mqtt_client_subscribe(gClient, gTopicCommand, 1);
      esp_mqtt_client_publish(gClient, gTopicStatus, "online", 0, 1, 1); 
      break;

    case MQTT_EVENT_DISCONNECTED:
      if (gMqttUp) {
        gMqttDownSince = millis();
        Serial.println("[MQTT] terputus, menyambung ulang");
      }
      gMqttUp = false;
      break;

    case MQTT_EVENT_ERROR:
      if (e->error_handle && e->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
        Serial.printf("[MQTT] ditolak broker (kode %d): cek username, API key, dan izin topik\n",
                      e->error_handle->connect_return_code);
      }
      break;

    case MQTT_EVENT_DATA:
      if (topicIs(e, gTopicCommand) && e->current_data_offset == 0 && e->data_len == e->total_data_len &&
          e->data_len < MQTT_COMMAND_MAX) {
        CommandMsg msg{};
        memcpy(msg.text, e->data, e->data_len);
        xQueueSend(gCommands, &msg, 0);
      }
      break;
    default:
      break;
  }
}

static void mqttStart() {
  esp_mqtt_client_config_t cfg = {};
  cfg.broker.address.uri = gCfg.url;
  cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;  // TLS untuk wss:// dan mqtts://
  cfg.credentials.client_id = gCfg.clientId;
  if (gCfg.username[0]) cfg.credentials.username = gCfg.username;
  if (gCfg.password[0]) cfg.credentials.authentication.password = gCfg.password;
  cfg.session.keepalive = MQTT_KEEPALIVE_S;
  cfg.session.last_will.topic = gTopicStatus; 
  cfg.session.last_will.msg = "offline";
  cfg.session.last_will.qos = 1;
  cfg.session.last_will.retain = 1;
  cfg.buffer.size = MQTT_BUFFER_SIZE;
  cfg.outbox.limit = 8 * 1024; 

  gClient = esp_mqtt_client_init(&cfg);
  if (!gClient) {
    Serial.println("[MQTT] gagal, cek MQTT_URL");
    return;
  }
  esp_mqtt_client_register_event(gClient, MQTT_EVENT_ANY, onMqttEvent, nullptr);
  esp_mqtt_client_start(gClient);
}

void mqttBegin(const MqttConfig &cfg) {
  gCfg = cfg;
  snprintf(gTopicTelemetry, sizeof gTopicTelemetry, "%s/%s/telemetry", TOPIC_ROOT, cfg.deviceId);
  snprintf(gTopicCommand, sizeof gTopicCommand, "%s/%s/command", TOPIC_ROOT, cfg.deviceId);
  snprintf(gTopicStatus, sizeof gTopicStatus, "%s/%s/status", TOPIC_ROOT, cfg.deviceId);
  gCommands = xQueueCreate(4, sizeof(CommandMsg));
}

void mqttTick(bool wifiUp) {
  if (wifiUp && !gClient) mqttStart();
}

bool mqttConnected() { return gMqttUp; }
uint32_t mqttDownForMs(uint32_t now) {
  const uint32_t since = gMqttDownSince; 
  return gMqttUp || (int32_t)(now - since) < 0 ? 0 : now - since;
}

bool mqttPublishTelemetry(const char *json) {
  return gMqttUp && esp_mqtt_client_enqueue(gClient, gTopicTelemetry, json, 0, 0, 0, true) >= 0;
}

bool mqttTakeCommand(char *buf, size_t size) {
  CommandMsg msg;
  if (!gCommands || xQueueReceive(gCommands, &msg, 0) != pdTRUE) return false;
  strlcpy(buf, msg.text, size);
  return true;
}
