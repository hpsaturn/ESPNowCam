#ifndef CAMTJOURNAL_H
#define CAMTJOURNAL_H

#include "CameraBase.hpp"
#include "esp_camera.h"

class CamTJournal : public CameraBase {
 public:
  using CameraBase::begin;
  using CameraBase::free;
  using CameraBase::get;

  CamTJournal(){
    config.pin_reset = 15;
    config.pin_xclk = 27;
    config.pin_sccb_sda = 25;
    config.pin_sccb_scl = 23;
    config.pin_d7 = 19;
    config.pin_d6 = 36;
    config.pin_d5 = 18;
    config.pin_d4 = 39;
    config.pin_d3 = 5;
    config.pin_d2 = 34;
    config.pin_d1 = 35;
    config.pin_d0 = 17;
    config.pin_vsync = 22;
    config.pin_href = 26;
    config.pin_pclk = 21;
    config.xclk_freq_hz = 20000000;
    config.ledc_timer   = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format  = PIXFORMAT_JPEG;
    config.frame_size    = FRAMESIZE_QVGA;
    config.jpeg_quality  = 12;
    config.fb_count      = 1;
    config.fb_location   = CAMERA_FB_IN_DRAM;
    config.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;
    fb = nullptr;
    sensor = nullptr;
  }
};

#endif