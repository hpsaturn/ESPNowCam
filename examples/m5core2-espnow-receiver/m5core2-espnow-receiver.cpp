/**************************************************
 * ESPNowCam video Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <M5Unified.h>
#include "ESPNowCam.h"
#include "Utils.h"

ESPNowCam radio;

// frame buffer
uint8_t *fb; 
// display globals
int32_t dw, dh;

void onDataReady(uint32_t lenght) {
  M5.Display.drawJpg(fb, lenght , 0, 0, dw, dh);
  printFPS("M5Core2:");
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(96);
  dw=M5.Display.width();
  dh=M5.Display.height();

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = static_cast<uint8_t*>(ps_malloc(15000 * sizeof(uint8_t)));

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    M5.Display.drawString("ESPNow Init Success", dw / 2, dh / 2);
    Serial.println("ESPNow Init Success");
  } 
  delay(500);
}

void loop() {
}
