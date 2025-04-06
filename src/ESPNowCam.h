#ifndef ESPNOWCAM_H
#define ESPNOWCAM_H

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include <map>
#include <vector>

#include "frame.pb.h"

extern "C" {
typedef void (*RecvCb)(uint32_t lenght);
}

#define ENC_VERSION "0.1.17"
#define ENC_REVISION 082

class ESPNowCam {
 private:
  uint8_t targetAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  int8_t _channel = -1;

 public:
  /// @brief send data to the target device or broadcast if targetAddress is not set.
  /// @param data pointer to the data to be sent
  /// @param lenght length of the data to be sent
  bool sendData(uint8_t *data, uint32_t lenght);

  /// @brief set the target device MAC address. (For 1:1 mode)
  /// @param macAddress pointer to the MAC address of the target device
  bool setTarget(const uint8_t *macAddress);

  /// @brief set the receiver data callback.
  /// @param cb callback function to be called when data is received
  void setRecvCallback(RecvCb cb);

  /// @brief set the receiver buffer.
  /// @param fb pointer to the buffer to receive the data
  void setRecvBuffer(uint8_t *fb);

  /// @brief filter for multiple sources (N:1)
  /// @param fb pointer to the buffer to receive the data
  /// @param macAddr source device MAC address to filter
  /// @param cb frame data callback for this source device
  void setRecvFilter(uint8_t *fb, const uint8_t *macAddr, RecvCb cb);

  /// @brief (optional) set WiFi channel. It should be the same on both sender and receiver.
  /// @param channel WiFi channel to be used
  void setChannel(uint8_t channel);

  /// @brief initialize the ESP-NOW communication.
  /// @param chunksize size of the chunks to be sent. Default is 244 bytes.
  bool init(uint8_t chunksize = 244);
};

#endif