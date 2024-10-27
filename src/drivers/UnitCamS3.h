#ifndef UNITCAMS3_H
#define UNITCAMS3_H

#include "CameraBase.hpp"
#include "esp_camera.h"

class UnitCamS3 : public CameraBase {
 public:
  using CameraBase::begin;
  using CameraBase::free;
  using CameraBase::get;

  UnitCamS3() {
    config.pin_reset = 21;
    config.pin_xclk = 11;
    config.pin_sccb_sda = 17;
    config.pin_sccb_scl = 41;
    config.pin_d7 = 13;
    config.pin_d6 = 4;
    config.pin_d5 = 10;
    config.pin_d4 = 5;
    config.pin_d3 = 7;
    config.pin_d2 = 16;
    config.pin_d1 = 15;
    config.pin_d0 = 6;
    config.pin_vsync = 42;
    config.pin_href = 18;
    config.pin_pclk = 12;
    config.xclk_freq_hz = 20000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    fb = nullptr;
    sensor = nullptr;
  }
};

#endif