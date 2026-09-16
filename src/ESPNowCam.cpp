/**************************************************
 * ESPNowCam Main Implementation (ESPNow/WiFiRaw unified)
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include "ESPNowCam.h"
#include "CommInterface.h"
#include "typeinfo"
#include <algorithm>

// Static instance of default ESPNow implementation
static ESPNowComm defaultESPNow;

/***********************************
 * S E N D E R  S E C T I O N
************************************/
uint16_t chunksize = 244;
uint8_t send_buffer[COMM_MAX_DATA_LEN - 8];
uint32_t chunk_size_left = chunksize;
uint32_t chunk_pos = 0;
uint8_t *outdata = NULL;
uint32_t outdata_len = 0;
bool msgReady = false;

bool sendMessage(uint32_t msglen, const uint8_t *mac, CommInterface* comm);
uint32_t encodeMsg(Frame msg);
uint32_t encodeMsg(FrameWifiRaw msg);

bool encode_uint8_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
  if (!pb_encode_tag_for_field(stream, field))
    return false;
  return pb_encode_string(stream, static_cast<uint8_t*>(outdata + chunk_pos), chunk_size_left);
}

void msgSentCb(const uint8_t *macAddr, comm_send_status_t  status) {
  msgReady = true;
}

// Constructor and Destructor
ESPNowCam::ESPNowCam(CommInterface* interface) {
  if (interface == nullptr) {
    comm = &defaultESPNow;
  } else {
    comm = interface;
  }
}

ESPNowCam::~ESPNowCam() {
  // Cleanup if needed
}

bool ESPNowCam::sendData(uint8_t *data, uint32_t lenght, const uint8_t *meta,
                         uint32_t metaLenght) {
  log_v("sending data (size: %i, meta: %i)", lenght, metaLenght);
  if (data == nullptr || lenght == 0) {
    return false;
  }

  /// the metadata of a frame only travels on the interfaces with room for it
  /// (the extended src/frame_wifiraw.proto schema). The old ESP-NOW protocol
  /// is kept byte compatible with the previous releases of the library: its
  /// frames have no metadata field at all
  static bool metaWarned = false;
  bool extended = comm->supportsFrameMetadata();
  if (metaLenght > 0 && !extended) {
    if (!metaWarned) {
      metaWarned = true;
      log_w("This interface can not carry frame metadata, the frames are sent without it..");
    }
    metaLenght = 0;
  }

  if (metaLenght > COMM_MAX_META_LEN) {
    log_w("Frame metadata too big (%u bytes, max %u), sending the frame without it..",
          (unsigned int)metaLenght, (unsigned int)COMM_MAX_META_LEN);
    metaLenght = 0;
  }
  /// the metadata rides with the last chunk of the frame, that chunk is
  /// reduced to leave room for it inside the send buffer (the protobuf headers
  /// and the length field need some bytes too)
  uint32_t meta_reserve = 0;
  if (metaLenght > 0) {
    meta_reserve = metaLenght + 16;
    if (meta_reserve >= sizeof(send_buffer)) {
      log_e("Frame metadata does not fit in the send buffer, sending the frame without it..");
      metaLenght = 0;
      meta_reserve = 0;
    }
  }

  outdata = data;
  outdata_len = lenght;

  Frame msg = Frame_init_zero;
  FrameWifiRaw msg_ext = FrameWifiRaw_init_zero;
  uint32_t offset = 0;
  while (offset < outdata_len) {
    uint32_t left = outdata_len - offset;
    log_v("frame left: %i)", left);
    chunk_pos = offset;
    chunk_size_left = min(left, (uint32_t)chunksize);
    bool last = (left <= (uint32_t)chunksize);

    if (last && meta_reserve > 0) {
      uint32_t room = (uint32_t)sizeof(send_buffer) - meta_reserve;
      if (chunk_size_left > room) {
        chunk_size_left = room; // the last chunk gives room to the metadata
        last = (left <= chunk_size_left);
      }
    }

    /// the schema of the message depends on the interface: the extended one
    /// carries the metadata of the frame in its last chunk
    uint32_t msgLen = 0;
    if (extended) {
      msg_ext.lenght = last ? outdata_len : 0;
      msg_ext.has_meta = false;
      if (last && metaLenght > 0) {
        memcpy(msg_ext.meta.bytes, meta, metaLenght);
        msg_ext.meta.size = (pb_size_t)metaLenght;
        msg_ext.has_meta = true;
      }
      msg_ext.data.funcs.encode = &encode_uint8_array;
      msgLen = encodeMsg(msg_ext);
    } else {
      msg.lenght = last ? outdata_len : 0;
      msg.data.funcs.encode = &encode_uint8_array;
      msgLen = encodeMsg(msg);
    }
    if (msgLen == 0) {
      log_e("Encoding failed, the rest of the frame is dropped..");
      break;
    }
    msgReady = false;
    sendMessage(msgLen, this->targetAddress, comm);
    uint32_t start = millis();
    while (!msgReady && (millis() - start) < COMM_SEND_TIMEOUT) delayMicroseconds(1);
    if (!msgReady) {
      log_e("Timeout waiting the send confirmation of the chunk, the frame is corrupted..");
      break;
    }
    offset += chunk_size_left;
  }
  chunk_pos = 0;
  chunk_size_left = chunksize;
  return true;
}

uint32_t encodeMsg(Frame msg) {
  pb_ostream_t stream = pb_ostream_from_buffer(send_buffer, sizeof(send_buffer));
  
  bool status = pb_encode(&stream, Frame_fields, &msg);
  uint32_t message_length = stream.bytes_written;
  if (!status) {
    printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
    return 0;
  }
  return message_length;
}

/// @brief encode the extended schema (src/frame_wifiraw.proto), it is the one
///        used by the interfaces with room for the metadata of the frame
uint32_t encodeMsg(FrameWifiRaw msg) {
  pb_ostream_t stream = pb_ostream_from_buffer(send_buffer, sizeof(send_buffer));

  bool status = pb_encode(&stream, FrameWifiRaw_fields, &msg);
  uint32_t message_length = stream.bytes_written;
  if (!status) {
    printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
    return 0;
  }
  return message_length;
}

bool sendMessage(uint32_t msglen, const uint8_t *mac, CommInterface* comm) {
  CommPeerInfo peerInfo = {};
  memcpy(&peerInfo.peer_addr, mac, 6);
  if (!comm->isPeerExist(mac)) {
    comm->addPeer(&peerInfo);
  }
  esp_err_t result = comm->send(mac, send_buffer, msglen);

  if (result == COMM_OK) {
    // log_v("send msg success");
    return true;
  } else if (result == COMM_ERR_NOT_INIT) {
    log_e("ESPNOW not Init.");
  } else if (result == COMM_ERR_ARG) {
    log_e("Invalid Argument");
  } else if (result == COMM_ERR_INTERNAL) {
    log_e("Internal Error");
  } else if (result == COMM_ERR_NO_MEM) {
    log_e("COMM_ERR_NO_MEM");
  } else if (result == COMM_ERR_NOT_FOUND) {
    log_e("Peer not found.");
  } else {
    log_e("Unknown error");
  }
  return false;
}

bool ESPNowCam::setTarget(const uint8_t *macAddress) {
  memcpy(targetAddress, macAddress, 6);
  return false;
}

/***********************************
 * U T I L S
************************************/

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength) {
    snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void printMacAddress(const uint8_t * macAddress){
    char macStr[18];
    formatMacAddress(macAddress, macStr, 18);
    log_i("%s",macStr);
}

/***********************************
 * R E C E I V E R  S E C T I O N
 * (for single camera source)
************************************/
uint8_t recv_buffer[COMM_MAX_DATA_LEN];
uint32_t fbpos = 0;
RecvCb recvCb = nullptr;
MetaCb metaCb = nullptr;
uint8_t *recvBuffer = nullptr;
Frame msg_recv = Frame_init_zero;
FrameWifiRaw msg_recv_ext = FrameWifiRaw_init_zero;
/// schema in use, it is selected by the communication interface on init()
/// (see ESPNowCam::init and CommInterface::supportsFrameMetadata)
bool frameMetadata = false;

bool decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  /// store the initial bytes left for after update fpos
  size_t bytes_left = stream->bytes_left;

  if (!pb_read(stream, recvBuffer + fbpos, stream->bytes_left))
    return false;

  fbpos = fbpos + bytes_left;
  return true;
}

bool decodeMessage(uint16_t message_length) {
  pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
  bool status;
  if (frameMetadata) {
    msg_recv_ext.data.funcs.decode = &decode_data;
    status = pb_decode(&stream, FrameWifiRaw_fields, &msg_recv_ext);
  } else {
    msg_recv.data.funcs.decode = &decode_data;
    status = pb_decode(&stream, Frame_fields, &msg_recv);
  }
  if (!status) {
    log_w("%s", PB_GET_ERROR(&stream));
    return false;
  }
  return true;
}

/// @brief length of the frame of the last decoded chunk (0 while the frame is
///        still incomplete, it is only set on its last chunk)
uint32_t decodedLenght() { return frameMetadata ? msg_recv_ext.lenght : msg_recv.lenght; }

/// @brief metadata of the last decoded frame (see FrameMeta.h), it is never
///        available on the interfaces that can not carry it (the ESP-NOW one)
/// @return false when the frame has no metadata
bool decodedMeta(const uint8_t **meta, uint32_t *lenght) {
  if (!frameMetadata || !msg_recv_ext.has_meta) {
    return false;
  }
  *meta = msg_recv_ext.meta.bytes;
  *lenght = msg_recv_ext.meta.size;
  return true;
}

void msgReceiveCb(const uint8_t *macAddr, const uint8_t *data, int32_t dataLen) {
  /// never copy more than the receive buffer (a truncated message is detected
  /// by the protobuf decoder, but an overflow is corrupted memory)
  uint32_t msgLen = ((uint32_t)dataLen < sizeof(recv_buffer)) ? (uint32_t)dataLen
                                                             : (uint32_t)sizeof(recv_buffer);
  memcpy(recv_buffer, data, msgLen);
  if (decodeMessage(msgLen) && decodedLenght() > 0) {
    if (recvCb != nullptr) recvCb(decodedLenght());
    /// the metadata of this frame is available once the frame is complete
    const uint8_t *meta = nullptr;
    uint32_t metaLen = 0;
    if (metaCb != nullptr && decodedMeta(&meta, &metaLen)) metaCb(meta, metaLen);
    fbpos = 0;
  }
}

void ESPNowCam::setRecvCallback(RecvCb cb) {
  recvCb = cb;
}

void ESPNowCam::setRecvBuffer(uint8_t *fb) {
  recvBuffer = fb;
}

void ESPNowCam::setMetaCallback(MetaCb cb) {
  metaCb = cb;
}

bool ESPNowCam::supportsFrameMetadata() {
  return (comm != nullptr) && comm->supportsFrameMetadata();
}

/***********************************
 * R E C E I V E R  S E C T I O N
 * (for multiple camera sources)
************************************/

typedef struct struct_receiver {
    uint32_t id;    // must be unique for each sender board
    uint32_t fbpos; 
    uint8_t *fb;
    RecvCb recvCb;
} struct_receiver;

struct_receiver *curReceiver = nullptr;

/// filters map (id,macaddr)
typedef std::map<uint32_t, struct_receiver> Buffers;
Buffers buffers;

uint32_t getReceiverId(const uint8_t *macAddr){
    return macAddr[0]+macAddr[1]+macAddr[2]+macAddr[3]+macAddr[4]+macAddr[5];
}

bool mul_decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  /// store the initial bytes left for after update fpos
  int bytes_left = stream->bytes_left;
  if (!pb_read(stream, curReceiver->fb + curReceiver->fbpos, stream->bytes_left))
    return false;

  curReceiver->fbpos = curReceiver->fbpos + bytes_left;
  return true;
}

bool mulDecodeMessage(uint16_t message_length) {
  pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
  bool status;
  if (frameMetadata) {
    msg_recv_ext.data.funcs.decode = &mul_decode_data;
    status = pb_decode(&stream, FrameWifiRaw_fields, &msg_recv_ext);
  } else {
    msg_recv.data.funcs.decode = &mul_decode_data;
    status = pb_decode(&stream, Frame_fields, &msg_recv);
  }
  if (!status) {
    log_w("%s", PB_GET_ERROR(&stream));
    return false;
  }
  return true;
}

void msgReceiveCbByMAC(const uint8_t *macAddr, const uint8_t *data, int32_t dataLen) {
  // printMacAddress(macAddr); 
  // std::lock_guard<std::mutex> lck(recv_cb_mtx);
  uint32_t id = getReceiverId(macAddr);
  Buffers::const_iterator pos = buffers.find(id);
  if (pos == buffers.end()) {
    log_v("filter not registered!");
    return;
  } else {
    curReceiver = (struct_receiver*)(&pos->second);
    uint32_t msgLen = ((uint32_t)dataLen < sizeof(recv_buffer)) ? (uint32_t)dataLen
                                                               : (uint32_t)sizeof(recv_buffer);
    memcpy(recv_buffer, data, msgLen);
    if (mulDecodeMessage(msgLen) && decodedLenght() > 0) {
      if (curReceiver->recvCb != nullptr) curReceiver->recvCb(decodedLenght());
      const uint8_t *meta = nullptr;
      uint32_t metaLen = 0;
      if (metaCb != nullptr && decodedMeta(&meta, &metaLen)) metaCb(meta, metaLen);
      curReceiver->fbpos = 0;
    }
  }
}

bool checkReceiver(const uint8_t *macAddr) {
  uint32_t id = getReceiverId(macAddr);
  Buffers::const_iterator pos = buffers.find(id);
  std::map<uint32_t, std::string>::const_iterator iter;
  if (pos != buffers.end()) return true;
  return false;
}

void saveReceiver(const uint8_t *macAddr, uint8_t *fb, RecvCb cb) {
  if (!checkReceiver(macAddr)) {
    uint32_t id = getReceiverId(macAddr);
    log_i("[%02d] New receiverId: %i with MAC:", buffers.size()+1, id);
    printMacAddress(macAddr);
    struct_receiver rcv;
    rcv.fb = fb;
    rcv.fbpos = 0;
    rcv.recvCb = cb;
    buffers.insert(std::make_pair(id, rcv));
  }
}

/**
 * @brief Callback and framebuffer setter filtered by MAC address
 *        Only for multiple sources (N:1)
 * @param fb buffer pointer to receive the whole frame data
 * @param macAddr source device MAC address to filter
 * @param cb frame data callback for this source device
 */
void ESPNowCam::setRecvFilter(uint8_t *fb, const uint8_t *macAddr, RecvCb cb) {
  saveReceiver(macAddr, fb, cb);
}

/***********************************
 * C O M M O N  S E C T I O N
************************************/
void ESPNowCam::registerCallbacks() {
  // Only for receivers devices
  if (recvCb != nullptr)
    comm->registerRecvCallback(msgReceiveCb);
  else if (buffers.size() > 0)
    comm->registerRecvCallback(msgReceiveCbByMAC);
  // Default send callback
  comm->registerSendCallback(msgSentCb);
}

void ESPNowCam::setChannel(uint8_t channel) {
  _channel = channel;
}

bool ESPNowCam::init(uint16_t chunk_size) {
  /// the encoded chunk (payload + protobuf headers) must fit in the send buffer
  uint32_t max_chunk = (uint32_t)sizeof(send_buffer) - 16;
  if (chunk_size > max_chunk) {
    log_w("Chunk size %u does not fit in the send buffer, using %u..", chunk_size,
          (unsigned int)max_chunk);
    chunk_size = (uint16_t)max_chunk;
  }
  chunksize = chunk_size; 
  chunk_size_left = chunk_size;

  /// the schema of the protocol depends on the interface: the WiFi raw one
  /// uses the extended src/frame_wifiraw.proto (with the frame metadata), the
  /// old ESP-NOW one keeps the minimal src/frame.proto shared by the previous
  /// releases of the library
  frameMetadata = comm->supportsFrameMetadata();
  if (!frameMetadata && metaCb != nullptr) {
    log_w("This interface can not carry frame metadata, the metadata callback is ignored..");
  }

  if (_channel != -1) {
    log_i("Set custom channel: %i", _channel);
    comm->setChannel(_channel);
  }

  if (comm != &defaultESPNow) {
    printf("Using WiFiRawComm 80211tx interface :)\r\n");
    registerCallbacks();
  } else {
    printf("Using old EspNow implementation :(\r\n");
    WiFi.mode(WIFI_STA);
    log_i("ESPNowCam Init");
    log_i("%s", WiFi.macAddress().c_str());
    // shutdown wifi
    WiFi.disconnect();
    delay(100);
  }

  if (comm->init() == COMM_OK) {
    log_i("ESPNow Init Success");
    if (comm == &defaultESPNow) {
      registerCallbacks();
    }
    return true;
  } 
  else {
    log_e("ESPNow Init Failed");
    delay(100);
    ESP.restart();
    return false;
  }
}
