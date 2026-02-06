#include "WiFiRawComm.h"
#include "Arduino.h"
#include "esp_heap_caps.h"

// Configuration
#define WIFI_CHANNEL 6
#define PACKET_SIZE 256
#define TEST_DURATION_MS 10000

// Initialize static members
comm_send_cb_t WiFiRawComm::user_send_cb = nullptr;
comm_recv_cb_t WiFiRawComm::user_recv_cb = nullptr;
bool WiFiRawComm::initialized = false;
uint8_t WiFiRawComm::current_channel = 1;
wifi_interface_t WiFiRawComm::wifi_if = WIFI_IF_STA;
uint8_t WiFiRawComm::local_mac[6] = {0};
std::vector<CommPeerInfo> WiFiRawComm::peers;

// Print MAC address
void print_mac_address(const uint8_t* mac) {
    log_i("MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Helper: Check if MAC address is broadcast
bool WiFiRawComm::is_broadcast_addr(const uint8_t* mac_addr) {
    for (int i = 0; i < 6; i++) {
        if (mac_addr[i] != 0xFF) return false;
    }
    return true;
}

// Helper: Check if MAC address is multicast
bool WiFiRawComm::is_multicast_addr(const uint8_t* mac_addr) {
    return (mac_addr[0] & 0x01) != 0;  // LSB of first byte is 1 for multicast
}

// Helper: Create raw 802.11 frame
void WiFiRawComm::create_raw_frame(uint8_t* buffer, const uint8_t* dest_mac, 
                                  const uint8_t* src_mac, const uint8_t* data, 
                                  size_t data_len, size_t* frame_len) {
    WiFiRawFrame* frame = reinterpret_cast<WiFiRawFrame*>(buffer);
    
    // Frame Control: Data frame, To DS=0, From DS=0, More Frag=0, Retry=0, Pwr Mgt=0, More Data=0, Protected=0
    frame->frame_control[0] = 0x08;  // Type: Data, Subtype: Data
    frame->frame_control[1] = 0x00;  // To DS=0, From DS=0, More Frag=0, Retry=0, Pwr Mgt=0, More Data=0, Protected=0
    
    // Duration: 0 (not used in this context)
    frame->duration[0] = 0x00;
    frame->duration[1] = 0x00;
    
    // Address fields
    memcpy(frame->addr1, dest_mac, 6);  // Receiver address (DA)
    memcpy(frame->addr2, src_mac, 6);   // Transmitter address (SA)
    memcpy(frame->addr3, dest_mac, 6);  // BSSID (same as receiver for ad-hoc)
    
    // Sequence control: Use fixed value for simplicity
    frame->sequence[0] = 0x00;
    frame->sequence[1] = 0x00;
    
    // LLC/SNAP header
    frame->llc_dsap = 0xAA;            // DSAP
    frame->llc_ssap = 0xAA;            // SSAP
    frame->llc_control = 0x03;         // Unnumbered Information
    frame->snap_oui[0] = 0x00;         // OUI
    frame->snap_oui[1] = 0x00;
    frame->snap_oui[2] = 0x00;
    frame->snap_type[0] = 0x08;        // EtherType: IP
    frame->snap_type[1] = 0x00;
    
    // Copy payload
    if (data_len > 0 && data != nullptr) {
        memcpy(frame->payload, data, data_len);
    }
    
    *frame_len = sizeof(WiFiRawFrame) + data_len;
}

// Helper: Parse raw 802.11 frame
void WiFiRawComm::parse_raw_frame(const uint8_t* frame, size_t frame_len,
                                 uint8_t* src_mac, uint8_t* dest_mac,
                                 const uint8_t** payload, size_t* payload_len) {
    if (frame_len < sizeof(WiFiRawFrame)) {
        *payload_len = 0;
        return;
    }
    
    const WiFiRawFrame* wifi_frame = reinterpret_cast<const WiFiRawFrame*>(frame);
    
    // Extract MAC addresses
    memcpy(dest_mac, wifi_frame->addr1, 6);  // Receiver address
    memcpy(src_mac, wifi_frame->addr2, 6);   // Transmitter address
    
    // Check if this is our frame type
    if (wifi_frame->llc_dsap == 0xAA && wifi_frame->llc_ssap == 0xAA && 
        wifi_frame->llc_control == 0x03 &&
        wifi_frame->snap_oui[0] == 0x00 && wifi_frame->snap_oui[1] == 0x00 && 
        wifi_frame->snap_oui[2] == 0x00 &&
        wifi_frame->snap_type[0] == 0x08 && wifi_frame->snap_type[1] == 0x00) {
        
        // Valid frame, extract payload
        *payload = wifi_frame->payload;
        *payload_len = frame_len - sizeof(WiFiRawFrame);
    } else {
        *payload_len = 0;
    }
}

// Promiscuous mode callback wrapper
void WiFiRawComm::wifi_promiscuous_cb_wrapper(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA || user_recv_cb == nullptr) {
        return;
    }
    
    wifi_promiscuous_pkt_t* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);
    
    // Parse the frame
    uint8_t src_mac[6];
    uint8_t dest_mac[6];
    const uint8_t* payload = nullptr;
    size_t payload_len = 0;
    
    parse_raw_frame(pkt->payload, pkt->rx_ctrl.sig_len, src_mac, dest_mac, &payload, &payload_len);
    
    if (payload_len > 0) {
        // Check if this frame is for us (our MAC or broadcast)
        bool for_us = false;
        
        // Check broadcast
        if (is_broadcast_addr(dest_mac)) {
            for_us = true;
        }
        // Check our MAC address
        else if (memcmp(dest_mac, local_mac, 6) == 0) {
            for_us = true;
        }
        // Check multicast (if we want to receive multicast)
        else if (is_multicast_addr(dest_mac)) {
            // For simplicity, we accept all multicast
            for_us = true;
        }
        
        if (for_us) {
            // Call user callback
            user_recv_cb(src_mac, payload, payload_len);
        }
    }
}

void init_sender() {
    // WiFi configuration - CRITICAL: Use defaults but adjust for large packets
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    
    // IMPORTANT: For large packets, we need dynamic buffers
    cfg.tx_buf_type = 1;           // Dynamic buffers
    cfg.dynamic_tx_buf_num = 32;   // Enough buffers for continuous transmission
    cfg.static_tx_buf_num = 8;     // No static buffers
    cfg.beacon_max_len = sizeof(WiFiRawFrame) + WIFI_RAW_MAX_DATA_LEN;  // MUST match packet size
    cfg.cache_tx_buf_num = 4;      // Cache for performance
    
    // Disable features we don't need
    cfg.ampdu_rx_enable = 0;
    cfg.ampdu_tx_enable = 0;
    cfg.amsdu_tx_enable = 0;
    
    log_i("Config: dynamic buffers=%d, beacon_max_len=%d", cfg.dynamic_tx_buf_num, cfg.beacon_max_len);
    
    // Initialize WiFi
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    log_i("initialized for transmiting");
}

void init_receiver() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    
    // Initialize WiFi
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    
    log_i("initialized in promiscuous mode");
}

// Initialize WiFi Raw communication
comm_err_t WiFiRawComm::init() {
    if (initialized) {
        return COMM_OK;
    }
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    log_i("NVS flash initialized");

    // ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t event_loop_ret = esp_event_loop_create_default();
    if (event_loop_ret == ESP_OK) {
        log_i("Event loop created");
    } else if (event_loop_ret == ESP_ERR_INVALID_STATE) {
        log_i("Event loop already exists");
    } else {
        log_e("Event loop error: %d", event_loop_ret);
        return COMM_ERR_INTERNAL;
    }
    
    esp_netif_create_default_wifi_sta();

    if (user_recv_cb != nullptr) {
      init_receiver();
      ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous_cb_wrapper));
    } 
    else init_sender();

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));
      
    uint8_t mac[6];
    // Get local MAC address
    if (esp_wifi_get_mac(wifi_if, local_mac) != ESP_OK) {
        return COMM_ERR_INTERNAL;
    }
    print_mac_address(mac);
    
    log_i("WiFi initialized. Free heap: %d bytes", esp_get_free_heap_size());
    initialized = true;
    return COMM_OK;    
}

// Check if peer exists
bool WiFiRawComm::isPeerExist(const uint8_t* mac_addr) {
    if (mac_addr == nullptr) return false;
    
    for (const auto& peer : peers) {
        if (memcmp(peer.peer_addr, mac_addr, 6) == 0) {
            return true;
        }
    }
    
    return false;
}

// Add a peer (for WiFi Raw, we just store the MAC address)
comm_err_t WiFiRawComm::addPeer(const CommPeerInfo* peer) {
    if (peer == nullptr) {
        return COMM_ERR_ARG;
    }
    
    // Check if peer already exists
    if (isPeerExist(peer->peer_addr)) {
        return COMM_OK;  // Already exists
    }
    
    // Add to peers list
    peers.push_back(*peer);
    return COMM_OK;
}

// Send data using raw 802.11 frames
comm_err_t WiFiRawComm::send(const uint8_t* mac_addr, const uint8_t* data, size_t len) {
    if (!initialized) {
        return COMM_ERR_NOT_INIT;
    }
    
    if (mac_addr == nullptr || data == nullptr || len == 0) {
        return COMM_ERR_ARG;
    }
    
    if (len > WIFI_RAW_MAX_DATA_LEN) {
        return COMM_ERR_ARG;  // Too large
    }
    
    // Create raw frame buffer
    size_t frame_len = 0;
    uint8_t frame_buffer[sizeof(WiFiRawFrame) + WIFI_RAW_MAX_DATA_LEN];
    
    create_raw_frame(frame_buffer, mac_addr, local_mac, data, len, &frame_len);
    
    // Memory exhaustion detection - check heap before sending
    size_t free_heap_before = esp_get_free_heap_size();
    // log_i("Memory before send: free_heap=%d, frame_len=%d", free_heap_before, frame_len);
    
    // Send using esp_wifi_80211_tx
    // Note: en_sys_seq = false for raw frames without connection
    esp_err_t esp_err = esp_wifi_80211_tx(wifi_if, frame_buffer, frame_len, false);
    // log_i("80211_tx esp_err_t: %s", esp_err_to_name(esp_err));
    
    // Check memory after sending
    size_t free_heap_after = esp_get_free_heap_size();
    // log_i("Memory after send: free_heap=%d, delta=%d", free_heap_after, free_heap_before - free_heap_after);
    
    // Handle ESP_ERR_NO_MEM with retry logic
    if (esp_err == ESP_ERR_NO_MEM) {
        log_w("Memory exhausted! Free heap: %d, waiting for recovery...", free_heap_after);
        
        // Exponential backoff retry
        for (int retry = 0; retry < 3; retry++) {
            vTaskDelay((50 * (1 << retry)) / portTICK_PERIOD_MS); // 50ms, 100ms, 200ms
            
            // Check if memory has recovered
            size_t current_heap = esp_get_free_heap_size();
            log_i("Retry %d: free_heap=%d", retry + 1, current_heap);
            
            esp_err = esp_wifi_80211_tx(wifi_if, frame_buffer, frame_len, false);
            log_i("80211_tx retry %d: %s", retry + 1, esp_err_to_name(esp_err));
            
            if (esp_err == ESP_OK) {
                log_i("Memory recovered after %d retries", retry + 1);
                break;
            }
        }
    }
    
    // vTaskDelay(5 / portTICK_PERIOD_MS);
    delay(5);
    
    if (esp_err != ESP_OK) {
        // Log detailed error information
        log_e("Send failed with error: %s (0x%x)", esp_err_to_name(esp_err), esp_err);
        log_e("Frame length: %d, Free heap: %d", frame_len, esp_get_free_heap_size());
        return WIFI_RAW_ERR_TX_FAILED;
    }
    
    // Call send callback if registered
    if (user_send_cb != nullptr) {
        user_send_cb(mac_addr, COMM_SEND_SUCCESS);
    }
    
    return COMM_OK;
}

// Register send callback
comm_err_t WiFiRawComm::registerSendCallback(comm_send_cb_t cb) {
    user_send_cb = cb;
    return COMM_OK;
}

// Register receive callback
comm_err_t WiFiRawComm::registerRecvCallback(comm_recv_cb_t cb) {
    log_i("Registering receive callback");
    user_recv_cb = cb;
    return COMM_OK;
}

// Set channel
void WiFiRawComm::setChannel(uint8_t channel) {
    if (channel >= 1 && channel <= 14) {
        current_channel = channel;
        if (initialized) {
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        }
    }
}

// Destructor
WiFiRawComm::~WiFiRawComm() {
    if (initialized) {
        // TODO: check this! Disable promiscuous mode
        esp_wifi_set_promiscuous(false);
        initialized = false;
    }
}
