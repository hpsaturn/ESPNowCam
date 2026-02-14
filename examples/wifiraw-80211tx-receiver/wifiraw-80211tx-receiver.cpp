/**************************************************
 * ESPNowCam receiver using WiFi Raw frames (802.11 Tx)
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

// N O T E:
// -------
// Don't forget first install NanoPb library!
// and also review the README.md file.

#include <Arduino.h>
#include <M5Unified.h>
#include "ESPNowCam.h"
#include "Utils.h"

WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);

uint8_t *fb;    // frame buffer
int32_t dw, dh; // display width and height

void onDataReady(uint32_t lenght) {
  M5.Display.drawJpg(fb, lenght , 0, 0, dw, dh);
  printFPS("M5Core2:");
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(100);

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

  if (radio.init(480)) {  // chunk size in bytes (beta feature)
    M5.Display.drawString("ESPNowCam Init Success", dw / 2, dh / 2);
    Serial.println("ESPNowCam Init Success");
  } 
  delay(100);
}

void loop() {}
