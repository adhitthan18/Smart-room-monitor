#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

const int PIN_SDA = 21;
const int PIN_SCL = 22;
const uint8_t OLED_ADDR = 0x3C;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledOk = false;

void scanI2c() {
  Serial.println("Address I2C ...");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Address 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) Serial.println("  tidak terbaca, cek SDA/SCL");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  Serial.println("\nUji OLED SSD1306");
  scanI2c();

  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!oledOk) {
    Serial.println("OLED gagal");
    return;
  }
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.println("SmartRoom");
  oled.setTextSize(1);
  oled.println("Uji OLED 128x64");
  oled.drawRect(0, 40, 128, 24, SSD1306_WHITE);
  oled.display();
}

void loop() {
  if (!oledOk) return;
  oled.fillRect(2, 42, 124, 20, SSD1306_BLACK);
  oled.setTextSize(2);
  oled.setCursor(8, 44);
  oled.printf("%lus", millis() / 1000);
  oled.display();
  delay(1000);
}
