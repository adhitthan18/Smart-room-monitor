# SmartRoom ESP32

Prototype smart room menggunakan esp32 dengan protokol MQTT sebagai main dan HTTP sebagai cadangan jika MQTT disconnect.

## Pemenuhan syarat

| Persyaratan | Pengerjaan |
|---|---|
| **Level 1: Hardware** | |
| Skematik rangkaian | ada di [config.h](firmware/smartroom/config.h). Gambar [Schematic.png](Schematic.png) |
| Design PCB | Top-Layer [PCB-TopLayer.png](PCB-TopLayer.png). Bottom-Layer [PCB-BottomLayer.png](PCB-BottomLayer.png) |
| Uji perangkat | ada di [hardware-test/](hardware-test/): LED dan buzzer, button, DHT, LDR, PIR, serta OLED |
| Foto prototype | Foto rangkaian prototype yang dikerjakan [Prototype-SmartRoom.jpeg](Prototype-SmartRoom.jpeg) |
| Video pengujian | [https://youtu.be/M9Wnl7IGAFw?si=bWgMDrhiYPDpfMWm](https://youtu.be/M9Wnl7IGAFw?si=bWgMDrhiYPDpfMWm) |
| **Level 2: Firmware** | |
| Pembacaan sensor | menggunakan `millis()` di [interval.h](firmware/smartroom/src/interval.h). [sensors.cpp](firmware/smartroom/src/hardware/sensors.cpp) DHT setiap 5 detik, LDR setiap 1 detik, dan PIR `loop()`. |
| Alarm dan button | [alarm_logic.h](firmware/smartroom/src/hardware/alarm_logic.h) mengatur keadaan normal, aktif, dan acknowledged. [alarm.cpp](firmware/smartroom/src/hardware/alarm.cpp) mengendalikan LED, buzzer, dan button |
| OLED | [display.cpp](firmware/smartroom/src/hardware/display.cpp) menampilkan suhu, kelembapan, cahaya, gerakan, ambang, jalur data, dan status alarm |
| Jika sensor tidak terbaca | maka data DHT menjadi `null` pada telemetri, keadaan alarm tetap, OLED yang tidak terdeteksi akan diabaikan, dan komunikasi I2C diberi timeout 50 ms |
| Kode | dipisahkan dalam [hardware/](firmware/smartroom/src/hardware/), [network/](firmware/smartroom/src/network/), dan [data/](firmware/smartroom/src/data/). [smartroom.ino](firmware/smartroom/smartroom.ino) mengatur inisialisasi |
| **Level 3: MQTT** | |
| WiFi reconnect otomatis | [wifi_link.cpp](firmware/smartroom/src/network/wifi_link.cpp) mengatur koneksi dan penyambungan ulang WiFi. |
| Kredensial di luar repository | Kredensial disimpan di `firmware/smartroom/secrets.h` |
| Telemetri JSON setiap 5 detik | [mqtt.cpp](firmware/smartroom/src/network/mqtt.cpp) mengirim ke `smartroom/<device_id>/telemetry`. `device_id` menggunakan MAC ESP32. Contoh telemetri dicantumkan di bawah. |
| Perintah ubah ambang | Perangkat menerima perintah pada `smartroom/<device_id>/command`. [payload.cpp](firmware/smartroom/src/data/payload.cpp) memvalidasi nilai ambang sebelum diterapkan. |
| Status dan Last Will | [mqtt.cpp](firmware/smartroom/src/network/mqtt.cpp) mengatur status retained `online` dan Last Will `offline` pada `smartroom/<device_id>/status` |
| Broker | Broker implementasi: `wss://iot.adhiteknologi.com:443/mqtt` |
| **Tambahan opsional** | |
| Cadangan HTTP | [http_backup.cpp](firmware/smartroom/src/network/http_backup.cpp) mengirim telemetri ke `POST https://smart.adhiteknologi.com/api/devices/<device_id>/telemetry` ketika MQTT disconnect 5 detik

### Contoh telemetri

Contoh berikut dari log Serial Monitor.

```json
{
  "device_id": "28:05:A5:25:59:28",
  "uptime_s": 80,
  "temperature": 30.8,
  "humidity": 65,
  "light_pct": 100,
  "motion": true,
  "alarm": "normal",
  "threshold": 34,
  "rssi": -44,
  "fw": "smartroom-2.5"
}
```

Status `normal` pada contoh tersebut sesuai dengan suhu 30,8 °C yang masih di bawah ambang runtime 34 °C.

### Pengujian perintah ambang

contoh payload.

```json
{"threshold": 28.5}
```

### Konfigurasi yang perlu disesuaikan

Sebelum melakukan pengujian, sesuaikan [config.h](firmware/smartroom/config.h) dengan komponen dan pin:

- `DHT_TYPE` saat ini `11`, gunakan `22` untuk sensor DHT22
- `DEFAULT_THRESHOLD_C` saat ini `35.0f`, Ambang yang sudah tersimpan di NVS tidak terhapus setelah restart.
- `BUZZER_ACTIVE_HIGH` saat ini `false`, Jika memakai modul buzzer aktif LOW gunakan `true`

## Struktur

```textfirmware/smartroom/     
  smartroom.ino          
  config.h               pin, interval, ambang
  secrets.example.h      contoh kredensial, copy jadi secrets.h
  src/                   
    hardware/            deklarasi perangkat
      sensors.*          DHT22, LDR, PIR
      alarm_logic.h      fungsi keadaan alarm
      alarm.*            LED, buzzer, button
      display.*          OLED 
    network/             fungsi pengiriman data
      wifi_link.*        connect dan reconnecting wifi 
      mqtt.*             MQTT topik, Last Will, telemetri, perintah
      http_backup.*      kirim lewat HTTP saat MQTT disconnect
      ota_http.*         update firmware dari dashboard
    data/
      payload.*          JSON telemetri dan perintah 
    interval.h           tanpa delay()
hardware-test/           uji sensor
```

## Pin

| Perangkat | Pin ESP32 |
|---|---|
| DHT22 / DHT11 | GPIO4, Jenis sensor 22 atau 11 |
| LDR | GPIO34 |
| PIR output | GPIO27 |
| LED | GPIO25 / GPIO26 |
| Buzzer | GPIO33 |
| Push button | GPIO14 dan GND |
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

4. **Upload** `firmware/smartroom/smartroom.ino`, lalu buka Serial Monitor 115200.

5. **Data Realtime**: Buka smart.adhiteknologi.com, pada halaman utama menampilkan tabel data yang dikirim perangkat. Untuk mengakses pengaturan diperlukan login. Tampilan halaman utama [Utama.png](Utama.png) dan halaman pengaturan [Pengaturan.png](Pengaturan.png).

6. **OTA**: Naikkan versi pada config.h dan export compiled binary. upload file bin pada halaman config smart.adhiteknologi.com, proses download akan terlihat pada OLED. 

## Ketentuan 

- Alarm menyala bila suhu **di atas** ambang (bawaan 30 °C).
- Tombol mematikan alarm. Alarm tidak berbunyi sampai suhu turun *ambang − 0,5 °C* lalu naik lagi melewati ambang. 0,5 °C mencegah alarm berkedip saat suhu naik turun tipis di sekitar ambang.
- Alarm mati bila suhu turun ke *ambang − 0,5 °C* tanpa ditekan.
- PIR diberikan waktu untuk stabil 30 detik pertama setelah aktif. OLED menampilkan `Gerak : Loading`.
- Pojok kanan atas OLED menunjukkan protokol: `MQTT`, `HTTP` (MQTT putus 5 detik atau lebih).

## Pengembangan selanjutnya ada pada data lokasi dan protokol yang sedang berjalan