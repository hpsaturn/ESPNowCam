#ifndef CAMFREENOVEWR_H
#define CAMFREENOVEWR_H

#include "CameraBase.hpp"
#include "esp_camera.h"

class CamFreenoveWR : public CameraBase {
 public:
  using CameraBase::begin;
  using CameraBase::free;
  using CameraBase::get;

  CamFreenoveWR(){
    config.pin_reset = -1;
    config.pin_xclk = 21;
    config.pin_sccb_sda = 26;
    config.pin_sccb_scl = 27;
    config.pin_d7 = 35;
    config.pin_d6 = 34;
    config.pin_d5 = 39;
    config.pin_d4 = 36;
    config.pin_d3 = 19;
    config.pin_d2 = 18;
    config.pin_d1 = 5;
    config.pin_d0 = 4;
    config.pin_vsync = 25;
    config.pin_href = 23;
    config.pin_pclk = 22;
    config.xclk_freq_hz = 20000000;
    config.ledc_timer   = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format  = PIXFORMAT_RGB565;
    config.frame_size    = FRAMESIZE_QVGA;
    config.jpeg_quality  = 12;
    config.fb_count      = 2;
    config.fb_location   = CAMERA_FB_IN_PSRAM;
    config.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;
    fb = nullptr;
    sensor = nullptr;
  }
};

#endif