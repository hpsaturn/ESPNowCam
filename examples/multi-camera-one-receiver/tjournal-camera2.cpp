/**************************************************
 * ESPNowCam video Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <ESPNowCam.h>
#include <drivers/CamTJournal.h>
#include <Utils.h>

CamTJournal Camera;
ESPNowCam radio;

void processFrame() {
  if (Camera.get()) {
    radio.sendData(Camera.fb->buf, Camera.fb->len);
    delay(60); // ==> weird delay for NOPSRAM camera. 
    printFPS("CAM:");
    Camera.free();
  }
}

void setup() {
  Serial.begin(115200);

  delay(100); // only for debugging 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // M5Core2 receiver MAC: B8:F0:09:C6:0E:CC
  // const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  // Makerfabs receiver 7C:DF:A1:F3:73:3C
  const uint8_t macRecv[6] = {0x7C,0xDF,0xA1,0xF3,0x73,0x3C};
  radio.setTarget(macRecv);
  radio.init();

  // You are able to change the Camera config E.g:
  Camera.config.fb_count = 1;
  Camera.config.frame_size = FRAMESIZE_QQVGA;

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
    ESP.restart();
  }
  delay(500);
}

void loop() {
  processFrame();
}
