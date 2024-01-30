#include "CamFreenove.h"

static camera_config_t camera_config = {
    .pin_pwdn     = -1,
    .pin_reset    = -1,
    .pin_xclk     = 15,
    .pin_sscb_sda = 4,
    .pin_sscb_scl = 5,
    .pin_d7       = 16,
    .pin_d6       = 17,
    .pin_d5       = 18,
    .pin_d4       = 12,
    .pin_d3       = 10,
    .pin_d2       = 8,
    .pin_d1       = 9,
    .pin_d0       = 11,

    .pin_vsync = 6,
    .pin_href  = 7,
    .pin_pclk  = 13,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format  = PIXFORMAT_RGB565,
    .frame_size    = FRAMESIZE_HVGA,
    .jpeg_quality  = 0,
    .fb_count      = 2,
    .fb_location   = CAMERA_FB_IN_PSRAM,
    .grab_mode     = CAMERA_GRAB_WHEN_EMPTY
    // .grab_mode     = CAMERA_GRAB_LATEST
    // .sccb_i2c_port = M5.In_I2C.getPort(),
};

bool CamFreenove::begin() {
  
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    return false;
  }
  sensor = esp_camera_sensor_get();
  return true;
}

bool CamFreenove::get() {
    fb = esp_camera_fb_get();
    if (!fb) {
        return false;
    }
    return true;
}

bool CamFreenove::free() {
    if (fb) {
        esp_camera_fb_return(fb);
        return true;
    }
    return false;
}
