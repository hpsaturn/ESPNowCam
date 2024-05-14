# ESPNowCam - Data streamer

[![PlatformIO](https://github.com/hpsaturn/esp32s3-cam/workflows/PlatformIO/badge.svg)](https://github.com/hpsaturn/esp32s3-cam/actions/) ![ViewCount](https://views.whatilearened.today/views/github/hpsaturn/esp32s3-cam.svg)  

The ESPNowCam library is a simple and direct video or data streamer designed for popular ESP32 devices, utilizing the ESPNow protocol. No need for IPs, routers, or credentials—keeping it straightforward and hassle-free! :D

**This library is for general purpose**, as it accepts pointers to various types of data, including buffers, strings, images, or any byte-formatted content. This flexibility enables transmission of larger packages across different scenarios, not limited to cameras alone. For instance, a buffer of 4000 bytes takes approximately 1/9 of a second to transmit, resulting in a frame rate of around 9FPS.

<table>
  <tr>
    <td>
      Don't forget to star ⭐ this repository
    </td>
  </tr>
</table>

## Features

The latest version brings numerous enhancements and is currently highly stable. It offers support for various transmission modes, including one transmitter to multiple receivers in real-time using the internal ESPNow broadcasting feature (1:N), peer-to-peer (P2P) connections utilizing MAC address targeting (1:1), and multi-sender mode with one receiver (N:1). It's important to note that the library is versatile and capable of transmitting various types of data, not limited to video.

[![ESPNowCam broadcast camera mode](https://raw.githubusercontent.com/hpsaturn/ESPNowCam/master/pictures/broadcast-camera-mode.gif)](https://youtu.be/zXIzP1TGlpA) [![ESPNowCam P2P mode](https://raw.githubusercontent.com/hpsaturn/ESPNowCam/master/pictures/p2p-camera-mode.gif)](https://youtu.be/XDIiJ25AKr8) [![ESPNowCam multi camera mode](https://raw.githubusercontent.com/hpsaturn/ESPNowCam/master/pictures/multi-camera-mode.gif)](https://youtu.be/ip6RohVEg2s)  
[[1:N mode video]](https://youtu.be/zXIzP1TGlpA) [[1:1 mode video]](https://youtu.be/XDIiJ25AKr8) [[N:1 mode video]](https://youtu.be/ip6RohVEg2s)

## Performance

The current version was tested with the next cameras:

| Sender |  Frame | PSRAM | JPGQ | FPS | Status |
|:---------|:-----:|:-----:|:------:|:-------:|:------:|
| TTGO TJournal |  QVGA | No | 12 | ~11 FPS | STABLE |
| XIAO Sense S3 | QVGA | Yes | 12 | ~11 FPS | STABLE |
| Freenove S3 | QVGA | Yes | 12 | ~10 FPS | STABLE |
| Freenove S3 | HVGA | Yes | 12 | ~6 FPS | STABLE |
| M5CoreS3 | QVGA | Yes | 12  | ~11 FPS | STABLE |
| M5UnitCamS3 | QVGA | Yes | 12 | ~9 FPS | STABLE |

[Full list of senders and receivers devices supported](#supported-devices)

## Library installation

**PlatformIO**:

Add the following line to the lib_deps option of your [env:] section:

```python
hpsaturn/EspNowCam@^0.1.12
```

Or via command line:  

```python
pio pkg install --library "hpsaturn/ESPNowCam@^0.1.12"
```

**Arduino IDE**:

For `Arduino IDE` is a little bit more complicated because the Arduino IDE dependencies resolver is very bad, but you only need:

1. Download and install the [Nanopb library](https://github.com/nanopb/nanopb/releases/tag/nanopb-0.4.8) using the `Include Library` section via zip file
2. and then with the **Library Manager** find **ESPNowCam** and install it.  

Note: Nanobp is not included as a dependency because, despite being 25 years after the invention of symbolic links, Arduino IDE does not support these types of files. Consider exploring PlatformIO for your future developments, as it offers a more versatile and modern development environment.

## Usage

**To send** any kind of data, you only need a buffer and the size to send:

```cpp
ESPNowCam radio;

radio.init();
radio.sendData(out_jpg, out_jpg_len);
```

**To receive** the data, you only need to define a buffer and callback:

```cpp
radio.setRecvBuffer(fb);
radio.setRecvCallback(onDataReady);
radio.init();
```

```cpp
void onDataReady(uint32_t lenght) {
  tft.drawJpg(fb, lenght , 0, 0, dw, dh);
}
```

Note: if you don't define any specific target, the radio will work in broadcasting mode, that means **1:N mode**, for instance one camera sending video to multiple screen receivers.

### P2P mode (1:1)

It's also possible to define a specific target:

```cpp
uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
radio.setTarget(macRecv);
radio.init();
```

Note: this mode is very recommended to increase the performance, and also it reduces the noise and possible glitches.

### Multi camera mode (N:1)

Is possible too configure multiple cameras or senders to only one receiver, N:1 mode, configuring filters by MAC in the receiver:

```cpp
radio.setRecvFilter(fb_cam1, mac_cam1, onCam1DataReady);
radio.setRecvFilter(fb_cam2, mac_cam2, onCam2DataReady);
radio.setRecvFilter(fb_cam3, mac_cam3, onCam3DataReady);
```

and each camera should have configured the receiver MAC like a target. Fore more details, please follow the [multi-camera-one-receiver](https://github.com/hpsaturn/ESPNowCam/tree/master/examples/multi-camera-one-receiver/) directory example.

### Predefined drivers

The library includes some pre-defined camera configs to have an easy implementation, for example:

```cpp
#include <ESPNowCam.h>
#include <drivers/CamFreenove.h>

CamFreenove Camera;
```

and you able to change the Camera parameters, e.g:

```cpp
Camera.config.fb_count = 2;
Camera.config.frame_size = FRAMESIZE_QQVGA;
```

For now, it includes drivers for FreenoveS3, XIAOS3, M5UnitCamS3, ESP32Cam AI-Thinker and the TTGO T-Journal cameras, but you are able to define your custom camera like is shown in the [custom-camera-sender](examples/custom-camera-sender/) example. If you can run it in a different camera, please notify me :D

### PSRAM or DRAM?

Well, in my last tests with different cameras and using QVGA frame size, seems that is better using the DRAM and the internal JPG. DRAM is more faster than PSRAM, and the internal compressor has a better quality with the same JPGQ level, but it uses more bandwidth, on the other hand the result is so good on P2P mode.

For change to DRAM and the internal JPG, you are able to pre-configure it like this:

```cpp
Camera.config.pixel_format = PIXFORMAT_JPEG;
Camera.config.frame_size = FRAMESIZE_QVGA;
Camera.config.fb_count = 2;
Camera.config.fb_location = CAMERA_FB_IN_DRAM;
```

more details in the sample [xiao-internal-jpg-sender](examples/xiao-internal-jpg-sender/).

## Examples

[![Tank Example - Video demo](https://raw.githubusercontent.com/hpsaturn/ESPNowCam/master/pictures/tank_example.jpg)](https://youtu.be/nhLr7XEUdfU) [![ESPNowCam Broadcasting - Video demo](https://raw.githubusercontent.com/hpsaturn/ESPNowCam/master/pictures/broadcasting_example.jpg)](https://youtu.be/zXIzP1TGlpA) [![Multi camera - Video demo](https://raw.githubusercontent.com/hpsaturn/ESPNowCam/master/pictures/multi-camera.jpg)](https://youtu.be/ip6RohVEg2s)

### Transmiter Camera samples

| ENV Name   |    Details      | Frame|   Status |
|:-----------------|:--------------:|:----------:|:----------:|
| freenove-p2p-sender  | PSRAM, 2FB, JPG | QVGA | STABLE |
| freenove-hvga-sender  | PSRAM, 2FB, JPG | HVGA | <6 FPS |
| freenove-nojpg-sender  | PSRAM, 2FB, NOJPG | QVGA | <2FPS |
| xiao-espnow-sender  |  PSRAM, 2FB, JPG | QVGA | STABLE |
| unitcams3-basic-sender | PSRAM, 2FB, JPG | QVGA | TESTING |
| custom-camera-sender | Custom settings - optional PSRAM | QVGA | STABLE |
| tjournal-espnow-sender  | NOPSRAM, 1FB, internal JPG | QVGA | STABLE |
| m5cores3-espnow-sender | PSRAM, 2FB, JPG built-in camera | QVGA | STABLE |
| esp32cam-p2p-sender | PSRAM, 1FB, IDF-JPG | QVGA | UNTESTED |

### Receivers samples

| ENV Name   |    Details      |   Status |
|:-----------------|:--------------:|:----------:|
| m5core2-basic-receiver | Video receiver [1] |  STABLE |
| m5core2-espnow-receiver | Video receiver [1] |  STABLE |
| m5cores3-espnow-receiver | Video receiver [1] |  STABLE|
| makerfabs-basic-receiver | Video receiver [1] [2] |  STABLE |  
| makerfabs-nojpg-receiver | Video receiver [1] [2] | <2FPS |
| tft-3.5-basic-receiver | Any TFT display with LGFX [1] | STABLE |

[1] Use with any sender sample  
[2] Use with freenove HVGA sender sample for example.

### Advanced samples

| ENV Name   |    Details      | Frame|   Status |
|:-----------------|:--------------:|:----------:|:----------:|
| xiao-fpv-sender  | POWER ON/OFF, PSRAM, 2FB, JPG | QVGA | STABLE |
| xiao-internal-jpg-sender  | POWER ON/OFF, NOPSRAM, 2FB, IDF-JPG | QVGA | STABLE |
| freenove-tank | sender and custom payload receiver | QVGA | TESTING |
| m5stickCplus-joystick-tank | custom payload - Telemetry | -- | TESTING |  
| makerfabs-multi-receiver | N:1 mode, muti camera one receiver | -- | TESTING |  
| m5cores3-camera1 | One target only for multi-receiver sample | QVGA | TESTING |  
| tjournal-camera2 | One target only for multi-receiver sample | QQVGA | TESTING |  
| xiao-camera3 | One target only for multi-receiver sample | QQVGA | TESTING |  

## Running samples

For install and run these tests, first install [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.

For compile and install each sample, only choose one of them envs names in the table, and run the next command in the root of this project, like this:

```bash
pio run -e m5cores3-espnow-receiver --target upload
```

Some examples, *.ino samples, only needs run `pio run --target upload` into each directory

## Troubleshooting

To increase the performance, **the recommended use is the 1:1 mode**, and also is a good practice to configure the other radio senders around of this device in this mode, because if you have other senders in broadcasting mode together, them could be generating interference.

The **Freenove camera** sometimes needs good power cable and also takes some seconds to stabilization, that means, that not worries for initial video glitches.

For **Arduino IDE users**, if you have a compiling error, maybe you forget install NanoPb library. Please see above.

This project was developed and thoroughly tested on PlatformIO. While I did compile and execute it successfully on Arduino IDE using Espressif 2.0.11 and Arduino IDE 2.2.1, with PSRAM enabled, I generally avoid using Arduino IDE due to its tendency to mix everything and its buggy nature. Therefore, **I highly recommend using PlatformIO** for a smoother and more reliable development experience.

## Supported Devices

The library was tested on the next devices:

**Cameras:**

- [x] ESP32 TTGO T-Journal (without PSRAM)
- [x] ESP32S3 Freenove Camera
- [x] M5CoreS3 (builtin Camera)
- [x] XIAO ESP32S3 Sense Camera
- [x] M5UnitCamS3
- [ ] ESP32Cam AI-Thinker (untested. Help wanted)

**Receivers:**

- [x] M5Core2 (tested on the AWS version)
- [x] M5CoreS3
- [x] Makerfabs Parallel using LGFX
- [x] TFT 3.5 and 2.5 " using LGFX (ILI9488/9486)
- [x] Generic TFT with LGFX support (better with PSRAM)

## TODO

- [x] NanoPb possible issue #1 (payload size)
- [x] Unified ESPNow in an one class for all transmitters and receivers
- [x] Isolate the ESPNow Receiver and Transmitter in a seperated library
- [x] Add sender callback to improve speed
- [x] Added internal drivers for some popular Cameras
- [x] Added multi-camera support with one only target
- [ ] Migration to esp_wifi_80211_tx() to improve Payload and Quality

## Credits

I want to extend my gratitude to @ElectroZeusTIC and @AcoranTf for testing it on Arduino IDE. Also to @UtaAoya for your findings around the M5UnitCam device.

---
