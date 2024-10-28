

static inline void printDataChksum(uint8_t * jpg_data, uint32_t jpg_len){
  uint32_t checksum = 0;
  Serial.println("JPG Frame:");
  for (int i = 0; i < jpg_len; i++) {
    checksum = checksum + (uint8_t) jpg_data[i];
    Serial.printf("%i ", jpg_data[i]);
  }
  Serial.printf("\r\nJPG lenght: %u cheksum: %u\r\n", jpg_len, checksum);
}

static uint16_t frame = 0;

static inline void printFPS(const char *msg) {
  static uint_least64_t timeStamp = 0;
  frame++;
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();
    Serial.printf("%s %2d FPS\r\n",msg, frame);
    frame = 0;
  } 
}