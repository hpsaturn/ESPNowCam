/**************************************************
 * ESPNowCam Main Implementation (ESPNow/WiFiRaw unified)
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#ifndef ESPNOWCAM_H
#define ESPNOWCAM_H

#include <WiFi.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include <map>
#include <vector>

#include "frame.pb.h"
#include "CommInterface.h"
#include "ESPNowComm.h"
#include "WiFiRawComm.h"

extern "C" {
typedef void (*RecvCb)(uint32_t lenght);
}

#define ENC_VERSION "0.1.17"
#define ENC_REVISION 082

// Maximum data length (can be overridden by implementations)
#ifndef COMM_MAX_DATA_LEN
#define COMM_MAX_DATA_LEN 350
#endif

class ESPNowCam {
 private:
  uint8_t targetAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  int8_t _channel = -1;
  CommInterface* comm = nullptr;
  
  void registerCallbacks();

 public:
  /// @brief Constructor with optional custom communication interface
  /// @param interface Pointer to CommInterface implementation (nullptr for default ESPNow)
  ESPNowCam(CommInterface* interface = nullptr);
  
  /// @brief Destructor
  ~ESPNowCam();

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

  /// @brief initialize the communication.
  /// @param chunksize size of the chunks to be sent. Default is 244 bytes.
  bool init(uint16_t chunksize = 244);
};

#endif
