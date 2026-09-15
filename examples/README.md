# Examples

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
| esp32cam-p2p-sender | PSRAM, 2FB, JPG | QVGA | UNTESTED |
| freenoveWR-basic-sender | PSRAM, 2FB, JPG | QVGA | UNTESTED |

### Receivers samples

Some video receivers

| ENV Name   |    Details      |   Status |
|:-----------------|:--------------:|:----------:|
| m5core2-basic-receiver   | M5Stack M5Core2 [1] |  STABLE |
| m5core2-espnow-receiver  | M5Stack M5Core2 [1] |  STABLE |
| m5cores3-espnow-receiver | M5Stack M5CoreS3 [1] |  STABLE |
| makerfabs-basic-receiver | Makerfabs RGB Parallel [1] [2] |  STABLE |  
| makerfabs-nojpg-receiver | Makerfabs RGB Parallel [1] [2] | <2FPS |
| tft-3.5-basic-receiver | Any TFT display with LGFX [1] | STABLE |
| tft-il9485-basic-receiver | M5Core TFT reciver il9481 [1] [2]| STABLE |
| tft-rgb-hmi-basic-receiver | ESP32S3_RGB_ESP32-8048S043 [1] [2] | STABLE |
| tft-s3-RGB-800x480-receiver | ESP32S3_RGB_ESP32-8048S043 [1] | STABLE |
| crowpanel-receiver | Elecrow round panel 320x240 [1] | TESTING |

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
