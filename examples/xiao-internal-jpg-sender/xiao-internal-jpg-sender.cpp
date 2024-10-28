/**************************************************
 * ESPNowCam video Transmitter.
 * ----------------------------
 *
 * XIAO FPV ESPNow transmitter uses some extra features of
 * this board to have some power consumption improvements.
 *
 * Also in this sample we are using DRAM, not PSRAM. It is
 * more faster than PSRAM, and also we are using the internal
 * compressor, that consume more bandwidht but the quality
 * is so good with the same JPG compression. I don't know why ??
 * The only consideration is that the reciever needs allocate
 * more memory.
 *
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
 **************************************************/

#include <Arduino.h>
#include <ESPNowCam.h>
#include <OneButton.h>
#include <Utils.h>
#include <drivers/CamXiao.h>

CamXiao Camera;
ESPNowCam radio;
OneButton btnB(GPIO_NUM_0, true);

void processFrame() {
  if (Camera.get()) {
    radio.sendData(Camera.fb->buf, Camera.fb->len);
    delay(60);  // ==> weird delay when you are using only DRAM.
    printFPS("CAM:");
    Camera.free();
  }
}

void shutdown() {
  Serial.println("shutdown..");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  delay(1000);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  // delay(4000);  // only for debugging

  if (psramFound()) {
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // Optional Makerfabs receiver 7C:DF:A1:F3:73:3C
  // const uint8_t macRecv[6] = {0x7C, 0xDF, 0xA1, 0xF3, 0x73, 0x3C};
  // Optional M5Core2 receiver B8:F0:09:C6:0E:CC
  // const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  // radio.setTarget(macRecv);
  radio.init();

  // Configuration without using the PSRAM, only DRAM. (more faster)
  Camera.config.pixel_format = PIXFORMAT_JPEG;
  Camera.config.frame_size = FRAMESIZE_QVGA;
  Camera.config.fb_count = 2;
  Camera.config.fb_location = CAMERA_FB_IN_DRAM;

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
  }

  btnB.attachClick([]() { shutdown(); });
  delay(100);
}

void loop() {
  processFrame();
  btnB.tick();
}
