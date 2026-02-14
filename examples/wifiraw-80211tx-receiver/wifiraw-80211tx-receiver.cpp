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

WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);
// ESPNowCam radio;

// frame buffer
uint8_t *fb; 
// display globals
int32_t dw, dh;

void onDataReady(uint32_t lenght) {
  M5.Display.drawJpg(fb, lenght , 0, 0, dw, dh);
  // M5.Display.pushImage(0, 0, dw, dh, (uint16_t *) fb);
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
  // fb = static_cast<uint8_t*>(ps_malloc(15000 * sizeof(uint8_t)));

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = (uint8_t*)  ps_malloc(310000 * sizeof( uint8_t ) ) ;



  // M5CoreS3 Camera  f4:12:fa:85:f4:9c
  // const uint8_t camera[6] = {0xF4, 0x12, 0xFA, 0x85, 0xF4, 0x9C};
  // Freenove Camera  C9:3F:01:00:00:00
  const uint8_t camera[6] = {0xC9, 0x3F, 0x01, 0x00, 0x00, 0x00};
  
  radio.setRecvBuffer(fb);
  // radio.setRecvFilter(fb, camera, onDataReady);
  radio.setRecvCallback(onDataReady);

  // if (radio.init()) {
  if (radio.init(512)) {
    M5.Display.drawString("ESPNow Init Success", dw / 2, dh / 2);
    Serial.println("ESPNow Init Success");
  } 
  delay(500);
}

void loop() {
}
