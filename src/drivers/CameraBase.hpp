#include "esp_camera.h"

class CameraBase {
 private:
 public:
  camera_fb_t* fb;
  sensor_t* sensor;
  camera_config_t config;

  bool begin() {
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
      return false;
    }
    sensor = esp_camera_sensor_get();
    return true;
  }

  bool get() {
    fb = esp_camera_fb_get();
    if (!fb) {
      return false;
    }
    return true;
  }

  bool free() {
    if (fb) {
      esp_camera_fb_return(fb);
      return true;
    }
    return false;
  }
};
