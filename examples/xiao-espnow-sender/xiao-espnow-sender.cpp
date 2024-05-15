/**************************************************
 * ESPNowCam video Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <ESPNowCam.h>
#include <drivers/CamXiao.h>
#include <Utils.h>

CamXiao Camera;
ESPNowCam radio;

void processFrame() {
  if (Camera.get()) {
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(Camera.fb, 12, &out_jpg, &out_jpg_len);
    radio.sendData(out_jpg, out_jpg_len);
    printFPS("CAM:");
    free(out_jpg);
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
  
  if (!radio.init()) {
    Serial.println("Radio Init Fail");
    delay(1000);
  }

  // You are able to change the Camera config E.g:
  // Camera.config.fb_count = 2;
  // Camera.config.frame_size = FRAMESIZE_QQVGA;

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
