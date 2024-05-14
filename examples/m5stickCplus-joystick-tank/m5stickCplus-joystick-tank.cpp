/**************************************************
 * ESPNowCam Joystick Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include "ESPNowCam.h"
#include <common/comm.pb.h>

ESPNowCam radio;

extern const unsigned char connect_on[800];
extern const unsigned char connect_off[800];

uint16_t AngleBuff[4];

bool receiverConnected = false;
float receiverBattVolt = 0.0;
uint16_t receiverBattLevel = 0;
uint_least32_t heartBeatStamp = 0;

uint8_t macAdd [6] = {0x34, 0x85, 0x18, 0x6F, 0x23, 0xF4};

M5Canvas disp;
M5Canvas list;

int w;
int h;
int ix = 21;
int fh;

bool mode = false;
uint32_t modeRestoreCount = 0;
uint32_t btnDebounce = 0;
uint32_t suspendCount = 0;

JoystickMessage jm = JoystickMessage_init_zero;

uint8_t I2CRead8bit(uint8_t Addr) {
  Wire.beginTransmission(0x38);
  Wire.write(Addr);
  Wire.endTransmission();
  Wire.requestFrom(0x38, 1);
  return Wire.read();
}

uint16_t I2CRead16bit(uint8_t Addr) {
  uint16_t ReData = 0;
  Wire.beginTransmission(0x38);
  Wire.write(Addr);
  Wire.endTransmission();
  Wire.requestFrom(0x38, 2);
  for (int i = 0; i < 2; i++) {
    ReData <<= 8;
    ReData |= Wire.read();
  }
  return ReData;
}

void drawJoystickValues(uint8_t ax, uint8_t ay, uint8_t az) {
  disp.fillRect(0, ix, w, ix + fh * 10, BLACK);
  disp.setTextColor(BLUE);
  disp.setCursor(10, ix -5 + fh * 1);
  disp.printf("JOYSTICK:");
  disp.setTextColor(WHITE);
  disp.setCursor(10, ix + fh * 2);
  disp.printf("AX: %04d", ax);
  disp.setCursor(10, ix + fh * 3);
  disp.printf("AY: %04d", ay);
  disp.setCursor(10, ix + fh * 4);
  disp.printf("AZ: %04d", az);
  disp.setCursor(10, ix + fh * 5);
  disp.printf("BV: %04d", M5.Power.getBatteryVoltage());
  disp.setCursor(10, ix - 5 + fh * 7);
  disp.setTextColor(MAGENTA);
  disp.printf("RECEIVER:");
  disp.setTextColor(WHITE);
  disp.setCursor(10, ix + fh * 8);
  disp.printf("BL: %03d%%", receiverBattLevel);
  disp.setCursor(10, ix + fh * 9);
  disp.printf("BV: %.2fv", receiverBattVolt);
}

void updateDisplay(uint8_t ax, uint8_t ay, uint8_t az) {
  static uint_least32_t guiTimeStamp = 0;
  if (millis() - guiTimeStamp > 100) {
    guiTimeStamp = millis();

    // update connection icon status
    if (receiverConnected)
      disp.pushImage(0, 0, 20, 20, (uint16_t *)connect_on);
    else
      disp.pushImage(0, 0, 20, 20, (uint16_t *)connect_off);
    disp.pushSprite(&M5.Display, 0, 0);

    drawJoystickValues(ax, ay, az);
 
    M5.update();

    // device connection watchdog
    if (millis() - heartBeatStamp > 1000) {
      receiverConnected = false;  // heartbeat should be renew this flag
    }
  }
}

void setup() {
  auto cfg = M5.config();

  M5.begin(cfg);
  M5.Display.setBrightness(96);
  w=M5.Display.width();
  h=M5.Display.height();
  disp.createSprite(w,h);
  disp.fillRect(0, 0, w, 20, disp.color565(50, 50, 50));
  disp.pushImage(0, 0, 20, 20, (uint16_t *)connect_on);
  disp.pushSprite(&M5.Display,0,0);

  Wire.begin(0, 26);
  uint8_t res = I2CRead8bit(0x32);
  Serial.printf("Res0 = %02X \r\n", res);

  M5.update();

  if (M5.getBoard() == m5::board_t::board_M5StickC) {
    disp.setTextSize(1);
    fh = 12;
  }
  else {
    disp.setTextSize(2);
    fh = 19;
  }

  radio.setTarget(macAdd);
  radio.init(244);

  disp.setTextColor(WHITE);
}

uint8_t bf[256];

void sendMsg() {
  pb_ostream_t stream = pb_ostream_from_buffer(bf, sizeof(bf));
  bool status = pb_encode(&stream, JoystickMessage_fields, &jm);
  size_t message_length = stream.bytes_written;
  if (!status) {
    printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
    return;
  }
  radio.sendData(bf, message_length);
  delay(10);
}

// sticks parameters
const uint8_t center = 100;
const uint8_t deathBand = 10;
static uint_least32_t cTStamp = 0;

void readValues() {
  for (int i = 0; i < 4; i++) {
    AngleBuff[i] = I2CRead16bit(0x50 + i * 2);
  }

  uint8_t ck = 0x01; // default command
  uint8_t ax = map(AngleBuff[0], 0, 4000, 0, 200);
  uint8_t ay = map(AngleBuff[1], 0, 4000, 0, 200);
  uint8_t az = map(AngleBuff[2], 0, 4000, 0, 200);

  jm.ay = ay;
  jm.ax = ax;
  jm.az = az;
  jm.ck = ck;

  if ((abs(ax - center) < deathBand) && (abs(ay - center) < deathBand) && (abs(az - center) < deathBand)) {
    // on death band, wait for improve power consumption
    if (millis() - cTStamp > 100) {
      cTStamp = millis();
      sendMsg();
    }
  }
  else
    sendMsg();
  
  updateDisplay(ax, ay, az);
}

void loop() {
  // auto power off if receiver is not connected
  // if (suspendCount++ > 3000) {
  //   Serial.println("not receiver detected. Turn off..");
  //   Serial.println("Releasing receivers:");
  //   joystick.printReceivers();
  //   delay(1000);
  //   M5.Power.powerOff();
  // }

  if (M5.BtnB.wasClicked() && btnDebounce++ > 15) {
    delay(100);
    M5.Power.powerOff();
  }

  // if (M5.BtnA.wasClicked() && btnDebounce++ > 25) {
  //   // Serial.printf("mode: %s\r\n", mode ? "stats" : "receivers");
  //   modeRestoreCount = 0;
  //   btnDebounce = 0;
  // }

  readValues();
}
