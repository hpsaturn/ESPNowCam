#ifndef WIFI_RAW_COMM_H
#define WIFI_RAW_COMM_H

#include "CommInterface.h"
#include <esp_wifi.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>
#include <algorithm>

// WiFi Raw-specific error codes
#define WIFI_RAW_ERR_BASE       0x3200
#define WIFI_RAW_ERR_NOT_INIT   (WIFI_RAW_ERR_BASE + 1)
#define WIFI_RAW_ERR_ARG        (WIFI_RAW_ERR_BASE + 2)
#define WIFI_RAW_ERR_INTERNAL   (WIFI_RAW_ERR_BASE + 3)
#define WIFI_RAW_ERR_NO_MEM     (WIFI_RAW_ERR_BASE + 4)
#define WIFI_RAW_ERR_NOT_FOUND  (WIFI_RAW_ERR_BASE + 5)
#define WIFI_RAW_ERR_IF         (WIFI_RAW_ERR_BASE + 6)
#define WIFI_RAW_ERR_TX_FAILED  (WIFI_RAW_ERR_BASE + 7)

// WiFi Raw-specific constants
#define WIFI_RAW_MAX_DATA_LEN 256  // Max payload for 802.11 frame
#define WIFI_RAW_ETH_HEADER_LEN 14
#define WIFI_RAW_80211_HEADER_LEN 24
#define WIFI_RAW_FRAME_CTRL 0x0800  // Ethernet type for IP

// Custom frame structure for raw WiFi transmission
#pragma pack(push, 1)
struct WiFiRawFrame {
    // 802.11 MAC header (24 bytes for data frames)
    uint8_t frame_control[2];      // Frame Control
    uint8_t duration[2];           // Duration
    uint8_t addr1[6];              // Receiver address (DA)
    uint8_t addr2[6];              // Transmitter address (SA)
    uint8_t addr3[6];              // BSSID
    uint8_t sequence[2];           // Sequence control
    
    // LLC/SNAP header (8 bytes)
    uint8_t llc_dsap;              // DSAP (0xAA)
    uint8_t llc_ssap;              // SSAP (0xAA)
    uint8_t llc_control;           // Control (0x03)
    uint8_t snap_oui[3];           // OUI (0x000000)
    uint8_t snap_type[2];          // EtherType (0x0800 for IP)
    
    // Payload
    uint8_t payload[0];            // Variable length payload
};
#pragma pack(pop)

// WiFi Raw Communication implementation
class WiFiRawComm : public CommInterface {
private:
    // Static callbacks to bridge between C-style WiFi callbacks and C++ class
    static void wifi_promiscuous_cb_wrapper(void* buf, wifi_promiscuous_pkt_type_t type);
    
    // User callbacks
    static comm_send_cb_t user_send_cb;
    static comm_recv_cb_t user_recv_cb;
    
    // Internal state
    static bool initialized;
    static uint8_t current_channel;
    static wifi_interface_t wifi_if;
    
    // MAC address handling
    static uint8_t local_mac[6];
    static std::vector<CommPeerInfo> peers;
    
    // Helper methods
    static bool is_broadcast_addr(const uint8_t* mac_addr);
    static bool is_multicast_addr(const uint8_t* mac_addr);
    static void create_raw_frame(uint8_t* buffer, const uint8_t* dest_mac, 
                                const uint8_t* src_mac, const uint8_t* data, 
                                size_t data_len, size_t* frame_len);
    static void parse_raw_frame(const uint8_t* frame, size_t frame_len,
                               uint8_t* src_mac, uint8_t* dest_mac,
                               const uint8_t** payload, size_t* payload_len);
    
public:
    comm_err_t init() override;
    bool isPeerExist(const uint8_t* mac_addr) override;
    comm_err_t addPeer(const CommPeerInfo* peer) override;
    comm_err_t send(const uint8_t* mac_addr, const uint8_t* data, size_t len) override;
    comm_err_t registerSendCallback(comm_send_cb_t cb) override;
    comm_err_t registerRecvCallback(comm_recv_cb_t cb) override;
    void setChannel(uint8_t channel) override;
    
    ~WiFiRawComm() override;
};

#endif // WIFI_RAW_COMM_H
