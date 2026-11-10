#pragma once
#include <stdint.h>

// Pin 
constexpr uint8_t PIN_DHT       = 4;
constexpr uint8_t DHT_TYPE      = 11;  // 11 = DHT11, 22 = DHT22
constexpr uint8_t PIN_LDR       = 34;  
constexpr uint8_t PIN_PIR       = 27;
constexpr uint8_t PIN_LED_GREEN = 25;
constexpr uint8_t PIN_LED_RED   = 26;
constexpr uint8_t PIN_BUZZER    = 33; 
constexpr uint8_t PIN_BUTTON    = 14; 
constexpr uint8_t PIN_SDA       = 21;
constexpr uint8_t PIN_SCL       = 22;
constexpr uint8_t OLED_ADDR     = 0x3C;
constexpr bool BUZZER_ACTIVE_HIGH = false;  // false buzzer aktif-LOW

// Waktu 
constexpr uint32_t DHT_INTERVAL_MS       = 5000;
constexpr uint32_t LDR_INTERVAL_MS       = 1000;
constexpr uint32_t PIR_WARMUP_MS         = 30000;  
constexpr uint32_t TELEMETRY_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_INTERVAL_MS   = 500;
constexpr uint32_t BUTTON_DEBOUNCE_MS    = 30;
constexpr uint32_t HTTP_AFTER_MS         = 5000;  // MQTT disconnect 5 detik, kirim lewat HTTP

// Alarm
constexpr float DEFAULT_THRESHOLD_C = 35.0f;
constexpr float THRESHOLD_MIN_C     = 0.0f;
constexpr float THRESHOLD_MAX_C     = 60.0f;
constexpr float ALARM_HYSTERESIS_C  = 0.5f;  // alarm mati bila suhu < ambang - 0,5
constexpr uint8_t DHT_FAULT_STREAK  = 3;     // gagal beruntun sebelum dianggap rusak

// MQTT
constexpr const char *TOPIC_ROOT = "smartroom";
constexpr int MQTT_KEEPALIVE_S   = 30;   
constexpr int MQTT_BUFFER_SIZE   = 4096; 
constexpr int MQTT_COMMAND_MAX   = 256;  // perintah OTA memuat sha256 dan versi

constexpr const char *FIRMWARE_VERSION = "smartroom-2.6";  // naikkan setiap membuat firmware baru

constexpr uint32_t OTA_VERIFY_MS = 5UL * 60 * 1000;
// Lama layar "firmware baru terpasang" tampil setelah OTA berhasil.
constexpr uint32_t NEW_FW_SHOW_MS = 10000;
