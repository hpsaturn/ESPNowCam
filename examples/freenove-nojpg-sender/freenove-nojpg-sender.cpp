/**************************************************
 * ESPNowCam Transmitter without JPG compression (very slow)
 * Use with: makerfabs-nojpg-receiver example
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include "CamFreenove.h"
#include "ESPNowCam.h"
#include "Utils.h"

CamFreenove Camera;
ESPNowCam radio;

void processFrame() {
  if (Camera.get()) {
    radio.sendData(Camera.fb->buf,Camera.fb->len);
    Camera.free();
    // printFPS("CAM:");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  
  radio.init();
  
  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  delay(500);
}

void loop() {
  processFrame();
}
