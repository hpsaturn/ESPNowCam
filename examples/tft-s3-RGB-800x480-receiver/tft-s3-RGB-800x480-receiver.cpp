/**************************************************
 * ESPNowCam video Receiver
 * @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam tests project:
 * https://github.com/hpsaturn/ESPNowCam
 * 
 * This code was submitted by @MeloCuentan
 * and tested on a real board around the next
 * issue:
 * https://github.com/hpsaturn/ESPNowCam/issues/53
**************************************************/

#include <Arduino.h>
#include <HardwareSerial.h>
#include "LGFX_ESP32S3_RGB_ESP32-8048S043.h"

#include <LGFX_TFT_eSPI.hpp>

#include "ESPNowCam.h"

const uint8_t pin_brightness = 2;

static LGFX tft;

ESPNowCam radio;

// frame buffer
uint8_t fb[60000];

// display globals
int32_t dw = 800;
int32_t dh = 480;

void onDataReady(uint32_t lenght) { tft.drawJpg(fb, lenght, 0, 0, dw, dh); }

void setup(void) {
  tft.init();
  tft.setRotation(0);

  pinMode(pin_brightness, OUTPUT);
  analogWriteResolution(8);
  analogWriteFrequency(1000);
  analogWrite(pin_brightness, 127);

  tft.setTextColor(TFT_WHITE);

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  // fb = (uint8_t *)ps_malloc(30000 * sizeof(uint8_t));

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    String text = "ESPNow Init Success";
    uint8_t text_length = text.length();
    uint8_t text_size = 4;
    uint16_t posX = (800 - (text_size * 6 * text_length)) / 2;
    uint16_t posY = ((480 - (text_size * 8)) / 3) * 2;
    tft.setTextSize(text_size);
    tft.drawString(text, posX, posY);
  }
  delay(500);
}

void loop(void) {}