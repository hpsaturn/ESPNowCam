#ifndef CAMFREENOVE_H
#define CAMFREENOVE_H

#include "esp_camera.h"
#include "camera_pins.h"

class CamFreenove {
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