#include <EspNowJoystick.hpp>
#include "CamFreenove.h"
#include "esp_camera.h"


CamFreenove Camera;
EspNowJoystick joystick;
JoystickMessage jm;

int32_t dw, dh;

// #define CONVERT_TO_JPEG


uint16_t frame = 0;

void printFPS(const char *msg) {
  static uint_least64_t timeStamp = 0;
  frame++;
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();
    Serial.printf("%s %2d FPS\r\n",msg, frame);
    frame = 0;
  } 
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);

  // if (!Camera.begin()) {
  //   Serial.println("Camera Init Fail");
  // }
  // Camera.sensor->set_framesize(Camera.sensor, FRAMESIZE_QVGA); 

  // if(psramFound()){
  //   size_t psram_size = esp_spiram_get_size() / 1048576;
  //   Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  // }

  jm = joystick.newJoystickMsg();
  joystick.init();
}

uint8_t targetMac[6] = {0x34, 0x85, 0x18, 0x8D, 0x7F, 0x64};

void loop() {
//   if (Camera.get()) {
// #ifdef CONVERT_TO_JPEG
//     uint8_t *out_jpg = NULL;
//     size_t out_jpg_len = 0;
//     frame2jpg(Camera.fb, 64, &out_jpg, &out_jpg_len);
//     // Serial.printf("jpg data size: %i\r\n",out_jpg_len);
//     jm.az = sizeof(Camera.fb);
//     jm.ax = out_jpg_len;
//     joystick.sendJoystickMsg(jm, targetMac);
//     printFPS("JPG compression at");
//     // CoreS3.Display.drawJpg(out_jpg, out_jpg_len, 0, 0, dw, dh);
//     free(out_jpg);
// #else
//     jm.az = Camera.fb->len;
//     joystick.sendJoystickMsg(jm, targetMac);
//     // CoreS3.Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
//     printFPS("frame ready at");
// #endif
//     Camera.free();
//   }
    jm.az = 10000;
    jm.ax = 150000;
    // joystick.sendJoystickMsg(jm, targetMac);
    if (joystick.sendJoystickMsg(jm, targetMac)) printFPS("ESPNow sending at:");
}
