/**************************************************
 * ESP32Cam Freenove ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

// N O T E:
// -------
// Don't forget first install NanoPb library!
// and also review the README.md file.

#include <Arduino.h>
#include <drivers/CamFreenove.h>
#include "ESPNowCam.h"
#include "Utils.h"

CamFreenove Camera;
ESPNowCam radio;

void processFrame() {
  if (Camera.get()) {
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(Camera.fb, 12, &out_jpg, &out_jpg_len);
    radio.sendData(out_jpg, out_jpg_len);
    free(out_jpg);
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

  // uint8_t macRecv[6] = {0xB8,0xFF,0x09,0xC6,0x0E,0xCC};
  // radio.setTarget(macRecv);
  radio.init();
  
  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  delay(500);
}

void loop() {
  processFrame();
}
