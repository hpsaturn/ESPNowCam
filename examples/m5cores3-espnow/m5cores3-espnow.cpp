#include <EspNowJoystick.hpp>

#include "M5CoreS3.h"
#include "esp_camera.h"

EspNowJoystick joystick;
JoystickMessage jm;

int32_t dw, dh;

#define CONVERT_TO_JPEG

void setup() {
  Serial.begin(115200);
   
  auto cfg = M5.config();
  CoreS3.begin(cfg);
  CoreS3.Display.setTextColor(GREEN);
  CoreS3.Display.setTextDatum(middle_center);
  CoreS3.Display.setFont(&fonts::Orbitron_Light_24);
  CoreS3.Display.setTextSize(1);

  dw = CoreS3.Display.width();
  dh = CoreS3.Display.height();

  if (!CoreS3.Camera.begin()) {
    CoreS3.Display.drawString("Camera Init Fail", dw / 2, dh / 2);
  }
  CoreS3.Display.drawString("Camera Init Success", dw / 2, dh / 2);
  CoreS3.Camera.sensor->set_framesize(CoreS3.Camera.sensor, FRAMESIZE_QVGA);
 
  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  jm = joystick.newJoystickMsg();
  joystick.init();
}

void loop() {
  if (CoreS3.Camera.get()) {
#ifdef CONVERT_TO_JPEG
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(CoreS3.Camera.fb, 32, &out_jpg, &out_jpg_len);
    // Serial.printf("jpg data size: %i\r\n",out_jpg_len);
    jm.az = sizeof(CoreS3.Camera.fb);
    jm.ax = out_jpg_len;
    jm.ay = CoreS3.Display.height();
    joystick.sendJoystickMsg(jm);
    CoreS3.Display.drawJpg(out_jpg, out_jpg_len, 0, 0, dw, dh);
    free(out_jpg);
#else
    jm.az = sizeof(CoreS3.Camera.fb->buf);
    jm.ax = CoreS3.Display.width();
    jm.ay = CoreS3.Display.height();
    joystick.sendJoystickMsg(jm);
    CoreS3.Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
#endif
    CoreS3.Camera.free();
  }
}
