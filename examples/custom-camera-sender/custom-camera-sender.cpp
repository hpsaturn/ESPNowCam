/**************************************************
 * ESPNowCam video Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <esp_camera.h>
#include <ESPNowCam.h>
#include <Utils.h>

ESPNowCam radio;
camera_fb_t* fb;

bool has_psram = false;

// !! N O T E !! 
// Please change this for your Camera pins if your camera
// is not listed in the examples or driver directory.

// If your new config works and it is not included in the
// drivers section, please notify it via an issue or please
// contribute to the proyect with a pull request. Thanks!

camera_config_t camera_config = {
    .pin_pwdn = -1,
    .pin_reset = 15,
    .pin_xclk = 27,
    .pin_sscb_sda = 25,
    .pin_sscb_scl = 23,
    .pin_d7 = 19,
    .pin_d6 = 36,
    .pin_d5 = 18,
    .pin_d4 = 39,
    .pin_d3 = 5,
    .pin_d2 = 34,
    .pin_d1 = 35,
    .pin_d0 = 17,
    .pin_vsync = 22,
    .pin_href = 26,
    .pin_pclk = 21,
    
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format  = PIXFORMAT_JPEG,
    .frame_size    = FRAMESIZE_QVGA,
    .jpeg_quality  = 12,
    .fb_count      = 1,
    .fb_location   = CAMERA_FB_IN_DRAM,
    .grab_mode     = CAMERA_GRAB_WHEN_EMPTY,
};

bool CameraBegin() {
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    return false;
  }
  return true;
}

bool CameraGet() {
  fb = esp_camera_fb_get();
  if (!fb) {
    return false;
  }
  return true;
}

bool CameraFree() {
  if (fb) {
    esp_camera_fb_return(fb);
    return true;
  }
  return false;
}

void processFrame() {
  if (CameraGet()) {
    if (has_psram) {
      uint8_t *out_jpg = NULL;
      size_t out_jpg_len = 0;
      frame2jpg(fb, 12, &out_jpg, &out_jpg_len);
      radio.sendData(out_jpg, out_jpg_len);
      free(out_jpg);
    }
    else{
      radio.sendData(fb->buf, fb->len);
      delay(30); // ==> weird delay for cameras without PSRAM
    }
    printFPS("CAM:");
    CameraFree();
  }
}

void setup() {
  Serial.begin(115200);

  delay(1000); // only for debugging 

  if(psramFound()){
    has_psram = true;
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
    // suggested config with PSRAM
    camera_config.pixel_format = PIXFORMAT_RGB565;
    camera_config.fb_location = CAMERA_FB_IN_PSRAM;
    camera_config.fb_count = 2;
  }
  else{
    Serial.println("PSRAM not found! Basic framebuffer setup.");
  }
  
  radio.init();

  if (!CameraBegin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
    ESP.restart();
  }
  delay(500);
}

void loop() {
  processFrame();
}
