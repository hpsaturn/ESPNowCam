/**************************************************
 * ESP32Cam Freenove ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

// N O T E:
// -------
// Don't forget first install NanoPb library!
// and also review the README.md file.

#include <Arduino.h>
#include <ESPNowCam.h>
#include <drivers/CamFreenove.h>

CamFreenove Camera;
WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);
// ESPNowCam radio;

void processFrame() {
  if (Camera.get()) {
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(Camera.fb, 16, &out_jpg, &out_jpg_len); 
    radio.sendData(out_jpg, out_jpg_len);
    free(out_jpg);

    // radio.sendData(Camera.fb->buf, Camera.fb->len);
    // delay(10);  // ==> weird delay when you are using only DRAM.
    
    Camera.free();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(100);

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // M5Core2 receiver
  const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  // radio.init();
  radio.init(480);

  // You are able to change the Camera config E.g:
  Camera.config.fb_count = 2;
  Camera.config.frame_size = FRAMESIZE_QVGA;
  
  // Camera.config.pixel_format = PIXFORMAT_JPEG;
  // Camera.config.jpeg_quality = 85;
  // Camera.config.frame_size = FRAMESIZE_QVGA;
  // Camera.config.fb_count = 2;
  // Camera.config.fb_location = CAMERA_FB_IN_DRAM;

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  delay(100);
}

void loop() {
  processFrame();
}
