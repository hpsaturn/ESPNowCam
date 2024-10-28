/**************************************************
 * ESPNowCam video Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam tests project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <ESPNowCam.h>
#include "S3_Parallel16_ili9488.h"
#include <LGFX_TFT_eSPI.hpp>
#include "Utils.h"

ESPNowCam radio;
LGFX tft;

#define LCD_CS 37
#define LCD_BLK 45

// frame buffer
uint8_t *fb; 
// display globals
int32_t dw, dh;

static uint32_t frame_camera = 0;
static uint_fast64_t time_stamp_camera = 0;

static void print_FPS(int x, int y, const char *msg, uint32_t &frame, uint_fast64_t &time_stamp, uint32_t len) {
  frame++;
  if (millis() - time_stamp > 1000) {
    time_stamp = millis();
    char output[40];
    sprintf(output, "%s%2d FPS  JPG: %05d\r\n",msg, frame, len);
    tft.drawString(output, x, y);
    frame = 0;
    Serial.print(output);
  } 
}

void onDataReady(uint32_t lenght) {
  tft.drawJpg(fb, lenght , 0, 0, dw, dh);
  print_FPS(5, 250, "CAM:", frame_camera, time_stamp_camera, lenght);
}

void setup() {
  Serial.begin(115200);

  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_BLK, OUTPUT);

  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_BLK, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.startWrite();

  dw = tft.width();
  dh = tft.height();

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = static_cast<uint8_t*>(ps_malloc(30000 * sizeof(uint8_t)));

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    tft.setTextSize(2);
    tft.drawString("ESPNow Init Success", 5, 2);
  }
  delay(1000);
}

void loop() {
}
