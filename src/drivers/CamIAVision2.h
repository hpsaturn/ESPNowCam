#ifndef CAMAIVISION_H
#define CAMAIVISION_H

#include <Seeed_Arduino_SSCMA.h>
#include <Wire.h>

#include <deque>
#include <memory>

#include "CameraBase.hpp"
#include "esp_camera.h"
#include <mbedtls/base64.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define PTR_BUFFER_SIZE 8
#define COM_BUFFER_SIZE (1024 * 128)
#define RSP_BUFFER_SIZE (1024 * 196)
#define JPG_BUFFER_SIZE (1024 * 128)
#define RST_BUFFER_SIZE (1024 * 64)
#define QRY_BUFFER_SIZE (1024 * 16)
#define CMD_BUFFER_SIZE (1024 * 12)

#define BYTE_TRACKER_ENABLED 1
#else
#warning "Server may not work properly due to resource constraints..."
#define PTR_BUFFER_SIZE 3
#define COM_BUFFER_SIZE (1024 * 32)
#define RSP_BUFFER_SIZE (1024 * 32)
#define JPG_BUFFER_SIZE (1024 * 32)
#define RST_BUFFER_SIZE (1024 * 4)
#define QRY_BUFFER_SIZE (1024 * 4)
#define CMD_BUFFER_SIZE (1024 * 4)

#define BYTE_TRACKER_ENABLED 0
#endif

#define CMD_TAG_FMT_STR "HTTPD%.8X@"
#define CMD_TAG_SIZE snprintf(NULL, 0, CMD_TAG_FMT_STR, 0)

#define MSG_IMAGE_KEY "\"image\": "
#define MSG_COMMA_STR ", "
#define MSG_QUOTE_STR "\""
#define MSG_REPLY_STR "\"type\": 0"
#define MSG_EVENT_STR "\"type\": 1"
#define MSG_LOGGI_STR "\"type\": 2"
#define MSG_TERMI_STR "\r\n"

enum MsgType : uint16_t {
  MSG_TYPE_UNKNOWN = 0,
  MSG_TYPE_REPLY = 0xff & (1 << 1),
  MSG_TYPE_EVENT = 0xff & (1 << 2),
  MSG_TYPE_LOGGI = 0xff & (1 << 3),
};

#define CMD_SAMPLE_STR "SAMPLE"
#define CMD_INVOKE_STR "INVOKE"

enum CmdType : uint16_t {
  CMD_TYPE_UNKNOWN = 0,
  CMD_TYPE_SAMPLE = 0xff00 & (1 << 8),
  CMD_TYPE_INVOKE = 0xff00 & (2 << 8),
  CMD_TYPE_SENSOR = 0xff00 & (3 << 8),
};

struct PtrBuffer {
  struct Slot {
    size_t id = 0;
    uint16_t type = 0;
    void* data = NULL;
    size_t size = 0;
    timeval timestamp;
  };

  SemaphoreHandle_t mutex;
  std::deque<std::shared_ptr<Slot>> slots;
  volatile size_t id = 1;
  const size_t limit = PTR_BUFFER_SIZE;
};

struct StatInfo {
  size_t last_frame_id = 0;
  timeval last_frame_timestamp;
  SemaphoreHandle_t mutex;
};

class CamIAVision2 : public CameraBase {
 public:

  using CameraBase::free;
  bool begin();
  bool get(); 
  bool sendCmd(const char * command, int len);

  CamIAVision2() {
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

 private:
  
};

#endif
