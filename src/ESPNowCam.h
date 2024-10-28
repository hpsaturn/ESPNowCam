#ifndef ESPNOWCAM_H
#define ESPNOWCAM_H

#include <WiFi.h>
#include <esp_now.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "frame.pb.h"
#include <map>
#include <vector>

extern "C" {
typedef void (*RecvCb)(uint32_t lenght);
}

#define ENC_VERSION "0.1.15"
#define ENC_REVISION 080

class ESPNowCam {
 private:
  uint8_t targetAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
 public:
  void setRecvCallback(RecvCb cb);
  void setRecvBuffer(uint8_t *fb);
  void setRecvFilter(uint8_t *fb, const uint8_t *macAddr, RecvCb cb);
  bool sendData(uint8_t* data, uint32_t lenght);
  bool setTarget(const uint8_t* macAddress);
  bool init(uint8_t chunksize = 244);
};

#endif