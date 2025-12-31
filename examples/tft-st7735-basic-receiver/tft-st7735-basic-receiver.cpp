/**
 * Sample shared by @turmandreams
 * Board: M5Stack Basic Core
 */

#include <Arduino.h>
#include <ESPNowCam.h>
#include "lgfx_custom_st7735_conf.hpp"
#include <LGFX_TFT_eSPI.hpp>
// #include <Utils.h>

ESPNowCam radio;

static TFT_eSPI lcd;              // Instance of LGFX
static TFT_eSprite sprite(&lcd);  // Instance of LGFX_Sprite when using sprites

// frame buffer
uint8_t fb[60000];
// display globals
int32_t dw = 160;
int32_t dh = 128;

void onDataReady(uint32_t lenght) {
  lcd.drawJpg(fb, lenght, 0, 0, dw, dh);
  // printFPS("MF:");
}

void setup(void) {
  Serial.begin(115200);

  pinMode(32, OUTPUT);    // Back Light
  digitalWrite(32, HIGH);

  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(128);
  lcd.setColorDepth(16);

  lcd.drawRect(0, 0, 160, 128, (uint16_t)0x1F);  // green
  delay(2000);

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);

  if (radio.init()) {
    lcd.drawString("ESPNow Init Success", 25, dh / 2);
    Serial.println("ESPNow Init Success");
  }
  delay(500);
}

void loop(void) {}
