#include <WiFi.h>
#include <esp_now.h>
#include <pb_decode.h>

#include "frame.pb.h"

extern "C" {
typedef void (*RecvCb)(uint32_t lenght);
}

class ESPNowReceiver {
 private:
 public:
  void setRecvCallback(RecvCb cb);
  void setRecvBuffer(uint8_t *fb);
  bool init();
};
