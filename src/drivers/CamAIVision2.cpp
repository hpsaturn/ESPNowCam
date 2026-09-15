/**************************************************
 * ESPNowCam AI Vision 2 camera driver
 * (Seeed Studio AI Vision 2 / Grove Vision AI V2 module, SSCMA driver)
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
 *
 * This driver wraps the Seeed_Arduino_SSCMA library:
 * https://github.com/Seeed-Studio/Seeed_Arduino_SSCMA
 *
 * The AI module owns the image sensor and the inference model, and it sends
 * every frame as a base64 encoded JPEG inside the SSCMA AT protocol events.
 * Here those events are decoded and exposed like any other camera driver of
 * this library: begin(), get(), free() and fb, where fb->format is
 * PIXFORMAT_JPEG and fb->buf is ready to be sent with ESPNowCam::sendData().
 *
 * The event payload is decoded with some tolerance on purpose: the module is
 * able to wrap the base64 data, to add a header or a tail to the JPEG, or to
 * cut the image when its internal buffer is smaller than the frame. Every
 * rejected frame is counted and the first ones are logged with all the
 * details, useful to know what the module is really sending.
 *
 * It only is compiled when the AIVISION2 build flag is defined, see the
 * xiao-ai-vision-sender environment on platformio.ini
**************************************************/

#ifdef AIVISION2
#include "CamAIVision2.h"

/// serial port used to talk with the AI module
static HardwareSerial aivisionSerial(AIVISION2_SERIAL_PORT);

/// frames are big and the internal RAM is needed by the radio and the AI
/// module buffers, so always try the PSRAM first
static void *frameAlloc(size_t size) {
#ifdef BOARD_HAS_PSRAM
  if (psramFound()) {
    void *ptr = ps_malloc(size);
    if (ptr != nullptr) {
      return ptr;
    }
  }
#endif
  return malloc(size);
}

static void frameDelete(camera_fb_t *frame) {
  if (frame == nullptr) {
    return;
  }
  if (frame->buf != nullptr) {
    ::free(frame->buf);
    frame->buf = nullptr;
  }
  ::free(frame);
}

static camera_fb_t *newFrame(uint8_t *jpeg, size_t len) {
  camera_fb_t *frame = (camera_fb_t *)calloc(1, sizeof(camera_fb_t));
  if (frame == nullptr) {
    return nullptr;
  }
  TickType_t ticks = xTaskGetTickCount();
  frame->buf = jpeg;
  frame->len = len;
  frame->format = PIXFORMAT_JPEG;
  frame->timestamp.tv_sec = ticks / configTICK_RATE_HZ;
  frame->timestamp.tv_usec = (ticks % configTICK_RATE_HZ) * 1e6 / configTICK_RATE_HZ;
  return frame;
}

/**************************************************
 * B A S E 6 4   A N D   H E X
 **************************************************/

static inline bool isBase64Char(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
         c == '+' || c == '/' || c == '=';
}

/// the module sends the image with the standard alphabet, but some firmwares
/// wrap it (line breaks) or escape it (URL safe chars), so here everything
/// that does not belong to the alphabet is dropped and the URL safe variants
/// are mapped to the standard ones. It is only called when the payload is not
/// a clean base64 string.
static size_t sanitizeBase64(const char *in, size_t in_len, char *out) {
  size_t n = 0;
  for (size_t i = 0; i < in_len; i++) {
    char c = in[i];
    if (c == '-') {
      c = '+';
    } else if (c == '_') {
      c = '/';
    }
    if (isBase64Char(c)) {
      out[n++] = c;
    }
  }
  out[n] = '\0';
  return n;
}

static bool isCleanBase64(const char *in, size_t in_len) {
  for (size_t i = 0; i < in_len; i++) {
    if (!isBase64Char(in[i])) {
      return false;
    }
  }
  return true;
}

static inline uint8_t hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return c - 'A' + 10;
}

/// some payloads are hex encoded instead of base64 (both are ASCII, but the
/// hex alphabet is a subset of the base64 one, so it is detected from the
/// payload itself and only used when the base64 decode has no JPEG inside)
static bool isHexString(const char *in, size_t in_len) {
  if (in_len < 8 || (in_len % 2) != 0) {
    return false;
  }
  for (size_t i = 0; i < in_len; i++) {
    char c = in[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
      return false;
    }
  }
  return true;
}

static size_t decodeHex(const char *in, size_t in_len, uint8_t *out) {
  size_t n = 0;
  for (size_t i = 0; i + 1 < in_len; i += 2) {
    out[n++] = (uint8_t)((hexValue(in[i]) << 4) | hexValue(in[i + 1]));
  }
  return n;
}

/**************************************************
 * J P E G   M A R K E R S
 **************************************************/

enum JpegStatus {
  JPEG_COMPLETE,   ///< the start and the end of image markers are there
  JPEG_INCOMPLETE, ///< the image starts, but the data ends before the end marker
  JPEG_INVALID,    ///< there is no JPEG inside the decoded data
};

/// the module always sends a JPEG, but some firmwares add a header (a size, a
/// timestamp, a thumbnail) to the image and some others cut the data when the
/// module buffer is smaller than the frame, so here the frame is cut to the
/// real image data instead of checking the first and the last bytes
static JpegStatus trimJpeg(uint8_t *buf, size_t *len) {
  const size_t size = *len;
  size_t start = size;
  size_t end = 0;
  bool complete = false;

  for (size_t i = 0; i + 2 < size; i++) {
    if (buf[i] == 0xFF && buf[i + 1] == 0xD8 && buf[i + 2] == 0xFF) {
      start = i;
      break;
    }
  }
  if (start == size) {
    return JPEG_INVALID;
  }
  for (size_t i = start + 3; i + 1 < size; i++) {
    if (buf[i] == 0xFF && buf[i + 1] == 0xD9) {
      end = i + 2;
      complete = true;
      break;
    }
  }
  /// the image can not start at the beginning of the payload (a header, a
  /// thumbnail or a size added by the module) and it can not end at the end
  /// of it (a tail or the end of a truncated frame), so the frame is cut
  size_t available = size - start;
  if (start > 0) {
    memmove(buf, buf + start, available);
  }
  *len = complete ? (end - start) : available;
  return complete ? JPEG_COMPLETE : JPEG_INCOMPLETE;
}

/// the size of the frame is only known by the JPEG itself (the module does not
/// report the width and the height on the event)
static bool jpegDimensions(const uint8_t *buf, size_t len, size_t *width, size_t *height) {
  if (buf == nullptr || len < 10 || buf[0] != 0xFF || buf[1] != 0xD8) {
    return false;
  }
  size_t pos = 2;
  while (pos + 9 < len) {
    if (buf[pos] != 0xFF) {
      pos++;
      continue;
    }
    uint8_t marker = buf[pos + 1];
    if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
      pos += 2;
      continue;
    }
    if (marker == 0xDA) {
      break; // start of scan, no dimensions on the header
    }
    size_t size = ((size_t)buf[pos + 2] << 8) | buf[pos + 3];
    if (size < 2) {
      return false;
    }
    // SOF0..SOF15 but not the DHT/JPG/DAC ones
    if (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
      *height = ((size_t)buf[pos + 5] << 8) | buf[pos + 6];
      *width = ((size_t)buf[pos + 7] << 8) | buf[pos + 8];
      return true;
    }
    pos += 2 + size;
  }
  return false;
}

/**************************************************
 * S S C M A   E V E N T S
 **************************************************/

/// SSCMA event messages have "type": 1 (the replies and the logs are ignored)
static bool isEvent(const char *resp, size_t len) {
  const char *end = resp + len;
  const char *key = strnstr(resp, MSG_TYPE_KEY, len);
  if (key == nullptr) {
    return false;
  }
  const char *pos = key + MSG_TYPE_KEY_LEN;
  while (pos < end && (*pos == ' ' || *pos == '\t')) pos++;
  if (pos >= end || *pos != ':') {
    return false;
  }
  pos++;
  while (pos < end && (*pos == ' ' || *pos == '\t')) pos++;
  return pos < end && *pos == MSG_EVENT_TYPE[0];
}

/// look for the base64 image data of the event, tolerating any JSON spacing
static const char *findImageData(const char *resp, size_t len, size_t *b64_len) {
  const char *end = resp + len;
  const char *key = resp;
  while ((key = strnstr(key, MSG_IMAGE_KEY, end - key)) != nullptr) {
    const char *pos = key + MSG_IMAGE_KEY_LEN;
    while (pos < end && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n')) pos++;
    if (pos < end && *pos == ':') {
      pos++;
      while (pos < end && (*pos == ' ' || *pos == '\t')) pos++;
      if (pos < end && *pos == MSG_QUOTE_STR[0]) {
        pos++;
        const char *close = strnstr(pos, MSG_QUOTE_STR, end - pos);
        if (close == nullptr) {
          return nullptr;
        }
        *b64_len = close - pos;
        return pos;
      }
    }
    key = pos;
  }
  return nullptr;
}

/**************************************************
 * F R A M E   D E C O D I N G
 **************************************************/

static void dumpHex(char *out, size_t out_size, const uint8_t *buf, size_t len) {
  size_t used = 0;
  for (size_t i = 0; i < len && used + 4 < out_size; i++) {
    used += snprintf(out + used, out_size - used, "%02X ", buf[i]);
  }
  out[used] = '\0';
}

/// the dropped frames are noisily logged (the console is not able to follow
/// hundreds of messages per second), the first ones with all the payload
/// details, after that only the summary of each 100 frames
void CamAIVision2::reportRejected(const char *reason, const char *b64, size_t b64_len,
                                  const uint8_t *jpeg, size_t jpeg_len) {
  _rejected++;
  bool details = (_rejected <= AIVISION2_DEBUG_FRAMES) || (_rejected % 100 == 0);
  if (!details) {
    if (_rejected == AIVISION2_DEBUG_FRAMES + 1) {
      log_w("%u frames rejected (%s), muting the per frame details, see rejected()",
            (unsigned int)_rejected, reason);
      if (strstr(reason, "truncated") != nullptr) {
        log_w("The module is not able to keep the whole image, try a lower resolution, "
              "e.g. Camera.setResolution(0) [240x240]");
      }
    }
    return;
  }

  char head[3 * 16 + 1] = {0};
  char tail[3 * 8 + 1] = {0};
  log_w("Frame rejected (%s) #%u: base64 %u chars, decoded %u bytes", reason,
        (unsigned int)_rejected, (unsigned int)b64_len, (unsigned int)jpeg_len);
  if (jpeg != nullptr && jpeg_len > 0) {
    dumpHex(head, sizeof(head), jpeg, jpeg_len < 16 ? jpeg_len : 16);
    dumpHex(tail, sizeof(tail), jpeg + (jpeg_len > 8 ? jpeg_len - 8 : 0), jpeg_len > 8 ? 8 : jpeg_len);
    log_w("  decoded head: %s", head);
    log_w("  decoded tail: %s", tail);
  }
  /// this shows what the module really sends, a JPEG starts with /9j/4 (FF D8 FF)
  log_w("  base64 head: %.*s", (int)(b64_len < 48 ? b64_len : 48), b64);
  log_w("  base64 tail: %.*s (len%%4=%u)", (int)(b64_len < 16 ? b64_len : 16),
        b64 + (b64_len < 16 ? 0 : b64_len - 16), (unsigned int)(b64_len % 4));
}

void CamAIVision2::onResponse(const char *resp, size_t len) {
  if (resp == nullptr || len == 0) {
    return;
  }
  if (!isEvent(resp, len)) {
    return; // command replies and logs have no frame data
  }

  size_t b64_len = 0;
  const char *data = findImageData(resp, len, &b64_len);
  if (data == nullptr || b64_len < 8) {
    return; // events like the inference results without image data
  }

  /// the base64 payload never contains a comma, so a comma at the beginning
  /// means a data URI (or a similar header) and the data starts after it
  for (size_t i = 0; i < b64_len && i < 32; i++) {
    if (data[i] == ',') {
      data += i + 1;
      b64_len -= i + 1;
      break;
    }
  }
  if (b64_len < 8) {
    return;
  }

  /// the head of the first events is logged to know what the module is really
  /// sending (a base64 JPEG starts with /9j/4)
  if (_events < AIVISION2_DEBUG_FRAMES) {
    _events++;
    const char *from = (resp[0] == '\r') ? resp + 1 : resp;
    size_t head = (size_t)(data - from) + 16;
    log_i("event payload: %.*s", (int)(head > 96 ? 96 : head), from);
  }

  /// the base64 length is always the upper bound of the decoded data
  size_t capacity = ((b64_len / 4) * 3) + 4;
  if (capacity > AIVISION2_MAX_JPEG) {
    reportRejected("frame too big", data, b64_len, nullptr, 0);
    return;
  }

  uint8_t *jpeg = (uint8_t *)frameAlloc(capacity);
  if (jpeg == nullptr) {
    log_e("Failed to allocate the frame buffer..");
    return;
  }

  /// the sanitized copy is only needed when the payload is not a clean base64
  const char *source = data;
  size_t source_len = b64_len;
  char *sanitized = nullptr;
  if (!isCleanBase64(data, b64_len)) {
    sanitized = (char *)frameAlloc(b64_len + 1);
    if (sanitized == nullptr) {
      log_e("Failed to allocate the frame buffer..");
      ::free(jpeg);
      return;
    }
    source_len = sanitizeBase64(data, b64_len, sanitized);
    source = sanitized;
  }

  size_t jpeg_len = 0;
  int ret = -1;
  if (source_len >= 8) {
    ret = mbedtls_base64_decode(jpeg, capacity, &jpeg_len, (const unsigned char *)source, source_len);
  }
  JpegStatus status = (ret == 0) ? trimJpeg(jpeg, &jpeg_len) : JPEG_INVALID;

  /// a hex encoded payload is ASCII too, so it decodes as base64 without
  /// errors, but there is no JPEG inside
  if (status == JPEG_INVALID && isHexString(data, b64_len)) {
    jpeg_len = decodeHex(data, b64_len, jpeg);
    status = trimJpeg(jpeg, &jpeg_len);
  }
  if (sanitized != nullptr) {
    ::free(sanitized);
  }

  if (status == JPEG_INVALID) {
    reportRejected(ret != 0 ? "base64 decode error" : "no JPEG data", data, b64_len, jpeg, jpeg_len);
    ::free(jpeg);
    return;
  }
  if (status == JPEG_INCOMPLETE) {
    reportRejected("truncated JPEG, the module sent an incomplete frame", data, b64_len, jpeg,
                   jpeg_len);
#if AIVISION2_STRICT_JPEG
    ::free(jpeg);
    return;
#endif
  }

  camera_fb_t *frame = newFrame(jpeg, jpeg_len);
  if (frame == nullptr) {
    log_e("Failed to allocate the frame descriptor..");
    ::free(jpeg);
    return;
  }
  jpegDimensions(jpeg, jpeg_len, &frame->width, &frame->height);
  log_v("new frame: %u bytes %ux%u", (unsigned int)frame->len, (unsigned int)frame->width,
        (unsigned int)frame->height);
  pushFrame(frame);
}

void CamAIVision2::pushFrame(camera_fb_t *frame) {
  while (_frames.size() >= AIVISION2_QUEUE_LIMIT) {
    frameDelete(_frames.front());
    _frames.pop_front();
    _discarded++;
  }
  _frames.push_back(frame);
}

void CamAIVision2::dropFrames() {
  while (!_frames.empty()) {
    frameDelete(_frames.front());
    _frames.pop_front();
  }
}

CamAIVision2::CamAIVision2() {
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // the real one is set on the module
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  fb = nullptr;
  sensor = nullptr;
}

/// the SSCMA library resizes its internal buffer with realloc(), and on
/// failure it leaves the driver without buffer, so here we only resize it if
/// the internal RAM has room enough, otherwise its default is used
void CamAIVision2::reserveResponseBuffer() {
  size_t want = AIVISION2_RESP_BUFFER;
  if (want <= AIVISION2_MIN_RESP_BUFFER) {
    return;
  }
  size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  while (want > AIVISION2_MIN_RESP_BUFFER && (want + 4096) > largest) {
    want /= 2;
  }
  if (want <= AIVISION2_MIN_RESP_BUFFER) {
    log_w("Not enough internal RAM for a big response buffer, using the default..");
    return;
  }
  if (!_module.set_rx_buffer(want)) {
    log_e("Failed to set the response buffer..");
  } else {
    log_i("Response buffer: %u bytes", (unsigned int)want);
  }
}

bool CamAIVision2::begin(uint32_t baud) {
  if (_streaming) {
    log_w("The stream is already running..");
    return true;
  }
  log_i("Starting AI Vision 2 interface..");
  aivisionSerial.setRxBufferSize(AIVISION2_UART_BUFFER);
  if (AIVISION2_PIN_RX >= 0 && AIVISION2_PIN_TX >= 0) {
    aivisionSerial.begin(baud, SERIAL_8N1, AIVISION2_PIN_RX, AIVISION2_PIN_TX);
  }
  if (!_module.begin(&aivisionSerial, AIVISION2_PIN_RST, baud)) {
    log_e("AI Vision 2 module not found..");
    return false;
  }
  log_i("AI Vision 2 module: %s", _module.name());
  reserveResponseBuffer();
  if (!applyResolution()) {
    log_w("Failed to set the sensor resolution..");
  }
  return startStream();
}

bool CamAIVision2::get() {
  if (fb != nullptr) {
    log_w("The current frame was not released, did you forget Camera.free()?");
    free();
  }
  /// fetch() pumps the serial port, the frames are decoded inside the callback
  _module.fetch([this](const char *resp, size_t len) { onResponse(resp, len); });
  if (_frames.empty()) {
    return false;
  }
  fb = _frames.front();
  _frames.pop_front();
  return true;
}

bool CamAIVision2::free() {
  if (fb == nullptr) {
    return false;
  }
  frameDelete(fb);
  fb = nullptr;
  return true;
}

bool CamAIVision2::sendCmd(const char *command, int len) {
  if (command == nullptr || len <= 0) {
    return false;
  }
  if (_module.write(CMD_PREFIX, strlen(CMD_PREFIX)) != (int)strlen(CMD_PREFIX)) {
    log_e("Failed to write the command prefix..");
    return false;
  }
  if (_module.write(command, len) != len) {
    log_e("Failed to write the command..");
    return false;
  }
  if (_module.write(CMD_SUFFIX, strlen(CMD_SUFFIX)) != (int)strlen(CMD_SUFFIX)) {
    log_e("Failed to write the command suffix..");
    return false;
  }
  log_v("sent command: %s%.*s", CMD_PREFIX, len, command);
  return true;
}

bool CamAIVision2::startStream(int times, bool differed, bool resultOnly) {
  char command[32] = {0};
  snprintf(command, sizeof(command), "%s=%d,%d,%d", CMD_AT_INVOKE, times, differed ? 1 : 0,
           resultOnly ? 1 : 0);
  _streaming = sendCmd(command, strlen(command));
  if (_streaming) {
    log_i("Streaming started: %s", command);
  } else {
    log_e("Failed to start the stream..");
  }
  return _streaming;
}

bool CamAIVision2::stopStream() {
  _streaming = false;
  dropFrames();
  return sendCmd(CMD_AT_BREAK, strlen(CMD_AT_BREAK));
}

bool CamAIVision2::setResolution(uint16_t optId, int sensorId) {
  _sensorOptId = optId;
  _sensorId = sensorId;
  if (!_streaming) {
    return true; // it is applied on begin()
  }
  return applyResolution();
}

bool CamAIVision2::applyResolution() {
  if (_sensorOptId < 0) {
    return true;
  }
  char command[24] = {0};
  snprintf(command, sizeof(command), "%s=%d,1,%d", CMD_AT_SENSOR, _sensorId, _sensorOptId);
  log_i("Sensor resolution: %s", command);
  return sendCmd(command, strlen(command));
}

size_t CamAIVision2::available() { return _frames.size(); }

size_t CamAIVision2::discarded() { return _discarded; }

size_t CamAIVision2::rejected() { return _rejected; }

#endif
