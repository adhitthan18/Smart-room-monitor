# SmartRoom ESP32

Prototype smart room menggunakan esp32 dengan protokol MQTT sebagai main dan HTTP sebagai cadangan jika MQTT disconnect.

## Pemenuhan syarat

| **Level 1: Hardware** | |
| Rangkaian Schematic | [Schematic.png](Schematic.png): daftar pin [firmware/smartroom/config.h](firmware/smartroom/config.h) |
| Design PCB | Top Layer: [PCB-TopLayer.png](PCB-TopLayer.png) Botton Layer: [PCB-BottomLayer.png](PCB-BottomLayer.png) |
| Pengujian Sensor | [hardware-test/](hardware-test/): LED + buzzer, Button, DHT22/DHT11, LDR, PIR, OLED |
| Hasil Prototype | [Prototype-SmartRoom.jpeg](Prototype-SmartRoom.jpeg) |
| Video Tetsing | 
| **Level 2: Firmware** | |
| Baca sensor tanpa `delay()` | `src/interval.h` (`millis()`), `src/hardware/sensors.cpp`: DHT22 tiap 5s, LDR tiap 1s, PIR tiap loop |
| Logika alarm + button | `src/hardware/alarm_logic.h`, `src/hardware/alarm.cpp` (LED, buzzer, button) |
| OLED | `src/hardware/display.cpp`: suhu, kelembapan, cahaya, gerak, ambang, jalur data, status alarm |
| Kode dikelompokkan per bagian (`hardware/`, `network/`, `data/`); `smartroom.ino|
| **Level 3: MQTT** | |
| Wi-Fi reconnect otomatis, kredensial di luar repo | `src/network/wifi_link.cpp`; kredensial di `secrets.h` yang tercantum di `.gitignore` |
| Telemetri JSON tiap 5s | `src/network/mqtt.cpp`: `smartroom/<MAC>/telemetry` Hasil: [MQTT] {"device_id":"28:05:A5:25:59:28","uptime_s":80,"temperature":30.8,"humidity":65,"light_pct":100,"motion":true,"alarm":"normal","threshold":34,"rssi":-44,"fw":"smartroom-2.5"} |
| Perintah ubah ambang | `src/network/mqtt.cpp` menerima `smartroom/<MAC>/command`, mis. `{"threshold": 28.5}` Hasil: [CMD] ambang baru 33.0 C |
| Status + Last Will | `src/network/mqtt.cpp`: `smartroom/<MAC>/status`: `online` / `offline`, retained |
| Broker | `wss://iot.adhiteknologi.com:443/mqtt`; |
| **Tambahan** | |
| Data tetap terkirim saat MQTT disconnect | `src/network/http_backup.cpp`: `POST https://smart.adhiteknologi.com/api/devices/<MAC>/telemetry`, login dengan akun MQTT |

## Struktur

```textfirmware/smartroom/      main file
  smartroom.ino          setup() dan loop(): merangkai modul, ID perangkat, NVS
  config.h               pin, interval, ambang bawaan
  secrets.example.h      contoh kredensial, salin jadi secrets.h
  src/                   semua modul
    hardware/            perangkat
      sensors.*          DHT22, LDR, PIR
      alarm_logic.h      fungsi keadaan alarm
      alarm.*            LED, buzzer, tombol
      display.*          OLED SSD1306
    network/             fungsi pengiriman data
      wifi_link.*        connect dan reconnecting wifi 
      mqtt.*             MQTT topik, Last Will, telemetri, perintah
      http_backup.*      kirim lewat HTTP saat MQTT disconnect
      ota_http.*         update firmware dari halaman: unduh HTTPS, sha256, rollback
    data/
      payload.*          JSON telemetri dan perintah (diuji di laptop)
    interval.h           penjadwal tanpa delay()
hardware-test/           enam sketsa uji Level 1
```

## Pin

| Perangkat | Pin ESP32 |
|---|---|
| DHT22 / DHT11 data | GPIO4 Jenis sensor dipilih di `config.h`: `DHT_TYPE` 22 atau 11 |
| LDR | GPIO34 |
| PIR output | GPIO27 |
| LED | GPIO25 / GPIO26 |
| Buzzer | GPIO33 |
| Push button | GPIO14 ke GND |
| OLED SDA / SCL | GPIO21 / GPIO22, 0x3C |

## Pengujian

1. **Arduino IDE 2** dengan board *ESP32 Dev Module*, partition scheme **Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)**.
2. **Library** (Library Manager):
   - Adafruit SSD1306 dan Adafruit GFX Library
   - DHT sensor library (Adafruit) dan Adafruit Unified Sensor
   - ArduinoJson **6.x**

3. **Kredensial**: `firmware/smartroom/secrets.example.h` menjadi `firmware/smartroom/secrets.h`.

   | Kolom | Isi |
   |---|---|
   | `WIFI_SSID`, `WIFI_PASSWORD` | Wi-Fi. |
   | `MQTT_URL` | `wss://iot.adhiteknologi.com:443/mqtt`, atau broker publik `mqtt://broker.emqx.io:1883` |
   | `MQTT_USERNAME` | Akun perangkat dari broker.adhiteknologi.com (Akun layanan), mis. `sr-aabbccddeeff`. Kosong + `MQTT_PASSWORD` diisi = login perangkat iot.adhiteknologi (username MAC, tanpa cadangan HTTP). |
   | `MQTT_PASSWORD` | Sandi akun itu. Kosong untuk broker publik. |
   | `HTTP_URL` | Biarkan `https://smart.adhiteknologi.com`. Cadangan HTTP memakai akun MQTT di atas |

4. Upload `firmware/smartroom/smartroom.ino`, lalu buka Serial Monitor 115200.

5. **Data Realtime**: Buka smart.adhiteknologi.com, pada halaman utama menampilkan tabel data yang dikirim perangkat. Untuk mengakses pengaturan diperlukan login. Tampilan halaman utama [Utama.png](Utama.png) dan halaman pengaturan [Pengaturan.png](Pengaturan.png).

## Ketentuan 

- Alarm menyala bila suhu **di atas** ambang (bawaan 30 °C).
- Tombol mematikan alarm. Alarm tidak berbunyi sampai suhu turun *ambang − 0,5 °C* lalu naik lagi melewati ambang. 0,5 °C mencegah alarm berkedip saat suhu naik turun tipis di sekitar ambang.
- Alarm mati bila suhu turun ke *ambang − 0,5 °C* tanpa ditekan.
- PIR diberikan waktu untuk stabil 30 detik pertama setelah aktif. OLED menampilkan `Gerak : Loading`.
- Pojok kanan atas OLED menunjukkan protokol: `MQTT`, `HTTP` (MQTT putus 5 detik atau lebih).
