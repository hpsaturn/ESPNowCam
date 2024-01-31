/**************************************************
 * ESPNowCam Transmitter/Receiver unified class
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include "ESPNowCam.h"

/***********************************
 * S E N D E R  S E C T I O N
************************************/
uint8_t send_buffer[256];
uint8_t chunk_size = CHUNKSIZE;
uint32_t chunk_pos = 0;
uint8_t *outdata = NULL;
size_t outdata_len = 0;
bool msgReady = false;

bool sendMessage(uint32_t msglen, const uint8_t *mac);
size_t encodeMsg(Frame msg);

bool encode_uint8_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
  if (!pb_encode_tag_for_field(stream, field))
    return false;
  return pb_encode_string(stream, (uint8_t *)(outdata + chunk_pos), chunk_size);
}

void msgSentCb(const uint8_t *macAddr, esp_now_send_status_t  status) {
  msgReady = true;
}


bool ESPNowCam::sendData(uint8_t *data, uint32_t lenght) {
  outdata = data;
  outdata_len = lenght;

  uint32_t chunk_left = outdata_len;
  Frame msg = Frame_init_zero;
  while (chunk_left > 0) {
    if (chunk_left <= chunk_size) {
      chunk_size = chunk_left;
      msg.lenght = outdata_len;
    } else {
      chunk_left = chunk_left - chunk_size;
      msg.lenght = 0;
    }
    msg.data.funcs.encode = &encode_uint8_array;
    msgReady = false;
    sendMessage(encodeMsg(msg), this->targetAddress);
    while(!msgReady)delayMicroseconds(1);
    chunk_pos = outdata_len - chunk_left;
    if (msg.lenght == outdata_len) {
      chunk_left = 0;
      chunk_size = CHUNKSIZE;
    }
    // delay(4);
  }
  chunk_pos = 0;
  return true;
}

size_t encodeMsg(Frame msg) {
  pb_ostream_t stream = pb_ostream_from_buffer(send_buffer, sizeof(send_buffer));
  bool status = pb_encode(&stream, Frame_fields, &msg);
  size_t message_length = stream.bytes_written;
  if (!status) {
    printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
    return 0;
  }
  return message_length;
}

bool sendMessage(uint32_t msglen, const uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, mac, 6);
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_add_peer(&peerInfo);
  }
  esp_err_t result = esp_now_send(mac, send_buffer, msglen);

  if (result == ESP_OK) {
    // Serial.println("send msg success");
    return true;
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Unknown error");
  }
  return false;
}

bool ESPNowCam::setTarget(uint8_t *macAddress) {
  memcpy(targetAddress, macAddress, 6);
  return false;
}

/***********************************
 * S E N D E R  S E C T I O N
************************************/
uint8_t recv_buffer[256];
uint32_t fbpos = 0;
RecvCb recvCb = nullptr;
uint8_t *recvBuffer = nullptr;

Frame msg_recv = Frame_init_zero;

bool decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  /// store the initial bytes left for after update fpos
  int bytes_left = stream->bytes_left;

  if (!pb_read(stream, recvBuffer + fbpos, stream->bytes_left))
    return false;

  fbpos = fbpos + bytes_left;
  return true;
}

bool decodeMessage(uint16_t message_length) {
  pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
  msg_recv.data.funcs.decode = &decode_data;
  // msg.data.arg = (void*) "array: \"%d\"\r\n";
  bool status = pb_decode(&stream, Frame_fields, &msg_recv);
  if (!status) {
    Serial.printf("Decoding msg failed: %s\r\n", PB_GET_ERROR(&stream));
    return false;
  }
  return true;
}

void msgReceiveCb(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  memcpy(recv_buffer, data, msgLen);
  if (decodeMessage(msgLen) && msg_recv.lenght > 0) {
    recvCb(msg_recv.lenght);
    fbpos = 0;
  }
}

void ESPNowCam::setRecvCallback(RecvCb cb) {
  recvCb = cb;
}

void ESPNowCam::setRecvBuffer(uint8_t *fb) {
  recvBuffer = fb;
}

/***********************************
 * C O M M O N  S E C T I O N
************************************/

bool ESPNowCam::init() {
  WiFi.mode(WIFI_STA);
  Serial.println("ESPNow Init");
  Serial.println(WiFi.macAddress());
  // shutdown wifi
  WiFi.disconnect();
  delay(100);

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
    esp_now_register_recv_cb(msgReceiveCb);
    esp_now_register_send_cb(msgSentCb);
    return true;
  } else {
    Serial.println("ESPNow Init Failed");
    delay(100);
    ESP.restart();
    return false;
  }
}
