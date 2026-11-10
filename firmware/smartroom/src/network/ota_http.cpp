#include "ota_http.h"
#include <Arduino.h>
#include "../../config.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "http_backup.h"
#include "mbedtls/sha256.h"

static volatile int gPct = -1;
static volatile bool gInstalled = false;
static volatile uint32_t gInstalledAt = 0;

extern "C" bool verifyRollbackLater() { return true; }

static bool gEnabled = false;
static char gUrl[160];
static const char *gUsername = "";
static const char *gPassword = "";

static char gSha[65];
static uint32_t gSize = 0;
static char gVersion[32];
static volatile bool gBusy = false;

static portMUX_TYPE gLock = portMUX_INITIALIZER_UNLOCKED;
static char gState[80] = "";

static void setState(const char *fmt, ...) {
  char buf[sizeof gState];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  portENTER_CRITICAL(&gLock);
  memcpy(gState, buf, sizeof gState);
  portEXIT_CRITICAL(&gLock);
  Serial.printf("[OTA] %s\n", buf);
}

void otaHttpState(char *out, uint32_t outSize) {
  portENTER_CRITICAL(&gLock);
  strlcpy(out, gState, outSize);
  portEXIT_CRITICAL(&gLock);
}

static char gErrBuf[64];

static const char *writeFirmware(esp_http_client_handle_t client) {
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  esp_ota_handle_t ota;
  if (!slot || esp_ota_begin(slot, gSize, &ota) != ESP_OK) return "OTA tidak cukup, pakai Minimal SPIFFS";

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  static uint8_t buf[4096];
  const char *err = nullptr;
  uint32_t got = 0;
  while (got < gSize && !err) {
    const int n = esp_http_client_read(client, (char *)buf, sizeof buf);
    if (n <= 0) {
      err = "download terputus";
    } else if (esp_ota_write(ota, buf, n) != ESP_OK) {
      err = "gagal flash";
    } else {
      mbedtls_sha256_update(&sha, buf, n);
      got += n;
      const int pct = (int)((uint64_t)got * 100 / gSize);
      if (pct / 10 != gPct / 10) setState("download %d%%", pct);  // tiap 10%
      gPct = pct;
    }
  }

  uint8_t digest[32];
  mbedtls_sha256_finish(&sha, digest);
  mbedtls_sha256_free(&sha);
  char hex[65];
  for (int i = 0; i < 32; i++) snprintf(hex + 2 * i, 3, "%02x", digest[i]);
  if (!err && strcmp(hex, gSha) != 0) err = "sha256 salah";

  if (err) {
    esp_ota_abort(ota);
    return err;
  }
  if (esp_ota_end(ota) != ESP_OK) return "bukan firmware ESP32";
  if (esp_ota_set_boot_partition(slot) != ESP_OK) return "slot baru tidak bisa dipilih";
  return nullptr;
}

static const char *download() {
  httpPause(true, 2000);

  esp_http_client_config_t cfg = {};
  cfg.url = gUrl;
  cfg.timeout_ms = 15000;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.username = gUsername;
  cfg.password = gPassword;
  cfg.auth_type = HTTP_AUTH_TYPE_BASIC;
  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  if (!client) return "HTTP tidak bisa dibuat";

  const char *err = nullptr;
  esp_err_t openErr = ESP_FAIL;
  for (int attempt = 1; attempt <= 3; attempt++) {
    openErr = esp_http_client_open(client, 0);
    if (openErr == ESP_OK) break;
    Serial.printf("[OTA] connect gagal (percobaan %d): %s, heap %u, blok terbesar %u\n", attempt,
                  esp_err_to_name(openErr), (unsigned)esp_get_free_heap_size(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    esp_http_client_close(client);
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
  if (openErr != ESP_OK) {
    snprintf(gErrBuf, sizeof gErrBuf, "server tidak connect (%s)", esp_err_to_name(openErr));
    err = gErrBuf;
  } else {
    const int64_t length = esp_http_client_fetch_headers(client);
    const int status = esp_http_client_get_status_code(client);
    if (status == 401) {
      err = "server menolak (401): cek akun perangkat";
    } else if (status == 404) {
      err = "firmware tidak ada di server (404)";
    } else if (status != 200) {
      snprintf(gErrBuf, sizeof gErrBuf, "server menjawab HTTP %d", status);
      err = gErrBuf;
    } else if (length != (int64_t)gSize) {
      err = "size file tidak cocok";
    } else {
      err = writeFirmware(client);
    }
  }
  esp_http_client_cleanup(client);
  return err;
}

static void otaTask(void *) {
  const char *err = download();
  if (err) {
    setState("gagal: %s", err);
    httpPause(false, 0);
    gPct = -1;
    gBusy = false;
    vTaskDelete(nullptr);
    return;
  }
  setState("selesai %s, restart", gVersion);
  vTaskDelay(pdMS_TO_TICKS(6000));
  esp_restart();
}

void otaHttpBegin(const OtaHttpConfig &cfg) {
  if (!cfg.baseUrl[0] || !cfg.username[0] || !cfg.password[0]) {
    Serial.println("[OTA] lewat http mati (HTTP_URL, MQTT_USERNAME, atau MQTT_PASSWORD kosong)");
    return;
  }
  const int n = snprintf(gUrl, sizeof gUrl, "%s/api/devices/%s/firmware", cfg.baseUrl, cfg.deviceId);
  if (n <= 0 || n >= (int)sizeof gUrl) return;
  gUsername = cfg.username;
  gPassword = cfg.password;
  gEnabled = true;

  if (esp_ota_get_last_invalid_partition() != nullptr) setState("firmware baru gagal, kembali ke %s", FIRMWARE_VERSION);
}

bool otaHttpStart(const char *sha256, uint32_t size, const char *version) {
  if (!gEnabled || gBusy) return false;
  strlcpy(gSha, sha256, sizeof gSha);
  strlcpy(gVersion, version, sizeof gVersion);
  gSize = size;
  gPct = 0;
  gBusy = true;
  setState("mulai download %s", gVersion);
  if (xTaskCreate(otaTask, "ota_http", 8192, nullptr, 1, nullptr) != pdPASS) {
    setState("gagal: task gagal");
    gPct = -1;
    gBusy = false;
    return false;
  }
  return true;
}

void otaHttpVerify(bool serverReachable, uint32_t now) {
  static bool done = false;
  if (done) return;
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) != ESP_OK ||
      state != ESP_OTA_IMG_PENDING_VERIFY) {
    done = true;  
    return;
  }
  if (serverReachable) {
    esp_ota_mark_app_valid_cancel_rollback();
    setState("terpasang %s", FIRMWARE_VERSION);
    gInstalled = true;
    gInstalledAt = now;
    done = true;
  } else if (now > OTA_VERIFY_MS) {
    Serial.println("[OTA] firmware baru gagal terhubung ke server, kembali ke versi lama");
    esp_ota_mark_app_invalid_rollback_and_reboot();
  }
}
int otaHttpProgress() { return gPct; }
const char *otaHttpTarget() { return gVersion; }
bool otaHttpJustInstalled(uint32_t now) { return gInstalled && now - gInstalledAt < NEW_FW_SHOW_MS; }
