#ifndef UNITCAMS3_H
#define UNITCAMS3_H

#include "esp_camera.h"

class UnitCamS3 {
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