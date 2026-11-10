#include "http_backup.h"
#include <Arduino.h>
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

struct Body {
  char json[384];
};

static QueueHandle_t gQueue = nullptr;  
static volatile bool gSucceeded = false; 
static volatile bool gPaused = false;
static volatile bool gClosed = true;
static const char *gUsername = "";
static const char *gPassword = "";
static char gUrl[160];

static void post(esp_http_client_handle_t client, const char *json) {
  esp_http_client_set_post_field(client, json, strlen(json));
  const esp_err_t err = esp_http_client_perform(client);
  if (err != ESP_OK) {
    Serial.printf("[HTTP] gagal: %s\n", esp_err_to_name(err));
    esp_http_client_close(client); 
    return;
  }
  const int code = esp_http_client_get_status_code(client);
  if (code >= 200 && code < 300) {
    gSucceeded = true;
    Serial.println("[HTTP] terkirim");
  } else if (code == 401) {
    Serial.println("[HTTP] ditolak server (HTTP 401): cek MQTT_USERNAME dan MQTT_PASSWORD");
  } else if (code == 403) {
    Serial.println("[HTTP] ditolak server (HTTP 403): akun tidak punya akses");
  } else {
    Serial.printf("[HTTP] ditolak server (HTTP %d)\n", code);
  }
}

static void httpTask(void *) {
  esp_http_client_config_t cfg = {};
  cfg.url = gUrl;
  cfg.method = HTTP_METHOD_POST;
  cfg.timeout_ms = 10000;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;  
  cfg.keep_alive_enable = true;                  
  cfg.username = gUsername;
  cfg.password = gPassword;
  cfg.auth_type = HTTP_AUTH_TYPE_BASIC; 
  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  esp_http_client_set_header(client, "Content-Type", "application/json");

  Body body;
  for (;;) {
    const bool got = xQueueReceive(gQueue, &body, pdMS_TO_TICKS(200)) == pdTRUE;
    if (gPaused) {
      if (!gClosed) {
        esp_http_client_close(client);  
        gClosed = true;
      }
      continue; 
    }
    if (got) {
      gClosed = false;
      post(client, body.json);
    }
  }
}

void httpPause(bool paused, uint32_t waitMs) {
  gPaused = paused;
  if (!paused || !gQueue) return;
  for (uint32_t t = 0; !gClosed && t < waitMs; t += 50) vTaskDelay(pdMS_TO_TICKS(50));
}

void httpBegin(const char *baseUrl, const char *deviceId, const char *username, const char *password) {
  if (!baseUrl[0] || !username[0] || !password[0]) {
    Serial.println("[HTTP] backup mati (HTTP_URL, MQTT_USERNAME, atau MQTT_PASSWORD kosong)");
    return;
  }
  const int n = snprintf(gUrl, sizeof gUrl, "%s/api/devices/%s/telemetry", baseUrl, deviceId);
  if (n <= 0 || n >= (int)sizeof gUrl) {
    Serial.println("[HTTP] backup mati (HTTP_URL melebihi batas)");
    return;
  }
  gUsername = username;
  gPassword = password;
  gQueue = xQueueCreate(1, sizeof(Body));
  xTaskCreate(httpTask, "http_backup", 8192, nullptr, 1, nullptr);
}

bool httpEnabled() { return gQueue != nullptr; }
bool httpSucceeded() { return gSucceeded; }

bool httpSend(const char *json) {
  if (!gQueue || gPaused || strlen(json) >= sizeof(Body::json)) return false;
  Body body;
  strlcpy(body.json, json, sizeof body.json);
  xQueueOverwrite(gQueue, &body);
  return true;
}
