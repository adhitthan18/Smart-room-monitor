#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "../../config.h"
#include "../interval.h"

//  y=0   SmartRoom     WiFi MQTT
//  y=12  29.5°C        Lembab 65.2%
//  y=31  Cahaya: 72%
//  y=39  Gerak : tidak
//  y=47  Ambang: 30.0°C
//  y=55  ▓▓▓▓▓ Normal ▓▓▓▓▓      

static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool gOk = false;
static Interval gRefresh(DISPLAY_INTERVAL_MS);

void displayBegin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setTimeOut(50); 
  Wire.beginTransmission(OLED_ADDR);
  gOk = Wire.endTransmission() == 0 && oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, false, false);
  if (!gOk) {
    Serial.println("[OLED] tidak terbaca");
    return;
  }
  oled.cp437(true);  
  oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay();
  oled.display();
}

static void printDegreeC() {
  oled.write(248);
  oled.print('C');
}

static void drawStatusBar(uint32_t now, const DisplayView &v) {
  const char *text = "Normal";
  bool inverse = false;
  if (v.alarm == AlarmState::Active) {
    text = "!! SUHU > AMBANG !!";
    inverse = (now / 500) % 2 == 0;  
  } else if (v.alarm == AlarmState::Acknowledged) {
    text = "Alarm dimatikan";
    inverse = true;
  } else if (!v.s->dhtOk) {
    text = "Sensor DHT gagal";
  }
  if (inverse) {
    oled.fillRect(0, 55, 128, 9, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
  }
  oled.setCursor((128 - (int)strlen(text) * 6) / 2, 56);
  oled.print(text);
  oled.setTextColor(SSD1306_WHITE);
}

static void drawMainSlide(uint32_t now, const DisplayView &v) {

  const SensorReadings &s = *v.s;
  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.print("SmartRoom");
  oled.setCursor(128 - 9 * 6, 0);
  oled.printf("%s %s", v.wifiOk ? "WiFi" : "----", v.link);
  oled.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(0, 12);
  if (s.dhtOk) {
    oled.printf("%.1f", s.temperature);
  } else {
    oled.print("--.-");
  }
  oled.setTextSize(1);
  printDegreeC();
  oled.setCursor(86, 12);
  oled.print("Lembab");
  oled.setCursor(86, 21);
  if (s.dhtOk) {
    oled.printf("%.1f%%", s.humidity);
  } else {
    oled.print("--");
  }

  oled.setCursor(0, 31);
  oled.printf("Cahaya: %u%%", s.lightPct);
  oled.setCursor(0, 39);
  oled.print(!s.pirReady ? "Gerak : Loading" : s.motion ? "Gerak : ADA" : "Gerak : TIDAK");
  oled.setCursor(0, 47);
  oled.printf("Ambang: %.1f", v.threshold);
  printDegreeC();

  drawStatusBar(now, v);
}

// Slide 2a firmware
static void drawOtaSlide(const DisplayView &v) {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Update firmware");
  oled.drawFastHLine(0, 9, 128, SSD1306_WHITE);
  oled.setCursor(0, 16);
  oled.print("Versi baru:");
  oled.setCursor(0, 26);
  oled.print(v.otaVersion);
  oled.drawRect(4, 38, 120, 10, SSD1306_WHITE);
  oled.fillRect(6, 40, 116 * v.otaPct / 100, 6, SSD1306_WHITE);
  oled.setCursor(0, 54);
  if (v.otaPct >= 100) {
    oled.print("Selesai, restart...");
  } else {
    oled.printf("Download %d%%", v.otaPct);
  }
}

static void drawNewFirmwareSlide(const DisplayView &v) {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Firmware baru");
  oled.drawFastHLine(0, 9, 128, SSD1306_WHITE);
  oled.setCursor(0, 20);
  oled.print("Terpasang:");
  oled.setCursor(0, 32);
  oled.print(v.firmware);
  oled.setCursor(0, 54);
  oled.print("OTA berhasil");
}

void displayTick(uint32_t now, const DisplayView &v) {
  if (!gOk || !gRefresh.due(now)) return;
  oled.clearDisplay();
  if (v.otaPct >= 0) {
    drawOtaSlide(v);
  } else if (v.newFirmware) {
    drawNewFirmwareSlide(v);
  } else {
    drawMainSlide(now, v);
  }
  oled.display();
}

