# esp32s3 camera tests

[![PlatformIO](https://github.com/hpsaturn/esp32s3-cam/workflows/PlatformIO/badge.svg)](https://github.com/hpsaturn/esp32s3-cam/actions/) ![ViewCount](https://views.whatilearened.today/views/github/hpsaturn/esp32s3-cam.svg)  

Basic tests and examples over ESP32S3 Cameras.

<table>
  <tr>
    <td>
      Don't forget to star ⭐ this repository
    </td>
  </tr>
</table>

[![video demo](pictures/youtube.jpg)](https://youtu.be/zXIzP1TGlpA)

## Supported devices

**Cameras:**

- [x] ESP32S3 Cam Freenove
- [x] M5CoreS3 builtin Camera

**Receivers:**

- [x] M5Core2 (AWS tested)
- [x] M5CoreS3
- [ ] Maybe any TFT with LGFX support and PSRAM

## Examples and Tests

| ENV Name   |    Target      |  Status |
|:-----------------|:--------------:|:----------:|
| freenove-basic   |  Basic FPS and JPEG speed tester | STABLE |
| freenove-espnow  | ESPNow camera transmitter | STABLE |
| freenove-espnow-stats | Basic stats via ESPNowJoystick [1] | STABLE |
| freenove-webcam | Original Freenove example with improvements  | STABLE |
| m5cores3-basic | Basic builtin Camera/Display test | STABLE |
| m5cores3-espnow | ESPNow built-in camera transmitter | STABLE |
| m5cores3-espnow-stats |Basic stats via ESPNowJoystick [1] | STABLE |
| m5cores3-espnow-receiver | Video receiver via ESPNow [2] | STABLE|
| core2-espnow-receiver | Video receiver via ESPNow  | STABLE |

[1] For receive the stats, configure other ESP32 how is explained here: [ESPNowJoystick Library](https://github.com/hpsaturn/espnow-joystick#readme)

[2] Use with freenove or M5CoreS3 espnow sample

## Usage and Install

Please install first [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.

For compile and install each sample, only choose one of them envs names in the table, and run the next command in the root of this project, like that:

```bash
pio run -e m5cores3-espnow-receiver --target upload
```

## TODO

- [x] NanoPb possible issue #1 (payload size)
- [x] Unified ESPNow in an one class for all transmitters and receivers
- [ ] Add sender callback to improve speed
- [ ] Isolate the ESPNow Receiver and Transmitter in a seperated library
- [ ] Migration to esp_wifi_80211_tx() to improve Payload

## ESPNow Transmitter and Receiver

The last version has many improvements, and right now is stable. It needs some tiny changes but now it supports ~9FPS with JPG Quality 12.

![ESPNow Camera Video](pictures/espnow_video.gif)

## Troubleshooting

The Freenove camera sometimes needs good power cable and also takes some seconds to stabilization, that means, that not worries for initial crashes.

**This is a proof of concept and some basic tests and examples, don't hope to much**  

---
