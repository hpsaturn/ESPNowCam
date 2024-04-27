#ifndef CAMTJOURNAL_H
#define CAMTJOURNAL_H

#include "esp_camera.h"

class CamTJournal {
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