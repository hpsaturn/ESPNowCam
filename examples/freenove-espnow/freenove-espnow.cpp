/**************************************************
 * ESP32Cam Freenove ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <pb_encode.h>
#include "CamFreenove.h"
#include "frame.pb.h"

#define CHUNKSIZE 87

CamFreenove Camera;

/// general buffer for msg sender
uint8_t send_buffer[256];

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  // shutdown wifi
  WiFi.disconnect();
  delay(100);

  while (esp_now_init() != ESP_OK) {
    Serial.print(".");
  } 
  Serial.println("\r\nESPNow Init Success");

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  Camera.sensor->set_framesize(Camera.sensor, FRAMESIZE_QVGA); 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  delay(500);
}

uint16_t frame = 0;

void printFPS(const char *msg) {
  static uint_least64_t timeStamp = 0;
  frame++;
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();
    Serial.printf("%s %2d FPS\r\n",msg, frame);
    frame = 0;
  } 
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

uint8_t targetAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t *out_jpg = NULL;
size_t out_jpg_len = 0;
uint8_t chunk_size = CHUNKSIZE;
uint32_t chunk_pos = 0;
uint32_t framesum = 0;
uint32_t bytecount = 0;


bool encode_uint8_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
  for (int i = 0; i < chunk_size; i++) {
    if (!pb_encode_tag_for_field(stream, field))
      return false;
    uint8_t val = (out_jpg+chunk_pos)[i];
    // framesum = framesum + val;
    // Serial.printf("%i ", val);
    // bytecount++;
    if (!pb_encode_varint(stream, val))
      return false;
  }
  // Serial.printf("%i ", chunk_pos);
  return true;
}

void dispatchFrame() {
  // Serial.println("Encoded Frame:");
  uint32_t chunk_left = out_jpg_len;
  Frame msg = Frame_init_zero;
  while (chunk_left > 0) {
    if (chunk_left <= chunk_size) {
      chunk_size = chunk_left;
      msg.lenght = out_jpg_len;
    }
    else {
      chunk_left = chunk_left - chunk_size;
      msg.lenght = 0;
    }
    msg.data.funcs.encode = &encode_uint8_array;
    sendMessage(encodeMsg(msg), targetAddress);
    chunk_pos = out_jpg_len - chunk_left;
    if (msg.lenght == out_jpg_len) {
      chunk_left = 0;
      chunk_size = CHUNKSIZE;
    }
    delay(4);
  }
  chunk_pos = 0;
  // Serial.printf("\r\nFrame encoded lenght: %u cheksum: %u\r\n", bytecount, framesum);
  // bytecount = 0;
  // framesum = 0;
}

void printJPGFrame(){
  uint32_t checksum = 0;
  Serial.println("JPG Frame:");
  for (int i = 0; i < out_jpg_len; i++) {
    checksum = checksum + (uint8_t) out_jpg[i];
    Serial.printf("%i ", out_jpg[i]);
  }
  Serial.printf("\r\nJPG lenght: %u cheksum: %u\r\n", out_jpg_len, checksum);
}

void processFrame() {
  if (Camera.get()) {
    frame2jpg(Camera.fb, 9, &out_jpg, &out_jpg_len);
    // Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
    // printJPGFrame();
    // Serial.println();
    dispatchFrame();
    // Serial.println(out_jpg_len);
    // printFPS("transmitting at ");
    free(out_jpg);
    Camera.free();
  }
}

void loop() {
  processFrame();
}
