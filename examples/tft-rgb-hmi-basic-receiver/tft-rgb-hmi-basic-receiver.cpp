/**
 * Sample shared by @MeloCuentan
 * Board:
 * https://www.aliexpress.com/item/1005006625303218.html
 */

#include <Arduino.h>
#include <HardwareSerial.h>

#include "LGFX_ESP32S3_RGB_ESP32-8048S043.h"
#include <LGFX_TFT_eSPI.hpp>

#include "ESPNowCam.h"

static LGFX tft;

ESPNowCam radio;

// frame buffer
uint8_t *fb;

// display globals
int32_t dw = 800;
int32_t dh = 480;

void onDataReady(uint32_t lenght) { tft.drawJpg(fb, lenght, 0, 0, dw, dh); }

void setup(void) {
  tft.init();
  tft.setRotation(0);

  pinMode(2, OUTPUT);          // Display brightness enable
  analogWriteResolution(8);    // PWM resolution
  analogWriteFrequency(1000);  // PWM frequency
  analogWrite(2, 127);         // Brightness set

  tft.setTextColor(TFT_WHITE);

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = (uint8_t *)malloc(30000 * sizeof(uint8_t));

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    String text = "ESPNow Init Success";
    uint8_t textLenght = text.length();
    uint8_t textSize = 4;
    uint16_t posX = (800 - (textSize * 6 * textLenght)) / 2;
    uint16_t posY = ((480 - (textSize * 8)) / 3) * 2;
    tft.setTextSize(textSize);
    tft.drawString(text, posX, posY);
  }
  delay(500);
}

void loop(void) {}