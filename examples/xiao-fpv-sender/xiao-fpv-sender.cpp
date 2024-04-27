/**************************************************
 * ESPNowCam video Transmitter.
 * ----------------------------
 * 
 * XIAO FPV ESPNow transmitter uses some extra features of
 * this board to have some power consumption improvements 
 *
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include <Arduino.h>
#include <OneButton.h>
#include <ESPNowCam.h>
#include <drivers/CamXiao.h>
#include <Utils.h>

CamXiao Camera;
ESPNowCam radio;
OneButton button1(GPIO_NUM_0, true);

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

void shutdown() {
  Serial.println("shutdown..");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0);
  delay(1000);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  delay(1000); // only for debugging 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  
  radio.init();
  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
  }

  button1.attachClick([]() { shutdown(); });
  delay(100);
}

void loop() {
  processFrame();
  button1.tick();
}
