/**************************************************
 * ESPNowCam video Transmitter.
 * ----------------------------
 *
 * XIAO ESP32S3 with the Seeed Studio AI Vision 2 camera module.
 * The AI module does the inference and sends the frames as JPEG, so this
 * sample only needs to stream them like any other ESPNowCam sender.
 *
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <OneButton.h>
#include <ESPNowCam.h>
#include <drivers/CamAIVision2.h>
#include <Utils.h>

CamAIVision2 Camera;  
WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);

OneButton btnB(GPIO_NUM_0, true);

void processFrame() {
  if (Camera.get()) {
    // the AI Vision 2 driver already provides a JPEG frame (PIXFORMAT_JPEG),
    // so there is no need to convert it with frame2jpg()
    radio.sendData(Camera.fb->buf, Camera.fb->len);
    printFPS("CAM:");
    Camera.free();
  }
}

void shutdown() {
  Serial.println("shutdown..");
  Camera.stopStream(); // stop the AI module tasks before sleeping
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0);
  delay(1000);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  delay(4000); // only for debugging 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // Optional: for better performance you can use the WiFi raw (80211tx)
  // implementation instead of ESP-NOW, see wifiraw-80211tx-* examples.

  // Optional M5Core2 receiver B8:F0:09:C6:0E:CC
  const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  radio.setChannel(6);
  radio.init(960);

  // The AI module sends the frames encoded in base64 over its UART, and that
  // link (921600 baud ~ 90KB/s) is the real limit of this camera: a 240x240
  // JPEG is ~13KB (17KB in base64, ~200ms), while 480x480 and VGA are ~4x and
  // ~6x bigger, so they cap the FPS below 1 and they overflow the receiver
  // buffers. Keep the 240x240 resolution (0) unless you really need more.
  // 0 -> 240x240 (default), 1 -> 480x480, 2 -> VGA 640x480
  Camera.setResolution(0);

  // Optional: ask for the AI results too instead of the plain image (the
  // driver only sends the image, so it is slower and does not change here)
  Camera.setFrameMode(CamAIVision2::FRAME_INVOKE);

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
  }

  // Optional: the module pushes frames as fast as it can, but its buffers
  // overflow when the host can not drain them, so the default (a frame per
  // request) is the recommended one for this radio
  Camera.setStreamMode(CamAIVision2::STREAM_CONTINUOUS);

  // ...or with any other SSCMA AT command, e.g. the model thresholds:
  // Camera.sendCmd("TSCORE=60");
  // more details: https://github.com/Seeed-Studio/SSCMA-Micro/blob/main/docs/protocol/at-protocol-en_US.md

  btnB.attachClick([]() { shutdown(); });
  delay(100);
}

void loop() {
  processFrame();
  btnB.tick();
}
