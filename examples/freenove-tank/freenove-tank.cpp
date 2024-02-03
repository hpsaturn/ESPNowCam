/**************************************************
 * ESP32Cam Freenove ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include <Arduino.h>
#include "CamFreenove.h"
#include "ESPNowCam.h"
#include "comm.pb.h"
#include "Utils.h"

CamFreenove Camera;
ESPNowCam radio;

// buffer for control commands from joystick
uint8_t *recv_buff;
// Joystick meessage for store the commands
JoystickMessage jm = JoystickMessage_init_zero;

// main method to frames send
void processFrame() {
  if (Camera.get()) {
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(Camera.fb, 12, &out_jpg, &out_jpg_len);
    radio.sendData(out_jpg, out_jpg_len);
    free(out_jpg);
    Camera.free();
    // printFPS("CAM:");
  }
}

bool decodeMsg(uint16_t message_length) {
  pb_istream_t stream = pb_istream_from_buffer(recv_buff, message_length);
  bool status = pb_decode(&stream, JoystickMessage_fields, &jm);
  if (!status) {
    Serial.printf("Decoding msg failed: %s\r\n", PB_GET_ERROR(&stream));
    return false;
  }
  Serial.printf("cmds: ax:%i ay:%i az:%i\r\n",jm.ax, jm.ay, jm.az);
  return true;
}

// control commands data
void onDataReady(uint32_t lenght) {
  // Serial.printf("onDataReady: len %i\r\n",lenght);
  decodeMsg(lenght);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  
  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  recv_buff = (uint8_t*)  ps_malloc(100* sizeof( uint8_t ) ) ;
  radio.setRecvBuffer(recv_buff);
  radio.setRecvCallback(onDataReady);

  uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  radio.init(244);
  
  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  delay(500);
}

void loop() {
  processFrame();
}
