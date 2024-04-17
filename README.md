# ESPNowCam - Camera streamer via ESPNow

[![PlatformIO](https://github.com/hpsaturn/esp32s3-cam/workflows/PlatformIO/badge.svg)](https://github.com/hpsaturn/esp32s3-cam/actions/) ![ViewCount](https://views.whatilearened.today/views/github/hpsaturn/esp32s3-cam.svg)  

The ESPNowCam library is a simple and direct video streamer designed for popular ESP32Cam models, utilizing the ESPNow protocol. No need for IPs, routers, or credentials—keeping it straightforward and hassle-free! :D

**This library is for general purpose**, as it accepts pointers to various types of data, including buffers, strings, images, or any byte-formatted content. This flexibility enables transmission of larger packages across different scenarios, not limited to cameras alone. For instance, a buffer of 4000 bytes takes approximately 1/9 of a second to transmit, resulting in a frame rate of around 9FPS.

<table>
  <tr>
    <td>
      Don't forget to star ⭐ this repository
    </td>
  </tr>
</table>

## Performance

The current version tested with:

| Sender | Receiver | Frame size | JPG Quality | FPS | Status |
|:-----------------|:-----------:|:-------:|:-----:|:------:|:------:|
| Freenove S3 Camera | M5Core2 / M5CoreS3 | QVGA | 12 | ~10 FPS | STABLE |
| M5CoreS3 builtin Camera | M5Core2 | QVGA | 12  | ~11 FPS | STABLE |
| Freenove S3 Camera | Makerfabs | HVGA | 12 | ~6 FPS | STABLE |
| XIAO ESP32S3 Camera | M5Core2 / M5CoreS3 | QVGA | 12 | ~9 FPS | STABLE |

## Library installation

**PlatformIO**:

Add the following line to the lib_deps option of your [env:] section:

```python
hpsaturn/EspNowCam@^0.1.6
```

Or via command line:  

```python
pio pkg install --library "hpsaturn/ESPNowCam@^0.1.6"
```

**Arduino IDE**:

For `Arduino IDE` is a little bit more complicated because the Arduino IDE dependencies resolver is very bad, but you only need:

1. Download and install the [Nanopb library](https://github.com/nanopb/nanopb/releases/tag/nanopb-0.4.8) using the `Include Library` section via zip file
2. and then with the **Library Manager** find **ESPNowCam** and install it.  

Note: Nanobp is not included as a dependency because, despite being 25 years after the invention of symbolic links, Arduino IDE does not support these types of files. Consider exploring PlatformIO for your future developments, as it offers a more versatile and modern development environment.

## Examples and Tests

[![video demo](https://raw.githubusercontent.com/hpsaturn/esp32s3-cam/master/pictures/youtube.jpg)](https://youtu.be/nhLr7XEUdfU)  
[[video]](https://youtu.be/nhLr7XEUdfU)

| ENV Name   |    Target      |  Status |
|:-----------------|:--------------:|:----------:|
| freenove-basic-sender  | ESPNow camera transmitter (QVGA) | STABLE |
| freenove-hvga-sender  | ESPNow camera transmitter (HVGA) | <6 FPS |
| freenove-nojpg-sender  | ESPNow camera transmitter (NOJPG) | DEMO ONLY (<2FPS) |
| freenove-tank  | Advanced sample. Sender/Receiver | TESTING |
| xiao-espnow-sender  | ESPNow camera transmitter (QVGA) | STABLE |
| m5core2-basic-receiver | Video receiver via ESPNow [1] | STABLE |
| m5core2-espnow-receiver | Video receiver via ESPNow [1] | STABLE |
| m5cores3-espnow-receiver | Video receiver via ESPNow [1] | STABLE|
| m5cores3-espnow-sender | ESPNow built-in camera transmitter | STABLE |
| m5stickCplus-joystick-tank | Advanced sample. Custom payload | TESTING |  
| makerfabs-basic-receiver | Video receiver via ESPNow [2] | STABLE |  
| makerfabs-nojpg-receiver | Video receiver via ESPNow [2] | DEMO ONLY (<2FPS) |  

[1] Use with freenove or M5CoreS3 senders sample  
[2] Use with freenove HVGA sender sample

### Install examples

For install and run these tests, first install [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.

For compile and install each sample, only choose one of them envs names in the table, and run the next command in the root of this project, like that:

```bash
pio run -e m5cores3-espnow-receiver --target upload
```

Some examples, only needs run `pio run --target upload` into each directory

## Tested devices

**Cameras:**

- [x] ESP32S3 Cam Freenove
- [x] M5CoreS3 builtin Camera
- [x] XIAO ESP32S3 Sense Camera

**Receivers:**

- [x] M5Core2 (AWS tested)
- [x] M5CoreS3
- [x] Makerfabs Parallel using LGFX
- [x] TFT 3.5 and 2.5 " using LGFX (ILI9488/9486)
- [x] Generic TFT with LGFX support (better with PSRAM)

## TODO

- [x] NanoPb possible issue #1 (payload size)
- [x] Unified ESPNow in an one class for all transmitters and receivers
- [x] Isolate the ESPNow Receiver and Transmitter in a seperated library
- [x] Unified and migrated to only one header `ESPNowCam.h`
- [x] Add sender callback to improve speed
- [ ] Migration to esp_wifi_80211_tx() to improve Payload and Quality

## ESPNow Transmitter and Receiver

The last version has many improvements, and right now is very stable. It needs some tiny changes but now it supports ~9FPS with JPG Quality 18.

[![ESPNow Camera Video](https://raw.githubusercontent.com/hpsaturn/esp32s3-cam/master/pictures/espnow_video.gif)](https://youtu.be/zXIzP1TGlpA)  
[[video]](https://youtu.be/zXIzP1TGlpA)

## Troubleshooting

The **Freenove camera** sometimes needs good power cable and also takes some seconds to stabilization, that means, that not worries for initial video glitches.

The **XIAO camera** experiences thermal issues; after a few minutes, it may stop transmission and won't restart it until it's cooled down.

For **Arduino IDE users**, if you have a compiling error, maybe you forget install NanoPb library. Please see above.

This project was developed and thoroughly tested on PlatformIO. While I did compile and execute it successfully on Arduino IDE using Espressif 2.0.11 and Arduino IDE 2.2.1, with PSRAM enabled, I generally avoid using Arduino IDE due to its tendency to mix everything and its buggy nature. Therefore, **I highly recommend using PlatformIO** for a smoother and more reliable development experience.

## Credits

I want to extend my gratitude to @ElectroZeusTIC and @AcoranTf for testing it on Arduino IDE.

---
