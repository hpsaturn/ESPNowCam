#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <pb_encode.h>
#include "CamFreenove.h"
#include "frame.pb.h"

#define CONVERT_TO_JPEG

CamFreenove Camera;

/// general buffer for msg sender
uint8_t send_buffer[256];

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(2000);

  WiFi.mode(WIFI_STA);
  // startup ESP Now
  Serial.println("ESPNow Init");
  Serial.println(WiFi.macAddress());
  // shutdown wifi
  WiFi.disconnect();
  delay(100);

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    delay(100);
    ESP.restart();
  }

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  Camera.sensor->set_framesize(Camera.sensor, FRAMESIZE_QVGA); 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  delay(1000);
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
const uint8_t chunk_size = 30;

bool encode_uint8_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
  uint8_t size = chunk_size / sizeof(uint8_t);
  for (int i = 0; i < size; i++) {
    if (!pb_encode_tag_for_field(stream, field))
      return false;
    Serial.printf("%i ", out_jpg[i]);
    if (!pb_encode_varint(stream, out_jpg[i]))
      return false;
  }
  return true;
}

void dispatchFrame() {
  uint32_t chunk_left = out_jpg_len;
  // while (chunk_left > 0) {

  // }
  Frame msg = Frame_init_zero;
  msg.chunk_left = out_jpg_len;
  msg.chunk_max = out_jpg_len;
  msg.lenght = chunk_size;
  msg.data.funcs.encode = &encode_uint8_array;
  sendMessage(encodeMsg(msg), targetAddress);
}

void printJPGFrame(){

  uint32_t checksum = 0;
  for (int i = 0; i < out_jpg_len; i++) {
    checksum = checksum + out_jpg[i];
    // Serial.printf("%i ", out_jpg[i]);
  }
  Serial.println();
  Serial.printf("framesum: %u\r\n", checksum);
}

void processFrame() {
  if (Camera.get()) {
#ifdef CONVERT_TO_JPEG
    frame2jpg(Camera.fb, 64, &out_jpg, &out_jpg_len);
    // printFPS("JPG compression at");
    dispatchFrame();
    Serial.println();
    // printJPGFrame();
    free(out_jpg);
    // Display.drawJpg(out_jpg, out_jpg_len, 0, 0, dw, dh);
#else
    printFPS("frame ready at");
    // Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
#endif
    Camera.free();
  }
}

void loop() {
  processFrame();
  // delay(1000);
}
