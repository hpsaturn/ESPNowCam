#ifndef CAMXIAO_H
#define CAMXIAO_H

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "esp_camera.h"
#include "xiao_pins.h"

class CamXiao {
   private:
   public:
    camera_fb_t* fb;
    sensor_t* sensor;
    camera_config_t* config;
    bool begin();
    bool get();
    bool free();
};

#endif