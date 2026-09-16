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
#include <common/DetectionsOverlay.h>
#include "ESPNowCam.h"
#include "Utils.h"

WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);
// ESPNowCam radio;

uint8_t *fb;    // frame buffer
int32_t dw, dh; // display width and height

uint32_t detections = 0;

void onDataReady(uint32_t lenght) {
  M5.Display.drawJpg(fb, lenght , 0, 0, dw, dh, 0, 0, 1.32, 1.0);
  printFPS("M5Core2:");
}

/// the AI results of the frame that was just painted (the sender attaches them
/// to the frame, see FrameMeta.h): the detection boxes are painted over the
/// image with the same rectangle used for it
void onMetaReady(const uint8_t *meta, uint32_t lenght) {
  detections = drawDetectionsOverlay(M5.Display, meta, lenght, 0, 0, dw, dh);
  FrameMetaReader reader(meta, lenght);
  static uint32_t last = 0;
  if (reader.valid() && (millis() - last) > 1000) {
    last = millis();
    Serial.printf("M5Core2: %u detection(s), inference %ums, %ux%u frame\r\n",
                  (unsigned int)reader.count(), (unsigned int)reader.perfInference(),
                  (unsigned int)reader.width(), (unsigned int)reader.height());
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(100);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(250);
  dw=M5.Display.width();
  dh=M5.Display.height();

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = static_cast<uint8_t*>(ps_malloc(40000 * sizeof(uint8_t)));

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);
  radio.setMetaCallback(onMetaReady);
  radio.setChannel(6);

  if (radio.init(960)) {  // chunk size in bytes (beta feature)
    M5.Display.drawString("ESPNowCam Init Success", dw / 2, dh / 2);
    Serial.println("ESPNowCam Init Success");
  } 
  delay(100);
}

void loop() {}
