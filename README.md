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
| freenove-espnow  | ESPNow camera transmitter | INPROGRESS |
| freenove-espnow-stats | Basic stats via ESPNowJoystick [1] | STABLE |
| freenove-webcam | Original Freenove example with improvements  | STABLE |
| m5cores3-basic | Basic builtin Camera/Display test | STABLE |
| m5cores3-espnow | ESPNow built-in camera transmitter | INPROGRESS |
| m5cores3-espnow-stats |Basic stats via ESPNowJoystick [1] | STABLE |
| m5cores3-espnow-receiver | Video receiver via ESPNow [2] |INPROGRESS |
| core2-espnow-receiver | Video receiver via ESPNow  | INPROGRESS |
||||


[1] For receive the stats, configure other ESP32 how is explained here: [ESPNowJoystick Library](https://github.com/hpsaturn/espnow-joystick#readme)

[2] Use with freenove or M5CoreS3 espnow sample

## Usage and Install

Please install first [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.

For compile and install each sample, only choose one of them envs names in the table, and run the next command in the root of this project, like that:

```bash
pio run -e m5cores3-espnow-receiver --target upload
```

## TODO

- [ ] NanoPb possible issue #1 (payload size)
- [ ] Unified ESPNow in an one class for all transmitters and receivers
- [ ] Migration to esp_wifi_80211_tx() to improve Payload

## Troubleshooting

The Freenove camera sometimes needs good power cable and also takes some seconds to stabilization, that means, that not worries for initial crashes.

**This is a proof of concept and some basic tests and examples, don't hope to much**  

---
