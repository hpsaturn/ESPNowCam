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
#include "FrameMeta.h"
#include "WiFiRawComm.h"

extern "C" {
typedef void (*RecvCb)(uint32_t lenght);
}

/// receiver callback for the metadata of the received frames (see FrameMeta.h).
/// It is called right after the frame callback, so the frame can be painted
/// first and the AI results on top of it.
typedef void (*MetaCb)(const uint8_t *meta, uint32_t lenght);

#define ENC_VERSION "0.2.2"
#define ENC_REVISION 085

// Maximum data length (can be overridden by implementations)
#ifndef COMM_MAX_DATA_LEN
#define COMM_MAX_DATA_LEN 350
#endif

// Maximum metadata size of a frame (see FrameMeta.h and src/frame.proto)
#ifndef COMM_MAX_META_LEN
#define COMM_MAX_META_LEN FRAME_META_MAX_SIZE
#endif

// Milliseconds that sendData() waits for the send confirmation of a chunk, the
// radio callback is synchronous on both interfaces, it is only a safety net
#ifndef COMM_SEND_TIMEOUT
#define COMM_SEND_TIMEOUT 500
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
  /// @param meta optional metadata of this frame (see FrameMeta.h), it travels
  ///        with the last chunk of the frame, so the receiver gets it together
  ///        with the frame it belongs to
  /// @param metaLenght length of the metadata
  bool sendData(uint8_t *data, uint32_t lenght, const uint8_t *meta = nullptr,
                uint32_t metaLenght = 0);

  /// @brief set the target device MAC address. (For 1:1 mode)
  /// @param macAddress pointer to the MAC address of the target device
  bool setTarget(const uint8_t *macAddress);

  /// @brief set the receiver data callback.
  /// @param cb callback function to be called when data is received
  void setRecvCallback(RecvCb cb);

  /// @brief set the receiver buffer.
  /// @param fb pointer to the buffer to receive the data
  void setRecvBuffer(uint8_t *fb);

  /// @brief set the receiver metadata callback (optional, single source mode).
  ///        see FrameMeta.h to read the metadata of the frame (AI results).
  /// @param cb callback function to be called with the metadata of a frame
  void setMetaCallback(MetaCb cb);

  /// @brief filter for multiple sources (N:1)
  /// @param fb pointer to the buffer to receive the data
  /// @param macAddr source device MAC address to filter
  /// @param cb frame data callback for this source device
  void setRecvFilter(uint8_t *fb, const uint8_t *macAddr, RecvCb cb);

  /// @brief (optional) set WiFi channel. It should be the same on both sender and receiver.
  /// @param channel WiFi channel to be used
  void setChannel(uint8_t channel);

  /// @brief initialize the communication.
  /// @param chunksize default for ESPNow <= 244, for WiFiRaw > 244 max ~1000 bytes.
  bool init(uint16_t chunksize = 244);
};

#endif
