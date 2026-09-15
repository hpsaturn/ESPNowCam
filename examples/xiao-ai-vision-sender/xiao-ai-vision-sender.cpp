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
ESPNowCam radio;
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
  // const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  // radio.setTarget(macRecv);
  radio.init();

  // You are able to change the sensor resolution of the AI module:
  // 0 -> 240x240 (default), 1 -> 480x480, 2 -> VGA 640x480
  // Camera.setResolution(1);

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
  }

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
