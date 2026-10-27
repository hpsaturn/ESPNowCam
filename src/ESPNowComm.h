/**************************************************
 * ESPNowCam EspNow (old) Communication Implementation
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
 **************************************************/

#ifndef ESPNOW_COMM_H
#define ESPNOW_COMM_H

#include <esp_now.h>

#include "CommInterface.h"

// ESPNow-specific error codes
#define ESPNOW_ERR_BASE 0x3100
#define ESPNOW_ERR_NOT_INIT (ESPNOW_ERR_BASE + 1)
#define ESPNOW_ERR_ARG (ESPNOW_ERR_BASE + 2)
#define ESPNOW_ERR_INTERNAL (ESPNOW_ERR_BASE + 3)
#define ESPNOW_ERR_NO_MEM (ESPNOW_ERR_BASE + 4)
#define ESPNOW_ERR_NOT_FOUND (ESPNOW_ERR_BASE + 5)
#define ESPNOW_ERR_IF (ESPNOW_ERR_BASE + 6)

// ESPNow-specific constants
#define ESPNOW_MAX_DATA_LEN 250

/**************************************************
 * E S P - N O W   C A L L B A C K   A D A P T E R
 *
 * The signature of the ESP-NOW callbacks is not the same in all the ESP-IDF
 * versions, so this adapter deduces it from the installed SDK instead of
 * guessing by version number (the same source builds with any of them):
 *
 *   send cb (IDF <= 5.4 / Arduino core 2.x and 3.0):
 *       void (*)(const uint8_t *mac_addr, esp_now_send_status_t status)
 *   send cb (IDF >= 5.5 / Arduino core 3.3+):
 *       void (*)(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
 *
 *   recv cb (IDF <= 4.4 / Arduino core 2.x):
 *       void (*)(const uint8_t *mac_addr, const uint8_t *data, int data_len)
 *   recv cb (IDF >= 5.0 / Arduino core 3.x):
 *       void (*)(const esp_now_recv_info_t *info, const uint8_t *data, int data_len)
 *
 * The peer address and the length type of each variant are extracted from the
 * callback typedef itself (CbSignature), and the fields that carry the address
 * are read through SendAddress/RecvAddress, which are specialized for the old
 * raw uint8_t mac_addr. A new SDK version only needs a new specialization here
 * if the shape of the signature changes again (the static_assert of the primary
 * templates says exactly that).
 **************************************************/
namespace espnow_adapter {

/// @brief Extracts the type of the first argument of a send callback
template <typename Fn>
struct SendCbSignature {
  static_assert(sizeof(Fn) == 0,
                "ESPNowComm: unsupported esp_now_send_cb_t signature. The ESP-NOW adapter of "
                "ESPNowComm.h needs a new SendCbSignature specialization for this ESP-IDF");
};
template <typename InfoT>
struct SendCbSignature<void (*)(const InfoT *, esp_now_send_status_t)> {
  typedef InfoT Info;
};

/// @brief Extracts the types of a recv callback (info type and length type)
template <typename Fn>
struct RecvCbSignature {
  static_assert(sizeof(Fn) == 0,
                "ESPNowComm: unsupported esp_now_recv_cb_t signature. The ESP-NOW adapter of "
                "ESPNowComm.h needs a new RecvCbSignature specialization for this ESP-IDF");
};
template <typename InfoT, typename LenT>
struct RecvCbSignature<void (*)(const InfoT *, const uint8_t *, LenT)> {
  typedef InfoT Info;
  typedef LenT Len;
};

/// @brief Reads the peer address of a send callback argument.
///        The primary template is the new IDF one (wifi_tx_info_t), the
///        uint8_t specialization is the old raw mac_addr of IDF <= 5.4.
template <typename InfoT>
struct SendAddress {
  static const uint8_t *of(const InfoT *info) { return (info != nullptr) ? info->des_addr : nullptr; }
};
template <>
struct SendAddress<uint8_t> {
  static const uint8_t *of(const uint8_t *mac_addr) { return mac_addr; }
};

/// @brief Reads the source address of a recv callback argument.
///        The primary template is the new IDF one (esp_now_recv_info_t), the
///        uint8_t specialization is the old raw mac_addr of IDF <= 4.4.
template <typename InfoT>
struct RecvAddress {
  static const uint8_t *of(const InfoT *info) { return (info != nullptr) ? info->src_addr : nullptr; }
};
template <>
struct RecvAddress<uint8_t> {
  static const uint8_t *of(const uint8_t *mac_addr) { return mac_addr; }
};

}  // namespace espnow_adapter

// ESPNow Communication implementation
class ESPNowComm : public CommInterface {
 private:
  // Types of the callbacks of the ESP-NOW SDK in use (see the adapter above)
  typedef typename espnow_adapter::SendCbSignature<esp_now_send_cb_t>::Info esp_send_info_t;
  typedef typename espnow_adapter::RecvCbSignature<esp_now_recv_cb_t>::Info esp_recv_info_t;
  typedef typename espnow_adapter::RecvCbSignature<esp_now_recv_cb_t>::Len esp_recv_len_t;

  // Static callbacks to bridge between C-style ESPNow callbacks and C++ class
  static void esp_send_cb_wrapper(const esp_send_info_t *info, esp_now_send_status_t esp_status);
  static void esp_recv_cb_wrapper(const esp_recv_info_t *info, const uint8_t *data, esp_recv_len_t data_len);

  // User callbacks
  static comm_send_cb_t user_send_cb;
  static comm_recv_cb_t user_recv_cb;

 public:
  comm_err_t init() override;
  bool isPeerExist(const uint8_t* mac_addr) override;
  comm_err_t addPeer(const CommPeerInfo* peer) override;
  comm_err_t send(const uint8_t* mac_addr, const uint8_t* data, size_t len) override;
  comm_err_t registerSendCallback(comm_send_cb_t cb) override;
  comm_err_t registerRecvCallback(comm_recv_cb_t cb) override;
  void setChannel(uint8_t channel) override;

  ~ESPNowComm() override = default;
};

#endif  // ESPNOW_COMM_H
