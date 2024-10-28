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
    delay(40); // ==> weird delay for NOPSRAM camera. 
    printFPS("CAM:");
    Camera.free();
  }
}

void setup() {
  Serial.begin(115200);

  delay(1000); // only for debugging 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  
  // Makerfabs receiver 7C:DF:A1:F3:73:3C
  // const uint8_t macRecv[6] = {0x7C,0xDF,0xA1,0xF3,0x73,0x3C};
  // radio.setTarget(macRecv);
  radio.init();

  // You are able to change the Camera config E.g:
  // Camera.config.fb_count = 2;
  // Camera.config.frame_size = FRAMESIZE_HVGA;

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
