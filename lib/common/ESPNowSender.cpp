/**************************************************
 * ESP32Cam Freenove ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include "ESPNowSender.h"

uint8_t send_buffer[256];
uint8_t chunk_size = CHUNKSIZE;
uint32_t chunk_pos = 0;

uint8_t *outdata = NULL;
size_t outdata_len = 0;

bool sendMessage(uint32_t msglen, const uint8_t *mac);
size_t encodeMsg(Frame msg);

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

bool encode_uint8_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
  if (!pb_encode_tag_for_field(stream, field))
    return false;
  return pb_encode_string(stream, (uint8_t *)(outdata + chunk_pos), chunk_size);
}

bool ESPNowSender::sendData(uint8_t *data, uint32_t lenght) {
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
    sendMessage(encodeMsg(msg), this->targetAddress);
    chunk_pos = outdata_len - chunk_left;
    if (msg.lenght == outdata_len) {
      chunk_left = 0;
      chunk_size = CHUNKSIZE;
    }
    delay(4);
  }
  chunk_pos = 0;
  // Serial.printf("\r\nFrame encoded lenght: %u cheksum: %u\r\n", bytecount, framesum);
  // bytecount = 0;
  // framesum = 0;
  // while(1);
  return true;
}

bool ESPNowSender::setTarget(uint8_t *macAddress) {
  memcpy(targetAddress, macAddress, 6);
  return false;
}

bool ESPNowSender::init() {
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  // shutdown wifi
  WiFi.disconnect();
  delay(100);

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
    return true;
  } else {
    Serial.println("ESPNow Init Failed");
    delay(100);
    return false;
  }
}
