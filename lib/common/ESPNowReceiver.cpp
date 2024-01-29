/**************************************************
 * ESP32Cam video ESPNow Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include "ESPNowReceiver.h"

/// general buffer for receive msgs
uint8_t recv_buffer[256];

Frame msg = Frame_init_zero;

uint32_t fbpos = 0;
RecvCb recvCb = nullptr;
uint8_t *recvBuffer = nullptr;

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
  msg.data.funcs.decode = &decode_data;
  // msg.data.arg = (void*) "array: \"%d\"\r\n";
  bool status = pb_decode(&stream, Frame_fields, &msg);
  if (!status) {
    Serial.printf("Decoding msg failed: %s\r\n", PB_GET_ERROR(&stream));
    return false;
  }
  return true;
}

void msgReceiveCb(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);

  memcpy(recv_buffer, data, msgLen);
  if (decodeMessage(msgLen) && msg.lenght > 0) {
    recvCb(msg.lenght);
    fbpos = 0;
  }
}

bool ESPNowReceiver::init() {
  WiFi.mode(WIFI_STA);
  Serial.println("ESPNow Init");
  Serial.println(WiFi.macAddress());
  // shutdown wifi
  WiFi.disconnect();
  delay(100);

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
    esp_now_register_recv_cb(msgReceiveCb);
    return true;
  } else {
    Serial.println("ESPNow Init Failed");
    delay(100);
    ESP.restart();
    return false;
  }
}

void ESPNowReceiver::setRecvCallback(RecvCb cb) {
  recvCb = cb;
}

void ESPNowReceiver::setRecvBuffer(uint8_t *fb) {
  recvBuffer = fb;
}