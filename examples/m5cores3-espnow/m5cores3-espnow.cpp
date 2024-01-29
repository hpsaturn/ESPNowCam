/**************************************************
 * ESP32Cam ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include <M5CoreS3.h>
#include "esp_camera.h"
#include "ESPNowSender.h"
#include "Utils.h"

ESPNowSender radio;
int32_t dw, dh;

void processFrame() {
  if (CoreS3.Camera.get()) {
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(CoreS3.Camera.fb, 12, &out_jpg, &out_jpg_len);
    CoreS3.Display.drawJpg(out_jpg, out_jpg_len, 0, 0, dw, dh);
    radio.sendData(out_jpg, out_jpg_len);
    // printFPS("CAM:");
    free(out_jpg);
    CoreS3.Camera.free();
  }
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  CoreS3.begin(cfg);
  CoreS3.Display.setTextColor(GREEN);
  CoreS3.Display.setTextDatum(middle_center);
  CoreS3.Display.setFont(&fonts::Orbitron_Light_24);
  CoreS3.Display.setTextSize(1);

  dw = CoreS3.Display.width();
  dh = CoreS3.Display.height();

  if (!CoreS3.Camera.begin()) {
    CoreS3.Display.drawString("Camera Init Fail", dw / 2, dh / 2);
  }
  CoreS3.Display.drawString("Camera Init Success", dw / 2, dh / 2);
  CoreS3.Camera.sensor->set_framesize(CoreS3.Camera.sensor, FRAMESIZE_QVGA);

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  radio.init();
  delay(500);
}

void loop() {
  processFrame();
}
