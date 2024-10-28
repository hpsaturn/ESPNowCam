/**************************************************
 * ESPNowCam receiver without JPG compression (very slow)
 *
 * Use with: freenove-nojpg-sender example
 *  
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam tests project:
 * https://github.com/hpsaturn/ESPNowCam.git
**************************************************/

#include <Arduino.h>
#include "S3_Parallel16_ili9488.h"
#include <LGFX_TFT_eSPI.hpp>
#include "ESPNowCam.h"
#include "Utils.h"

ESPNowCam radio;
LGFX tft;

#define LCD_CS 37
#define LCD_BLK 45


// frame buffer
uint8_t *fb; 
// display globals
int32_t dw, dh;

void onDataReady(uint32_t lenght) {
  tft.pushImage(0, 0, dw, dh, (uint16_t *) fb);
  printFPS("MF:");
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
  fb = (uint8_t*)  ps_malloc(310000 * sizeof( uint8_t ) ) ;

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    tft.drawString("ESPNow Init Success", dw / 2, dh / 2);
  }
  delay(1000);
}

void loop() {
}
