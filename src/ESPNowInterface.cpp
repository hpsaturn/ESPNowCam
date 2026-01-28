#include "ESPNowInterface.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

// Static variables to store callbacks
static espnow_send_cb_t user_send_cb = nullptr;
static espnow_recv_cb_t user_recv_cb = nullptr;

// Wrapper callback to convert ESP types to abstract types
static void esp_send_cb_wrapper(const uint8_t* mac_addr, esp_now_send_status_t esp_status) {
    if (user_send_cb) {
        espnow_send_status_t status = (espnow_send_status_t)esp_status;
        user_send_cb(mac_addr, status);
    }
}

// Wrapper callback for receive
static void esp_recv_cb_wrapper(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    if (user_recv_cb) {
        user_recv_cb(mac_addr, data, data_len);
    }
}

espnow_err_t ESPNowConcrete::init() {
    return (espnow_err_t)esp_now_init();
}

bool ESPNowConcrete::isPeerExist(const uint8_t* mac_addr) {
    return esp_now_is_peer_exist(mac_addr);
}

espnow_err_t ESPNowConcrete::addPeer(const ESPNowPeerInfo* peer) {
    esp_now_peer_info_t espnow_peer = {};
    memcpy(espnow_peer.peer_addr, peer->peer_addr, 6);
    espnow_peer.channel = peer->channel;
    espnow_peer.encrypt = peer->encrypt;
    espnow_peer.ifidx = (wifi_interface_t)peer->ifidx;
    return (espnow_err_t)esp_now_add_peer(&espnow_peer);
}

espnow_err_t ESPNowConcrete::send(const uint8_t* mac_addr, const uint8_t* data, size_t len) {
    return (espnow_err_t)esp_now_send(mac_addr, data, len);
}

espnow_err_t ESPNowConcrete::registerSendCallback(espnow_send_cb_t cb) {
    user_send_cb = cb;
    return (espnow_err_t)esp_now_register_send_cb(esp_send_cb_wrapper);
}

espnow_err_t ESPNowConcrete::registerRecvCallback(espnow_recv_cb_t cb) {
    user_recv_cb = cb;
    return (espnow_err_t)esp_now_register_recv_cb(esp_recv_cb_wrapper);
}

void ESPNowConcrete::setChannel(uint8_t channel) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}
