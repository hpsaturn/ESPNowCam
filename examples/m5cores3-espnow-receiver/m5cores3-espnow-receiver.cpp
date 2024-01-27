#include <M5CoreS3.h>
#include <WiFi.h>
#include <esp_now.h>
#include <pb_decode.h>
#include "frame.pb.h"

int32_t dw, dh;

/// general buffer for receive msgs
uint8_t recv_buffer[256];
/// frame buffer
// uint8_t fb[3000];
uint8_t *fb; 

Frame msg = Frame_init_zero;


uint32_t fbpos = 0;
uint32_t cksum = 0;

bool decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  uint64_t value;
  if (!pb_decode_varint(stream, &value))
    return false;
    cksum = cksum + (uint8_t) value;
  fb[fbpos++] = (uint8_t) value;
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

void printFB(uint32_t len){
  for (int32_t i = 0; i < len; i++) {
    Serial.printf("%i ",fb[i]);
  }
}

void msgReceiveCb(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  memcpy(recv_buffer, data, msgLen);
  if (decodeMessage(msgLen)) {
    // Serial.println();
    if (msg.lenght > 0 ) {
      // printFB(msg.lenght);
      // Serial.printf("\r\nfb size: %i msg lenght: %i cksum: %u\r\n",fbpos,msg.lenght,cksum);
      CoreS3.Display.drawJpg(fb, msg.lenght , 0, 0, dw, dh);
      printFPS("ESPNow reception at:");
      fbpos = 0;
      cksum = 0;
    }
    // Serial.printf("chunk len: %d\r\n",msg.lenght);
  }
}

bool ESPNowInit() {
    WiFi.mode(WIFI_STA);
    // startup ESP Now
    Serial.println("ESPNow Init");
    Serial.println(WiFi.macAddress());
    // shutdown wifi
    WiFi.disconnect();
    delay(100);

    if (esp_now_init() == ESP_OK) { 
        Serial.println("ESPNow Init Success");
        esp_now_register_recv_cb(msgReceiveCb);
        Serial.println("Registered receiver callback");
        // esp_now_register_send_cb(telemetrySendCallback);
        return true;
    } else {
        Serial.println("ESPNow Init Failed");
        delay(100);
        ESP.restart();
        return false;
    }
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  CoreS3.begin(cfg);
  CoreS3.Display.setTextColor(GREEN);
  CoreS3.Display.setTextDatum(middle_center);
  CoreS3.Display.setFont(&fonts::Orbitron_Light_24);
  CoreS3.Display.setTextSize(1);

  WiFi.mode(WIFI_STA);
  // startup ESP Now
  Serial.println("ESPNow Init");
  Serial.println(WiFi.macAddress());
  // shutdown wifi
  WiFi.disconnect();
  delay(100);

  dw = CoreS3.Display.width();
  dh = CoreS3.Display.height();

  if (ESPNowInit()) {
    CoreS3.Display.drawString("ESPNow Init Success", dw / 2, dh / 2);
  } 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  fb = (uint8_t*)  malloc(3500* sizeof( uint8_t ) ) ;

  delay(1000);
}

void loop() {
}
