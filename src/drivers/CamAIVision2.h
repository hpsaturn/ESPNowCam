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
 * It only is compiled when the AIVISION2 build flag is defined, see the
 * xiao-ai-vision-sender environment on platformio.ini
**************************************************/

#ifndef CAMAIVISION2_H
#define CAMAIVISION2_H

#ifdef AIVISION2

#include <Arduino.h>
#include <Seeed_Arduino_SSCMA.h>
#include <Wire.h>
#include <mbedtls/base64.h>

#include <deque>

#include "CameraBase.hpp"
#include "esp_camera.h"

/**************************************************
 * U A R T   L I N K
 **************************************************/
// serial port and speed used to talk with the AI module
#ifndef AIVISION2_SERIAL_PORT
#define AIVISION2_SERIAL_PORT 0
#endif

// SSCMA UART default speed
#ifndef AIVISION2_BAUD
#define AIVISION2_BAUD 921600
#endif

// use -1 for the default pins of the selected serial port
// (XIAO ESP32S3: D6/GPIO43 TX and D7/GPIO44 RX). Some boards also wire the
// module reset pin, if your wiring has it, define AIVISION2_PIN_RST
#ifndef AIVISION2_PIN_RX
#define AIVISION2_PIN_RX -1
#endif
#ifndef AIVISION2_PIN_TX
#define AIVISION2_PIN_TX -1
#endif
#ifndef AIVISION2_PIN_RST
#define AIVISION2_PIN_RST -1
#endif

/**************************************************
 * M E M O R Y   L I M I T S
 **************************************************/
// UART driver buffer: it only needs to cover the gap between two get() calls
#ifndef AIVISION2_UART_BUFFER
#define AIVISION2_UART_BUFFER (16 * 1024)
#endif

// SSCMA internal buffer: it must be bigger than the biggest event, that means
// a full JSON payload with a base64 JPEG inside (it is dynamically reduced
// on begin() if the internal RAM is not enough)
#ifndef AIVISION2_RESP_BUFFER
#define AIVISION2_RESP_BUFFER (128 * 1024)
#endif

// SSCMA library default (SSCMA_MAX_RX_SIZE), below this value there is no
// reason to resize the internal buffer
#ifndef AIVISION2_MIN_RESP_BUFFER
#define AIVISION2_MIN_RESP_BUFFER (32 * 1024)
#endif

// safety check for the decoded frames
#ifndef AIVISION2_MAX_JPEG
#define AIVISION2_MAX_JPEG (96 * 1024)
#endif

// decoded frames waiting to be consumed by get()
#ifndef AIVISION2_QUEUE_LIMIT
#define AIVISION2_QUEUE_LIMIT 2
#endif

// a frame without the end of image marker means that the module cut the data
// (its internal buffer is smaller than the frame), define it to 1 to drop those
// frames instead of sending them, see the README notes
#ifndef AIVISION2_STRICT_JPEG
#define AIVISION2_STRICT_JPEG 0
#endif

// rejected frames are logged with all the payload details (the first
// AIVISION2_DEBUG_FRAMES ones and then one of each 100), it is useful to know
// what the module is really sending
#ifndef AIVISION2_DEBUG_FRAMES
#define AIVISION2_DEBUG_FRAMES 5
#endif

// SSCMA AT protocol continuous inference, the args are:
// <N_TIMES,DIFFERED,RESULT_ONLY>, where RESULT_ONLY=0 means that the events
// will include the image data (see the Seeed AT protocol documentation)
#ifndef AIVISION2_INVOKE_TIMES
#define AIVISION2_INVOKE_TIMES -1
#endif
#ifndef AIVISION2_INVOKE_DIFFERED
#define AIVISION2_INVOKE_DIFFERED 0
#endif
#ifndef AIVISION2_INVOKE_RESULT_ONLY
#define AIVISION2_INVOKE_RESULT_ONLY 0
#endif

/**************************************************
 * P R O T O C O L   K E Y S
 **************************************************/
#define MSG_IMAGE_KEY "\"image\""
#define MSG_IMAGE_KEY_LEN (sizeof(MSG_IMAGE_KEY) - 1)
#define MSG_QUOTE_STR "\""
#define MSG_TYPE_KEY "\"type\""
#define MSG_TYPE_KEY_LEN (sizeof(MSG_TYPE_KEY) - 1)
#define MSG_EVENT_TYPE "1"

// the SSCMA library does not have a define for the sensor command
#define CMD_AT_SENSOR "SENSOR"

class CamAIVision2 : public CameraBase {
 public:
  CamAIVision2();

  /// @brief initialize the serial link with the AI module and start the
  ///        continuous inference stream (AT+INVOKE)
  /// @param baud UART speed, 921600 by default
  bool begin(uint32_t baud = AIVISION2_BAUD);

  /// @brief read pending data from the module and attach the next available
  ///        JPEG frame to fb. It returns false if there is no new frame yet.
  bool get();

  /// @brief release the current frame (fb) and its memory
  bool free();

  /// @brief send a raw AT command to the module, the AT+ prefix and the
  ///        CRLF suffix are added here, e.g: sendCmd("INFO?")
  /// @param command command body without the AT+ prefix
  /// @param len command body length
  bool sendCmd(const char *command, int len);

  /// @brief convenience wrapper for null terminated commands
  /// @param command command body without the AT+ prefix
  bool sendCmd(const char *command) { return sendCmd(command, strlen(command)); }

  /// @brief start the continuous inference stream (AT+INVOKE)
  /// @param times number of inferences, -1 for an infinite loop
  /// @param differed only send events when the result changes
  /// @param resultOnly do not include the image data in the events
  bool startStream(int times = AIVISION2_INVOKE_TIMES,
                   bool differed = AIVISION2_INVOKE_DIFFERED,
                   bool resultOnly = AIVISION2_INVOKE_RESULT_ONLY);

  /// @brief stop the running tasks on the module (AT+BREAK)
  bool stopStream();

  /// @brief set the resolution of the sensor attached to the AI module
  ///        (AT+SENSOR), 0 -> 240x240, 1 -> 480x480, 2 -> VGA 640x480
  /// @note it can be called before begin(), then it is applied on the
  ///       initialization, or later, then it is applied on the fly
  /// @param optId sensor resolution option
  /// @param sensorId sensor id, 1 by default
  bool setResolution(uint16_t optId, int sensorId = 1);

  /// @brief frames decoded and waiting to be read with get()
  size_t available();

  /// @brief frames discarded because the queue was full (the receiver is
  ///        slower than the camera). It could be useful to tune the stream.
  size_t discarded();

  /// @brief frames with an invalid or incomplete payload (a corrupted message,
  ///        an image cut by the module, an unknown encoding, etc). The details
  ///        of the first ones are logged, see AIVISION2_DEBUG_FRAMES
  size_t rejected();

  /// @brief direct access to the Seeed SSCMA driver, useful to get the module
  ///        ID, name, version, model info, thresholds, etc.
  SSCMA &module() { return _module; }

 private:
  SSCMA _module;
  std::deque<camera_fb_t *> _frames;
  size_t _discarded = 0;
  size_t _rejected = 0;
  size_t _events = 0;
  bool _streaming = false;
  int _sensorOptId = -1;
  int _sensorId = 1;

  void reserveResponseBuffer();
  bool applyResolution();
  void onResponse(const char *resp, size_t len);
  void reportRejected(const char *reason, const char *b64, size_t b64_len,
                      const uint8_t *jpeg, size_t jpeg_len);
  void pushFrame(camera_fb_t *frame);
  void dropFrames();
};

#endif
#endif
