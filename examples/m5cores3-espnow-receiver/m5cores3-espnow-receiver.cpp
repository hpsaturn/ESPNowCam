/**************************************************
 * ESPNowCam video Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <M5CoreS3.h>
#include "ESPNowCam.h"

ESPNowCam radio;

// frame buffer
uint8_t *fb; 
// display globals
int32_t dw, dh;

static uint32_t frame_camera1 = 0;
static uint_fast64_t time_stamp_camera1 = 0;

static void print_FPS(int x, int y, const char *msg, uint32_t &frame, uint_fast64_t &time_stamp, uint32_t len) {
  frame++;
  if (millis() - time_stamp > 1000) {
    time_stamp = millis();
    char output[40];
    sprintf(output, "%s %2d FPS   JPG: %05d\r\n", msg, frame, len);
    // M5.Display.drawString(output, x, y);
    frame = 0;
    Serial.print(output);
  } 
}

void onDataReady(uint32_t lenght) {
  CoreS3.Display.drawJpg(fb, lenght , 0, 0, dw, dh);
  print_FPS(0,0,"cam1",frame_camera1,time_stamp_camera1,lenght);
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

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = static_cast<uint8_t *>(ps_malloc(5000 * sizeof(uint8_t)));

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    M5.Display.drawString("ESPNow Init Success", dw / 2, dh / 2);
  }
  delay(1000);
}

void loop() {
}
