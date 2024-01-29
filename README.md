# ESPNowCam - Camera streamer via ESPNow

[![PlatformIO](https://github.com/hpsaturn/esp32s3-cam/workflows/PlatformIO/badge.svg)](https://github.com/hpsaturn/esp32s3-cam/actions/) ![ViewCount](https://views.whatilearened.today/views/github/hpsaturn/esp32s3-cam.svg)  

ESPNowCam library, is a straightforward video streamer for popular ESP32Cam models, leveraging the ESPNow protocol. No need for IPs, routers, or credentials—keeping it simple! :D

<table>
  <tr>
    <td>
      Don't forget to star ⭐ this repository
    </td>
  </tr>
</table>

[![video demo](https://raw.githubusercontent.com/hpsaturn/esp32s3-cam/master/pictures/youtube.jpg)](https://youtu.be/zXIzP1TGlpA)

## Supported devices

**Cameras:**

- [x] ESP32S3 Cam Freenove
- [x] M5CoreS3 builtin Camera

**Receivers:**

- [x] M5Core2 (AWS tested)
- [x] M5CoreS3
- [x] Makerfabs Parallel using LGFX
- [ ] Maybe any TFT with LGFX support (better with PSRAM)

## Examples and Tests

| ENV Name   |    Target      |  Status |
|:-----------------|:--------------:|:----------:|
| freenove-espnow-sender  | ESPNow camera transmitter | STABLE |
| m5cores3-espnow-sender | ESPNow built-in camera transmitter | STABLE |
| m5cores3-espnow-receiver | Video receiver via ESPNow [2] | STABLE|
| m5core2-espnow-receiver | Video receiver via ESPNow [2] | STABLE |

[2] Use with freenove or M5CoreS3 espnow sender sample

### Install examples

For install and run these tests, first install [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.

For compile and install each sample, only choose one of them envs names in the table, and run the next command in the root of this project, like that:

```bash
pio run -e m5cores3-espnow-receiver --target upload
```

### Library installation

**PlatformIO**:

Add the following line to the lib_deps option of your [env:] section:

```python
hpsaturn/EspNowCam@^0.1.0
```

Or via command line:  

```python
pio pkg install --library "hpsaturn/EspNowCam@^0.1.0"
```

**Arduino IDE**:

For `Arduino IDE` is a little bit more complicated because the Arduino IDE dependencies resolver is very bad, but you only need first download and install the [Nanopb library](https://github.com/nanopb/nanopb/releases/tag/nanopb-0.4.8) using the `Include Library` section via zip file, and then with the **Library Manager** find **ESPNowCam**

## TODO

- [x] NanoPb possible issue #1 (payload size)
- [x] Unified ESPNow in an one class for all transmitters and receivers
- [x] Isolate the ESPNow Receiver and Transmitter in a seperated library
- [ ] Add sender callback to improve speed
- [ ] Migration to esp_wifi_80211_tx() to improve Payload and Quality

## ESPNow Transmitter and Receiver

The last version has many improvements, and right now is stable. It needs some tiny changes but now it supports ~9FPS with JPG Quality 12.

![ESPNow Camera Video](https://raw.githubusercontent.com/hpsaturn/esp32s3-cam/master/pictures/espnow_video.gif)

## Troubleshooting

The Freenove camera sometimes needs good power cable and also takes some seconds to stabilization, that means, that not worries for initial video glitches.

---
