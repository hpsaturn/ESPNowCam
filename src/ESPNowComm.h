#ifndef ESPNOW_COMM_H
#define ESPNOW_COMM_H

#include "CommInterface.h"
#include <esp_now.h>

// ESPNow-specific error codes
#define ESPNOW_ERR_BASE       0x3100
#define ESPNOW_ERR_NOT_INIT   (ESPNOW_ERR_BASE + 1)
#define ESPNOW_ERR_ARG        (ESPNOW_ERR_BASE + 2)
#define ESPNOW_ERR_INTERNAL   (ESPNOW_ERR_BASE + 3)
#define ESPNOW_ERR_NO_MEM     (ESPNOW_ERR_BASE + 4)
#define ESPNOW_ERR_NOT_FOUND  (ESPNOW_ERR_BASE + 5)
#define ESPNOW_ERR_IF         (ESPNOW_ERR_BASE + 6)

// ESPNow-specific constants
#define ESPNOW_MAX_DATA_LEN 250

// ESPNow Communication implementation
class ESPNowComm : public CommInterface {
private:
    // Static callbacks to bridge between C-style ESPNow callbacks and C++ class
    static void esp_send_cb_wrapper(const uint8_t* mac_addr, esp_now_send_status_t esp_status);
    static void esp_recv_cb_wrapper(const uint8_t* mac_addr, const uint8_t* data, int data_len);
    
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

#endif // ESPNOW_COMM_H
