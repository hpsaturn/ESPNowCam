/**************************************************
 * ESPNowCam EspNow (old) Communication Implementation
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
 **************************************************/

#include "ESPNowComm.h"

#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

// Initialize static members
comm_send_cb_t ESPNowComm::user_send_cb = nullptr;
comm_recv_cb_t ESPNowComm::user_recv_cb = nullptr;

// Wrapper callback to convert ESP types to abstract types
void ESPNowComm::esp_send_cb_wrapper(const uint8_t* mac_addr, esp_now_send_status_t esp_status) {
  if (user_send_cb) {
    comm_send_status_t status = (comm_send_status_t)esp_status;
    user_send_cb(mac_addr, status);
  }
}

// Wrapper callback for receive
void ESPNowComm::esp_recv_cb_wrapper(const uint8_t* mac_addr, const uint8_t* data, int32_t data_len) {
  if (user_recv_cb) {
    user_recv_cb(mac_addr, data, data_len);
  }
}

comm_err_t ESPNowComm::init() { return (comm_err_t)esp_now_init(); }

bool ESPNowComm::isPeerExist(const uint8_t* mac_addr) { return esp_now_is_peer_exist(mac_addr); }

comm_err_t ESPNowComm::addPeer(const CommPeerInfo* peer) {
  esp_now_peer_info_t espnow_peer = {};
  memcpy(espnow_peer.peer_addr, peer->peer_addr, 6);
  espnow_peer.channel = peer->channel;
  espnow_peer.encrypt = peer->encrypt;
  espnow_peer.ifidx = (wifi_interface_t)peer->ifidx;
  return (comm_err_t)esp_now_add_peer(&espnow_peer);
}

comm_err_t ESPNowComm::send(const uint8_t* mac_addr, const uint8_t* data, size_t len) {
  return (comm_err_t)esp_now_send(mac_addr, data, len);
}

comm_err_t ESPNowComm::registerSendCallback(comm_send_cb_t cb) {
  user_send_cb = cb;
  return (comm_err_t)esp_now_register_send_cb(esp_send_cb_wrapper);
}

comm_err_t ESPNowComm::registerRecvCallback(comm_recv_cb_t cb) {
  user_recv_cb = cb;
  return (comm_err_t)esp_now_register_recv_cb(esp_recv_cb_wrapper);
}

void ESPNowComm::setChannel(uint8_t channel) {
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}
