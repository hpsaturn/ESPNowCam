/**************************************************
 * ESPNowCam video Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam tests project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <ESPNowCam.h>
#include "lgfx-esp32c3-crowpanel.h"
#include <LGFX_TFT_eSPI.hpp>
#include "Utils.h"

ESPNowCam radio;
LGFX tft;

#define I2C_SDA 4
#define I2C_SCL 5
#define TP_INT 0
#define TP_RST -1
#define LCD_CS 10
#define LCD_BLK -1
#define PI4IO_I2C_ADDR 0x43

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

//Extended IO function
void init_IO_extender() {
  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x01); // test register
  Wire.endTransmission();
  Wire.requestFrom(PI4IO_I2C_ADDR, 1);
  uint8_t rxdata = Wire.read();
  Serial.print("Device ID: ");
  Serial.println(rxdata, HEX);

  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x03); // IO direction register
  Wire.write((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)); // set pins 0, 1, 2 as outputs
  Wire.endTransmission();
  
  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x07); // Output Hi-Z register
  Wire.write(~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4))); // set pins 0, 1, 2 low
  Wire.endTransmission();
}

void set_pin_io(uint8_t pin_number, bool value) {

  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x05); // test register
  Wire.endTransmission();
  Wire.requestFrom(PI4IO_I2C_ADDR, 1);
  uint8_t rxdata = Wire.read();
  Serial.print("Before the change: ");
  Serial.println(rxdata, HEX);

  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x05); // Output register

  if (!value)
    Wire.write((~(1 << pin_number)) & rxdata); // set pin low
  else
    Wire.write((1 << pin_number) | rxdata); // set pin high
  Wire.endTransmission();
  
  Wire.beginTransmission(PI4IO_I2C_ADDR);
  Wire.write(0x05); // test register
  Wire.endTransmission();
  Wire.requestFrom(PI4IO_I2C_ADDR, 1);
  rxdata = Wire.read();
  Serial.print("after the change: ");
  Serial.println(rxdata, HEX);
}


void setup() {
  Serial.begin(115200);

  Wire.begin(4, 5);
  init_IO_extender();
  delay(100);
  set_pin_io(3, true);
  set_pin_io(4, true);
  set_pin_io(2, true);

  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  // ticker.attach(1, tcr1s);
  tft.init();
  tft.initDMA();
  tft.startWrite();
  tft.setColor(0, 0, 0);

  tft.fillScreen(TFT_BLACK);

  dw = tft.width();
  dh = tft.height();

  // if(psramFound()){
  //   size_t psram_size = esp_spiram_get_size() / 1048576;
  //   Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  // }

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  fb = static_cast<uint8_t*>(malloc(10000 * sizeof(uint8_t)));

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
