#ifndef ESPNOW_INTERFACE_H
#define ESPNOW_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

// Simple error type for abstraction
typedef int espnow_err_t;
#define ESPNOW_OK 0

// Error codes for abstraction
#define ESPNOW_ERR_BASE       0x3000
#define ESPNOW_ERR_NOT_INIT   (ESPNOW_ERR_BASE + 1)
#define ESPNOW_ERR_ARG        (ESPNOW_ERR_BASE + 2)
#define ESPNOW_ERR_INTERNAL   (ESPNOW_ERR_BASE + 3)
#define ESPNOW_ERR_NO_MEM     (ESPNOW_ERR_BASE + 4)
#define ESPNOW_ERR_NOT_FOUND  (ESPNOW_ERR_BASE + 5)
#define ESPNOW_ERR_IF         (ESPNOW_ERR_BASE + 6)

// Send status for abstraction
typedef enum {
    ESPNOW_SEND_SUCCESS = 0,
    ESPNOW_SEND_FAIL = 1
} espnow_send_status_t;

// Maximum data length
#define ESPNOW_MAX_DATA_LEN 250

// Callback types using abstracted types
typedef void (*espnow_send_cb_t)(const uint8_t* mac_addr, espnow_send_status_t status);
typedef void (*espnow_recv_cb_t)(const uint8_t* mac_addr, const uint8_t* data, int data_len);

// Simple peer info structure (platform independent)
struct ESPNowPeerInfo {
    uint8_t peer_addr[6];
    uint8_t channel;
    uint8_t encrypt;
    uint8_t ifidx;
};

// ESPNow Interface class
class ESPNowInterface {
public:
    // Initialization
    virtual espnow_err_t init() = 0;
    
    // Peer management
    virtual bool isPeerExist(const uint8_t* mac_addr) = 0;
    virtual espnow_err_t addPeer(const ESPNowPeerInfo* peer) = 0;
    
    // Sending data
    virtual espnow_err_t send(const uint8_t* mac_addr, const uint8_t* data, size_t len) = 0;
    
    // Callback registration
    virtual espnow_err_t registerSendCallback(espnow_send_cb_t cb) = 0;
    virtual espnow_err_t registerRecvCallback(espnow_recv_cb_t cb) = 0;
    
    // WiFi channel control
    virtual void setChannel(uint8_t channel) = 0;
    
    virtual ~ESPNowInterface() = default;
};

// Concrete implementation using actual ESPNow library
class ESPNowConcrete : public ESPNowInterface {
public:
    espnow_err_t init() override;
    bool isPeerExist(const uint8_t* mac_addr) override;
    espnow_err_t addPeer(const ESPNowPeerInfo* peer) override;
    espnow_err_t send(const uint8_t* mac_addr, const uint8_t* data, size_t len) override;
    espnow_err_t registerSendCallback(espnow_send_cb_t cb) override;
    espnow_err_t registerRecvCallback(espnow_recv_cb_t cb) override;
    void setChannel(uint8_t channel) override;
};

#endif // ESPNOW_INTERFACE_H
