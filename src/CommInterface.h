/**************************************************
 * ESPNowCam Communication Common Interface
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
 **************************************************/

#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

// Maximum data length (can be overridden by implementations)
#ifndef COMM_MAX_DATA_LEN
#define COMM_MAX_DATA_LEN 640
#endif

// Simple error type for abstraction
typedef int comm_err_t;
#define COMM_OK 0

// Error codes for abstraction
#define COMM_ERR_BASE 0x3000
#define COMM_ERR_NOT_INIT (COMM_ERR_BASE + 1)
#define COMM_ERR_ARG (COMM_ERR_BASE + 2)
#define COMM_ERR_INTERNAL (COMM_ERR_BASE + 3)
#define COMM_ERR_NO_MEM (COMM_ERR_BASE + 4)
#define COMM_ERR_NOT_FOUND (COMM_ERR_BASE + 5)
#define COMM_ERR_IF (COMM_ERR_BASE + 6)

// Send status for abstraction
typedef enum { COMM_SEND_SUCCESS = 0, COMM_SEND_FAIL = 1 } comm_send_status_t;

// Callback types using abstracted types
typedef void (*comm_send_cb_t)(const uint8_t* mac_addr, comm_send_status_t status);
typedef void (*comm_recv_cb_t)(const uint8_t* mac_addr, const uint8_t* data, int32_t data_len);

// Simple peer info structure (platform independent)
struct CommPeerInfo {
  uint8_t peer_addr[6];
  uint8_t channel;
  uint8_t encrypt;
  uint8_t ifidx;
};

// Communication Interface base class
class CommInterface {
 public:
  // Initialization
  virtual comm_err_t init() = 0;

  // Peer management
  virtual bool isPeerExist(const uint8_t* mac_addr) = 0;
  virtual comm_err_t addPeer(const CommPeerInfo* peer) = 0;

  // Sending data
  virtual comm_err_t send(const uint8_t* mac_addr, const uint8_t* data, size_t len) = 0;

  // Callback registration
  virtual comm_err_t registerSendCallback(comm_send_cb_t cb) = 0;
  virtual comm_err_t registerRecvCallback(comm_recv_cb_t cb) = 0;

  // Channel control (optional - some comm technologies may not support this)
  virtual void setChannel(uint8_t channel) = 0;

  virtual ~CommInterface() = default;
};

#endif  // COMM_INTERFACE_H
