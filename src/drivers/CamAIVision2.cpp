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
 * cut the image when its buffer or its link is smaller than the frame. Every
 * bad frame is counted and the first ones are logged with all the details,
 * useful to know what the module is really sending.
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

/// when the module can not complete an image it leaves the rest of its frame
/// buffer untouched: those bytes are not decodable data and they only bother
/// the JPEG decoder of the receiver, so they are removed here
static size_t trimPadding(uint8_t *buf, size_t len) {
  size_t n = len;
  while (n > 3 && buf[n - 1] == 0x00) {
    n--;
  }
  return n;
}

/// the receivers need the end of image marker to paint a frame, so the missing
/// one is added: the decoders paint what they have and stop at the end of the
/// data instead of discarding the whole frame
static bool closeJpeg(uint8_t *buf, size_t *len, size_t capacity) {
  if (*len + 2 > capacity) {
    return false;
  }
  buf[(*len)++] = 0xFF;
  buf[(*len)++] = 0xD9;
  return true;
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

/// the SSCMA library hands over everything that it finds between "\r{" and
/// "}\n", but when the module (or the link) loses the tail of a message, the
/// payload holds the cut message followed by the next complete one. The image
/// of the newest message is the useful one, so the payload is parsed from its
/// last message start. The base64 data can not hold '\r' nor '{', so looking
/// for those two bytes is enough and it can not confuse the image data.
static const char *lastMessageStart(const char *resp, size_t len) {
  for (size_t i = len; i > 1; i--) {
    if (resp[i - 1] == '{' && resp[i - 2] == '\r') {
      return resp + i - 2;
    }
  }
  return resp;
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
void CamAIVision2::logFrameIssue(const char *reason, const char *b64, size_t b64_len,
                                 const uint8_t *jpeg, size_t jpeg_len) {
  size_t issues = _rejected + _partial;
  if (issues == AIVISION2_DEBUG_FRAMES + 1) {
    log_w("%u bad frames so far, muting the per frame details, see rejected() and partial()",
          (unsigned int)issues);
  }
  if (issues > AIVISION2_DEBUG_FRAMES && (issues % 100) != 0) {
    return;
  }

  char head[3 * 16 + 1] = {0};
  char tail[3 * 8 + 1] = {0};
  log_w("Frame issue (%s) #%u: base64 %u chars, decoded %u bytes", reason, (unsigned int)issues,
        (unsigned int)b64_len, (unsigned int)jpeg_len);
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

/// the module answers with "type": 0 messages (SENSOR?, VER?, ID?, ...), they
/// are logged when the command was sent by the user or by the driver itself,
/// it is the only way to know if a command was applied by the module
void CamAIVision2::onReply(const char *resp, size_t len) {
  if (!_logNextReply) {
    return;
  }
  _logNextReply = false;
  const char *from = (len > 0 && resp[0] == '\r') ? resp + 1 : resp;
  size_t size = len - (size_t)(from - resp);
  log_i("module reply: %.*s", (int)(size < AIVISION2_REPLY_LOG ? size : AIVISION2_REPLY_LOG), from);
}

void CamAIVision2::onResponse(const char *resp, size_t len) {
  if (resp == nullptr || len == 0) {
    return;
  }

  /// a payload can hold the cut tail of a message followed by the next one,
  /// the newest message is the complete one and the one parsed here
  const char *msg = lastMessageStart(resp, len);
  size_t msg_len = len - (size_t)(msg - resp);

  if (!isEvent(msg, msg_len)) {
    /// the command answers are the way to know if a command was applied
    onReply(msg, msg_len);
    return; // command replies and logs have no frame data
  }

  size_t b64_len = 0;
  const char *data = findImageData(msg, msg_len, &b64_len);
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

  /// an event with image data arrived, even if it ends rejected, the caller
  /// should not wait for the answer timeout of its request
  _gotEvent = true;

  /// the head of the first events is logged to know what the module is really
  /// sending (a base64 JPEG starts with /9j/4)
  if (_events < AIVISION2_DEBUG_FRAMES) {
    _events++;
    const char *from = (msg[0] == '\r') ? msg + 1 : msg;
    size_t head = (size_t)(data - from) + 16;
    log_i("event payload: %.*s", (int)(head > 96 ? 96 : head), from);
  }

  /// the base64 length is always the upper bound of the decoded data
  size_t capacity = ((b64_len / 4) * 3) + 4;
  if (capacity > AIVISION2_MAX_JPEG) {
    _rejected++;
    logFrameIssue("frame too big for the configured limits", data, b64_len, nullptr, 0);
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
    _rejected++;
    logFrameIssue(ret != 0 ? "base64 decode error" : "no JPEG data", data, b64_len, jpeg, jpeg_len);
    ::free(jpeg);
    return;
  }
  if (status == JPEG_INCOMPLETE) {
#if AIVISION2_STRICT_JPEG
    _rejected++;
    logFrameIssue("truncated JPEG, the module sent an incomplete frame", data, b64_len, jpeg, jpeg_len);
    ::free(jpeg);
    return;
#else
    /// a receiver can not paint a frame without the end of image marker, so
    /// the padded tail is removed and the marker is added here (the module
    /// cuts the frame when its link or its buffers can not follow it)
    jpeg_len = trimPadding(jpeg, jpeg_len);
    if (jpeg_len < AIVISION2_MIN_JPEG || !closeJpeg(jpeg, &jpeg_len, capacity)) {
      _rejected++;
      logFrameIssue("truncated JPEG, too short to be useful", data, b64_len, jpeg, jpeg_len);
      ::free(jpeg);
      return;
    }
    _partial++;
    logFrameIssue("partial frame completed with the end of image marker", data, b64_len, jpeg, jpeg_len);
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

/**************************************************
 * M O D U L E   I N I T I A L I Z A T I O N
 **************************************************/

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

/// the AI module is a separate MCU that only resets with the power, so it can
/// be still running the tasks (the stream included) of a previous session: its
/// frames would be mixed with the answers of the new commands and they would
/// fill the UART buffers, so they are stopped here before anything else
void CamAIVision2::stopModuleTasks() {
  char command[16] = {0};
  snprintf(command, sizeof(command), "%s%s%s", CMD_PREFIX, CMD_AT_BREAK, CMD_SUFFIX);
  aivisionSerial.write((const uint8_t *)command, strlen(command));
  delay(AIVISION2_BREAK_DELAY);
  while (aivisionSerial.available()) {
    aivisionSerial.read();
  }
  _streaming = false;
}

/// read the serial port for the given time, it is used to collect the answers
/// of the commands sent during the initialization
void CamAIVision2::pump(uint32_t ms) {
  uint32_t start = millis();
  do {
    _module.fetch([this](const char *resp, size_t len) { onResponse(resp, len); });
    if ((millis() - start) < ms) {
      delay(AIVISION2_POLL_DELAY);
    }
  } while ((millis() - start) < ms);
}

bool CamAIVision2::begin(uint32_t baud) {
  if (_started) {
    log_w("The AI Vision 2 interface is already started..");
    return true;
  }
  log_i("Starting AI Vision 2 interface..");

  aivisionSerial.setRxBufferSize(AIVISION2_UART_BUFFER);
  if (AIVISION2_PIN_RX >= 0 && AIVISION2_PIN_TX >= 0) {
    aivisionSerial.begin(baud, SERIAL_8N1, AIVISION2_PIN_RX, AIVISION2_PIN_TX);
  } else {
    aivisionSerial.begin(baud);
  }

  stopModuleTasks();

  /// the SSCMA initialization asks the module ID and name (the official way to
  /// know if the module is there), but it is not fatal when it fails: the
  /// module does not answer while it is busy
  if (!_module.begin(&aivisionSerial, AIVISION2_PIN_RST, baud)) {
    log_w("The AI Vision 2 module did not answer the identification query..");
  }
  const char *name = _module.name();
  log_i("AI Vision 2 module: %s", (name && name[0]) ? name : "(unknown)");
  _started = true;

  reserveResponseBuffer();

  /// the module keeps its resolution between sessions, so the active one is
  /// asked and logged (the answer includes its name, e.g "480x480 Auto")
  _logNextReply = true;
  writeCmd(CMD_AT_SENSOR "?", strlen(CMD_AT_SENSOR "?"));
  pump(AIVISION2_REPLY_DELAY);

  if (!applyResolution()) {
    log_w("Failed to set the sensor resolution..");
  }
  pump(AIVISION2_REPLY_DELAY);

  if (_streamMode == STREAM_CONTINUOUS) {
    return startStream();
  }
  log_i("Ready, frames are requested on demand (see get() and requestFrame())");
  return true;
}

/**************************************************
 * D R I V E R   A P I
 **************************************************/

bool CamAIVision2::get() {
  if (fb != nullptr) {
    log_w("The current frame was not released, did you forget Camera.free()?");
    free();
  }
  if (!_started) {
    log_e("The AI Vision 2 interface is not initialized, call Camera.begin() first..");
    return false;
  }

  if (_streamMode == STREAM_ON_DEMAND) {
    /// the module is idle until a frame is requested, so the UART can not
    /// overflow while the current frame is sent by the radio (that is what
    /// was cutting the frames)
    _gotEvent = false;
    if (_frames.empty() && !requestFrame()) {
      log_e("Failed to request a frame from the module..");
      _timeouts++;
      return false;
    }
    uint32_t start = millis();
    while (_frames.empty() && !_gotEvent && (millis() - start < AIVISION2_GET_TIMEOUT)) {
      _module.fetch([this](const char *resp, size_t len) { onResponse(resp, len); });
      if (_frames.empty()) {
        delay(AIVISION2_POLL_DELAY);
      }
    }
    if (_frames.empty() && !_gotEvent) {
      _timeouts++;
      if (_timeouts <= AIVISION2_DEBUG_FRAMES || (_timeouts % 100) == 0) {
        log_w("#%u requests without an answer (%u ms), is the module there? (try Camera.sendCmd(\"SENSOR?\"))",
              (unsigned int)_timeouts, (unsigned int)AIVISION2_GET_TIMEOUT);
      }
      return false;
    }
  } else {
    /// the module pushes the frames as fast as it can, here they are pumped
    _module.fetch([this](const char *resp, size_t len) { onResponse(resp, len); });
  }

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

bool CamAIVision2::writeCmd(const char *command, int len) {
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

bool CamAIVision2::sendCmd(const char *command, int len) {
  if (!_started) {
    log_e("The AI Vision 2 interface is not initialized, call Camera.begin() first..");
    return false;
  }
  _logNextReply = true; // the user wants to see the answer of the module
  return writeCmd(command, len);
}

bool CamAIVision2::buildStreamCmd(char *buffer, size_t size, int times, bool differed,
                                  bool resultOnly) {
  if (_frameMode == FRAME_SAMPLE) {
    /// AT+SAMPLE only asks the sensor for an image, there is no inference and
    /// no model to load, so it is the fastest way to get the frames
    return snprintf(buffer, size, "%s=%d", CMD_AT_SAMPLE, times) > 0;
  }
  return snprintf(buffer, size, "%s=%d,%d,%d", CMD_AT_INVOKE, times, differed ? 1 : 0,
                  resultOnly ? 1 : 0) > 0;
}

bool CamAIVision2::requestFrame() {
  if (!_started) {
    return false;
  }
  char command[32] = {0};
  if (_frameMode == FRAME_SAMPLE) {
    buildStreamCmd(command, sizeof(command), 1, false, false);
  } else {
    /// a single inference, the image is always in the event (a differed
    /// result would not send any event at all, so it is disabled here)
    snprintf(command, sizeof(command), "%s=1,0,0", CMD_AT_INVOKE);
  }
  return writeCmd(command, strlen(command));
}

bool CamAIVision2::startStream(int times, bool differed, bool resultOnly) {
  if (!_started) {
    log_e("The AI Vision 2 interface is not initialized, call Camera.begin() first..");
    return false;
  }
  char command[32] = {0};
  buildStreamCmd(command, sizeof(command), times, differed, resultOnly);
  _streamMode = STREAM_CONTINUOUS;
  _streaming = writeCmd(command, strlen(command));
  if (_streaming) {
    log_i("Streaming started: %s (%s)", command, (_frameMode == FRAME_SAMPLE) ? "image only" : "AI results");
  } else {
    log_e("Failed to start the stream..");
  }
  return _streaming;
}

bool CamAIVision2::stopStream() {
  bool ok = false;
  if (_started) {
    ok = writeCmd(CMD_AT_BREAK, strlen(CMD_AT_BREAK));
  }
  _streaming = false;
  _streamMode = STREAM_ON_DEMAND;
  _logNextReply = true;
  dropFrames();
  return ok;
}

void CamAIVision2::setFrameMode(FrameMode mode) {
  if (mode == _frameMode) {
    return;
  }
  _frameMode = mode;
  if (_started && _streaming) {
    startStream(); // the stream command changes with the frame mode
  }
  log_i("Frame mode: %s", (mode == FRAME_SAMPLE) ? "image only (AT+SAMPLE)" : "AI results (AT+INVOKE)");
}

void CamAIVision2::setStreamMode(StreamMode mode) {
  if (!_started) {
    _streamMode = mode;
    return;
  }
  if (mode == STREAM_CONTINUOUS) {
    startStream();
  } else {
    stopStream();
  }
}

bool CamAIVision2::setResolution(uint16_t optId, int sensorId) {
  _sensorOptId = optId;
  _sensorId = sensorId;
  if (!_started) {
    return true; // it is applied on begin()
  }
  /// the module deInitializes and initializes its sensor to change the
  /// resolution, so the running stream (if any) is stopped first, otherwise
  /// the change is mixed with the frames in flight and it is lost
  bool restart = _streaming;
  if (restart) {
    stopStream();
  }
  dropFrames(); // the buffered frames belong to the old resolution
  bool ok = applyResolution();
  pump(AIVISION2_REPLY_DELAY);
  if (restart) {
    ok = startStream() && ok;
  }
  return ok;
}

bool CamAIVision2::applyResolution() {
  if (_sensorOptId < 0) {
    return true;
  }
  char command[24] = {0};
  snprintf(command, sizeof(command), "%s=%d,1,%d", CMD_AT_SENSOR, _sensorId, _sensorOptId);
  log_i("Setting the sensor resolution: %s (see the module reply below)", command);
  _logNextReply = true; // the answer says which resolution is active now
  return writeCmd(command, strlen(command));
}

size_t CamAIVision2::available() { return _frames.size(); }
size_t CamAIVision2::discarded() { return _discarded; }
size_t CamAIVision2::rejected() { return _rejected; }
size_t CamAIVision2::partial() { return _partial; }
size_t CamAIVision2::timeouts() { return _timeouts; }

#endif
