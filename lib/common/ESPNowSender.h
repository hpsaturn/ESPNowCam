#include <WiFi.h>
#include <esp_now.h>
#include <pb_encode.h>
#include "frame.pb.h"

#define CHUNKSIZE 245

class ESPNowSender {
  private:
    
    uint8_t targetAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  public:
    bool sendData(uint8_t * data, uint32_t lenght);
    bool setTarget(uint8_t * macAddress);
    bool init();
};

