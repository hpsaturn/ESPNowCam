/**************************************************
 * ESPNowCam transmitter using WiFi Raw frames (802.11 Tx)
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <M5CoreS3.h>
#include "esp_camera.h"
#include "ESPNowCam.h"
#include "Utils.h"

WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);

int32_t dw, dh;

static void drawFPS() {
  static uint_least64_t timeStamp = 0;
  frame++;
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();
    char bf [10];
    sprintf(bf,"FPS: %i",frame);
    CoreS3.Display.clear();
    CoreS3.Display.drawString(String(bf), dw / 2, dh / 2);
    frame = 0;
  } 
}

void processFrame() {
  if (CoreS3.Camera.get()) {
    CoreS3.Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(CoreS3.Camera.fb, 24, &out_jpg, &out_jpg_len);
    radio.sendData(out_jpg, out_jpg_len);
    printFPS("CAM:");
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

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  radio.setChannel(6);
  
  if (radio.init(960)) {  // chunk size in bytes (beta feature)
    Serial.println("ESPNowCam Init Success");
  }

  dw = CoreS3.Display.width();
  dh = CoreS3.Display.height();

  if (!CoreS3.Camera.begin()) {
    CoreS3.Display.drawString("Camera Init Fail", dw / 2, dh / 2);
  }
  CoreS3.Display.drawString("Camera Init Success", dw / 2, dh / 2);

  CoreS3.Camera.sensor->set_framesize(CoreS3.Camera.sensor, FRAMESIZE_QVGA);

  delay(100);
}

void loop() {
  processFrame();
}
