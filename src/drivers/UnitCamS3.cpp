#include "UnitCamS3.h"

static camera_config_t camera_config = {
    .pin_reset = 21,
    .pin_xclk = 11, 
    .pin_sscb_sda = 17,
    .pin_sscb_scl = 41,
    .pin_d7 = 13,   
    .pin_d6 = 4,    
    .pin_d5 = 10,   
    .pin_d4 = 5,    
    .pin_d3 = 7,    
    .pin_d2 = 16,   
    .pin_d1 = 15,   
    .pin_d0 = 6,    
    .pin_vsync = 42,
    .pin_href = 18, 
    .pin_pclk = 12, 
    
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format  = PIXFORMAT_RGB565,
    .frame_size    = FRAMESIZE_QVGA,
    .jpeg_quality  = 12,
    .fb_count      = 2,
    .fb_location   = CAMERA_FB_IN_PSRAM,
    .grab_mode     = CAMERA_GRAB_WHEN_EMPTY,
};

bool UnitCamS3::begin() {
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    return false;
  }
  sensor = esp_camera_sensor_get();
  return true;
}

bool UnitCamS3::get() {
  fb = esp_camera_fb_get();
  if (!fb) {
    return false;
  }
  return true;
}

bool UnitCamS3::free() {
  if (fb) {
    esp_camera_fb_return(fb);
    return true;
  }
  return false;
}
