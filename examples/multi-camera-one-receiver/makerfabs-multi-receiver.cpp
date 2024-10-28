/**************************************************
 * ESPNowCam Multi Camera Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
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

// frame buffers cameras:
uint8_t *fb_camera1; 
uint8_t *fb_camera2; 
uint8_t *fb_camera3;

// display globals
int32_t dw, dh;

// timestamp counters
static uint32_t frame_camera1 = 0;
static uint32_t frame_camera2 = 0;
static uint32_t frame_camera3 = 0;

static uint_fast64_t time_stamp_camera1 = 0;
static uint_fast64_t time_stamp_camera2 = 0;
static uint_fast64_t time_stamp_camera3 = 0;

static void print_FPS(int x, int y, const char *msg, uint32_t &frame, uint_fast64_t &time_stamp, uint32_t len) {
  frame++;
  if (millis() - time_stamp > 1000) {
    time_stamp = millis();
    char output[40];
    sprintf(output, "%s %2d FPS   JPG: %05d\r\n",msg, frame, len);
    tft.drawString(output, x, y);
    frame = 0;
    // Serial.print(output);
  } 
}

void onCamera1DataReady(uint32_t lenght) {
  tft.drawJpg(fb_camera1, lenght , 0, 0, dw, dh);  // FRAME_SIZE 320x240
  print_FPS(5, 250, "Camera1:", frame_camera1, time_stamp_camera1, lenght);
}

void onCamera2DataReady(uint32_t lenght) {
  tft.drawJpg(fb_camera2, lenght , 320, 0, dw, dh); // FRAME_SIZE 160x120
  print_FPS(5, 265, "Camera2:", frame_camera2, time_stamp_camera2, lenght);
}

void onCamera3DataReady(uint32_t lenght) {
  tft.drawJpg(fb_camera3, lenght , 320, 120, dw, dh); // FRAME_SIZE 160x120
  print_FPS(5, 280, "Camera3:", frame_camera3, time_stamp_camera3, lenght);
}

void setup() {
  Serial.begin(115200);

  delay(1000); // only for debugging 

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
  fb_camera1 = static_cast<uint8_t*>(ps_malloc(20000 * sizeof(uint8_t)));
  fb_camera2 = static_cast<uint8_t*>(ps_malloc(10000 * sizeof(uint8_t)));
  fb_camera3 = static_cast<uint8_t*>(ps_malloc(10000 * sizeof(uint8_t)));

  // M5CoreS3 Camera  f4:12:fa:85:f4:9c
  const uint8_t camera1[6] = {0xF4, 0x12, 0xFA, 0x85, 0xF4, 0x9C};
  // TJournal Camera  24:0a:c4:2f:8e:90
  const uint8_t camera2[6] = {0x24, 0x0A, 0xC4, 0x2F, 0x8E, 0x90};
  // XIAOSense Camera 74:4d:bd:81:4e:fc
  const uint8_t camera3[6] = {0x74, 0x4D, 0xBD, 0x81, 0x4E, 0xFC};

  radio.setRecvFilter(fb_camera1, camera1, onCamera1DataReady);
  radio.setRecvFilter(fb_camera2, camera2, onCamera2DataReady);
  radio.setRecvFilter(fb_camera3, camera3, onCamera3DataReady);

  if (radio.init()) {
    tft.drawString("ESPNow init Success", 0, 0);
  }
  tft.setTextFont(2);
  delay(1000);
}

void loop() {
}
