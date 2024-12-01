#include "CamIAVision2.h"

PtrBuffer PB;
StatInfo SI;
SSCMA AI;

inline uint16_t getMsgType(const char* resp, size_t len) {
  uint16_t type = MSG_TYPE_UNKNOWN;

  if (strnstr(resp, MSG_REPLY_STR, len) != NULL) {
    type |= MSG_TYPE_REPLY;
  } else if (strnstr(resp, MSG_EVENT_STR, len) != NULL) {
    type |= MSG_TYPE_EVENT;
  } else if (strnstr(resp, MSG_LOGGI_STR, len) != NULL) {
    type |= MSG_TYPE_LOGGI;
  } else {
    log_w("Unknown message type...");
  }

  return type;
}

inline uint16_t getCmdType(const char* resp, size_t len) {
  uint16_t type = CMD_TYPE_UNKNOWN;

  if (strnstr(resp, CMD_SAMPLE_STR, len) != NULL) {
    type |= CMD_TYPE_SAMPLE;
  } else if (strnstr(resp, CMD_INVOKE_STR, len) != NULL) {
    type |= CMD_TYPE_INVOKE;
  }

  return type;
}
static void proxyCallback(const char* resp, size_t len) {
  log_i("proxy_callback");
  static timeval timestamp;
  TickType_t ticks = xTaskGetTickCount();
  timestamp.tv_sec = ticks / configTICK_RATE_HZ;
  timestamp.tv_usec = (ticks % configTICK_RATE_HZ) * 1e6 / configTICK_RATE_HZ;

  if (!len) {
    log_i("Response is empty...");
    return;
  }

  uint16_t type = 0;
  type |= getMsgType(resp, len);
  if (type == MSG_TYPE_UNKNOWN) {
    return;
  }
  type |= getCmdType(resp, len);

  char* copy = (char*)malloc(len);
  if (copy == NULL) {
    log_e("Failed to allocate resp copy...");
    return;
  }
  memcpy(copy, resp, len);

  size_t limit = PB.limit;
  PtrBuffer::Slot* p_slot = (PtrBuffer::Slot*)malloc(sizeof(PtrBuffer::Slot));
  if (p_slot == NULL) {
    log_e("Failed to allocate slot...");
    return;
  }

  p_slot->id = PB.id;
  p_slot->type = type;
  p_slot->data = copy;
  p_slot->size = len;
  p_slot->timestamp = timestamp;

  const char* slice = strnstr((const char*)p_slot->data, MSG_IMAGE_KEY MSG_QUOTE_STR, p_slot->size);
  if (slice == NULL) {
    log_w("No image data found...");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return;
  }
  size_t offset = (slice - (const char*)p_slot->data) + strlen(MSG_IMAGE_KEY MSG_QUOTE_STR);
  const char* data = (const char*)p_slot->data + offset;
  const char* quote = strnstr(data, MSG_QUOTE_STR, p_slot->size - offset);
  if (quote == NULL) {
    log_w("Invalid image data size...");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return;
  }
  size_t ilen = quote - data;
  if (ilen == 0) {
    log_w("Empty image data...");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return;
  }

  size_t jpeg_size = 0;
  char* jpeg_buf = NULL;
  jpeg_buf = (char*)malloc(JPG_BUFFER_SIZE);
  if (jpeg_buf == NULL) {
    log_e("Failed to allocate jpeg buffer...");
    return;
  }
  memset(jpeg_buf, 0, JPG_BUFFER_SIZE);
  if (mbedtls_base64_decode((unsigned char*)jpeg_buf, JPG_BUFFER_SIZE, &jpeg_size,
                            (const unsigned char*)data, len) != 0) {
    log_e("Failed to decode image data...");
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return;
  }
  log_i("decoded JPG!");

  size_t discarded = 0;
  xSemaphoreTake(PB.mutex, portMAX_DELAY);
  while (PB.slots.size() >= limit) {
    PB.slots.pop_front();
    discarded += 1;
  }
  PB.slots.emplace_back(std::shared_ptr<PtrBuffer::Slot>(p_slot, [](PtrBuffer::Slot* p) {
    if (p == NULL) {
      return;
    }
    if (p->data != NULL) {
      free(p->data);
      p->data = NULL;
    }
    free(p);
  }));
  xSemaphoreGive(PB.mutex);
  PB.id += 1;

  if (discarded > 0) {
    log_i("Discarded %u old responses...", discarded);
  }

  log_i("Received %u bytes...", len);
}

bool CamIAVision2::get() {
  if (!AI.invoke(-1,0,0)) {
    AI.fetch(proxyCallback);
    return true;
  }
  else return false;
}

bool CamIAVision2::sendCmd(const char * command, int len) {

  TickType_t ticks = xTaskGetTickCount();
  char cmd_tag_buf[32] = {0};
  size_t cmd_tag_size = snprintf(cmd_tag_buf, sizeof(cmd_tag_buf), CMD_TAG_FMT_STR, ticks);
  // INVOKE=-1,0,0
  log_i("cmd_buf: [%i] %.*s\r\n",command, len, command);
  AI.write(CMD_PREFIX, strlen(CMD_PREFIX));
  AI.write(cmd_tag_buf, cmd_tag_size);
  AI.write(command, len);
  AI.write(CMD_SUFFIX, strlen(CMD_SUFFIX));
  return true;
}

bool CamIAVision2::begin() {
  static HardwareSerial atSerial(0);
  // the esp32 arduino library may have a bug in setRxBufferSize
  // we cannot set the buffer size larger than uint16_t max value
  // a workaround is to modify uartBegin() in
  //     esp32/hardware/esp32/2.0.14/cores/esp32/esp32-hal-uart.c
  atSerial.setRxBufferSize(COM_BUFFER_SIZE);
  log_i("starting AI vision Serial interface..");
  return AI.begin(&atSerial, D3);
}