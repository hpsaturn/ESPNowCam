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
 * NOTES
 * - The module is a separate MCU: it only resets with the power, so its tasks
 *   survive a host reboot. The driver stops them on begin() and then requests
 *   one frame at a time (STREAM_ON_DEMAND), the module is idle in between and
 *   nothing is lost while the frame is sent over the radio.
 * - The AT+SAMPLE command (default) only asks for the image, AT+INVOKE asks
 *   for the AI results too, see FrameMode.
 * - The UART is the bottleneck of this camera: 921600 baud is ~90KB/s, so a
 *   240x240 JPEG (~13KB -> ~17KB in base64) needs ~200ms, and 480x480/VGA are
 *   much bigger (they cap the FPS around 1). See the README notes.
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
// UART driver buffer: it only needs to cover the gap between two reads
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

// a frame smaller than this is not worth sending (a cut frame with a few
// bytes of the JPEG header inside)
#ifndef AIVISION2_MIN_JPEG
#define AIVISION2_MIN_JPEG 256
#endif

// decoded frames waiting to be consumed by get()
#ifndef AIVISION2_QUEUE_LIMIT
#define AIVISION2_QUEUE_LIMIT 2
#endif

/**************************************************
 * T I M I N G S
 **************************************************/
// the module keeps its tasks running while the host reboots (it is only reset
// with the power), so the old ones are broken on begin() and this is the time
// given to the module to stop them and to empty its buffers
#ifndef AIVISION2_BREAK_DELAY
#define AIVISION2_BREAK_DELAY 200
#endif

// time given to the module to answer a command (SENSOR?, SENSOR=, ...)
#ifndef AIVISION2_REPLY_DELAY
#define AIVISION2_REPLY_DELAY 250
#endif

// STREAM_ON_DEMAND: how long get() waits for the frame of the current request.
// It is only spent when the module does not answer (a VGA frame needs ~1s)
#ifndef AIVISION2_GET_TIMEOUT
#define AIVISION2_GET_TIMEOUT 2000
#endif

// idle time between two serial reads while waiting for a frame
#ifndef AIVISION2_POLL_DELAY
#define AIVISION2_POLL_DELAY 2
#endif

/**************************************************
 * F R A M E S
 **************************************************/
// a frame without the end of image marker means that the module (or the link)
// cut the data: by default it is completed with the missing marker, because a
// receiver can paint a partial frame but not a frame without its end. Define
// it to 1 to drop those frames instead, see the README notes
#ifndef AIVISION2_STRICT_JPEG
#define AIVISION2_STRICT_JPEG 0
#endif

// the payload of the first frames and the details of the first bad ones are
// logged, after that only one of each 100 (the console can not follow the
// stream speed)
#ifndef AIVISION2_DEBUG_FRAMES
#define AIVISION2_DEBUG_FRAMES 5
#endif

// chars of the module replies that are logged (VER?, SENSOR?, ...)
#ifndef AIVISION2_REPLY_LOG
#define AIVISION2_REPLY_LOG 240
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
  /// @brief what is asked to the module for every frame
  enum FrameMode : uint8_t {
    FRAME_SAMPLE = 0,  ///< AT+SAMPLE: the image only, no inference (default)
    FRAME_INVOKE = 1,  ///< AT+INVOKE: the AI results and the image
  };

  /// @brief how the frames are delivered by the module
  enum StreamMode : uint8_t {
    STREAM_ON_DEMAND = 0,   ///< one frame per request (default), the module is
                            ///< idle while the frame is sent by the radio
    STREAM_CONTINUOUS = 1,  ///< the module pushes frames as fast as it can
  };

  CamAIVision2();

  /// @brief initialize the serial link with the AI module, stop the tasks that
  ///        a previous session left running and (optionally) start the stream
  /// @param baud UART speed, 921600 by default
  bool begin(uint32_t baud = AIVISION2_BAUD);

  /// @brief read pending data from the module and attach the next available
  ///        JPEG frame to fb, see free(). It returns false if the module did
  ///        not send a frame (in STREAM_ON_DEMAND a new frame is requested).
  bool get();

  /// @brief release the current frame (fb) and its memory
  bool free();

  /// @brief send a raw AT command to the module, the AT+ prefix and the CRLF
  ///        suffix are added here, e.g: sendCmd("INFO?"). The reply is logged.
  /// @param command command body without the AT+ prefix
  /// @param len command body length
  bool sendCmd(const char *command, int len);

  /// @brief convenience wrapper for null terminated commands
  /// @param command command body without the AT+ prefix
  bool sendCmd(const char *command) { return sendCmd(command, strlen(command)); }

  /// @brief ask the module for a single frame (STREAM_ON_DEMAND, get() calls
  ///        it automatically), it uses the current FrameMode
  bool requestFrame();

  /// @brief start the continuous stream, the module pushes the frames as fast
  ///        as it can, e.g: startStream(-1) for an endless stream
  /// @param times number of frames, -1 for an endless stream
  /// @param differed only send events when the result changes (INVOKE mode)
  /// @param resultOnly do not include the image data in the events
  bool startStream(int times = AIVISION2_INVOKE_TIMES,
                   bool differed = AIVISION2_INVOKE_DIFFERED,
                   bool resultOnly = AIVISION2_INVOKE_RESULT_ONLY);

  /// @brief stop the running tasks on the module (AT+BREAK) and go back to the
  ///        on demand mode
  bool stopStream();

  /// @brief set how the frames are requested (SAMPLE or INVOKE)
  void setFrameMode(FrameMode mode);
  FrameMode frameMode() { return _frameMode; }

  /// @brief set how the frames are delivered (on demand or continuous), it
  ///        starts/stops the stream if the module is already initialized
  void setStreamMode(StreamMode mode);
  StreamMode streamMode() { return _streamMode; }

  /// @brief true when the module is pushing frames (continuous mode)
  bool streaming() { return _streaming; }

  /// @brief set the resolution of the sensor attached to the AI module
  ///        (AT+SENSOR), 0 -> 240x240, 1 -> 480x480, 2 -> VGA 640x480
  /// @note it can be called before begin(), then it is applied on the
  ///       initialization, or later, then the stream (if any) is restarted
  /// @param optId sensor resolution option
  /// @param sensorId sensor id, 1 by default
  bool setResolution(uint16_t optId, int sensorId = 1);

  /// @brief frames decoded and waiting to be read with get()
  size_t available();

  /// @brief frames discarded because the queue was full (the receiver is
  ///        slower than the camera). It could be useful to tune the stream.
  size_t discarded();

  /// @brief frames with an invalid payload (a corrupted message, an image cut
  ///        by the module and too short to be useful, an unknown encoding...)
  size_t rejected();

  /// @brief frames delivered without the end of image marker (the module cut
  ///        them) and completed here, see AIVISION2_STRICT_JPEG
  size_t partial();

  /// @brief requests without an answer (the module did not send the frame)
  size_t timeouts();

  /// @brief direct access to the Seeed SSCMA driver, useful to get the module
  ///        ID, name, version, model info, thresholds, etc.
  SSCMA &module() { return _module; }

 private:
  SSCMA _module;
  std::deque<camera_fb_t *> _frames;
  size_t _discarded = 0;
  size_t _rejected = 0;
  size_t _partial = 0;
  size_t _timeouts = 0;
  size_t _events = 0;
  bool _started = false;
  bool _streaming = false;
  bool _logNextReply = false;
  bool _gotEvent = false;
  FrameMode _frameMode = FRAME_SAMPLE;
  StreamMode _streamMode = STREAM_ON_DEMAND;
  int _sensorOptId = -1;
  int _sensorId = 1;

  void reserveResponseBuffer();
  void stopModuleTasks();
  void pump(uint32_t ms);
  bool writeCmd(const char *command, int len);
  bool applyResolution();
  bool buildStreamCmd(char *buffer, size_t size, int times, bool differed, bool resultOnly);
  void onResponse(const char *resp, size_t len);
  void onReply(const char *resp, size_t len);
  void logFrameIssue(const char *reason, const char *b64, size_t b64_len,
                     const uint8_t *jpeg, size_t jpeg_len);
  void pushFrame(camera_fb_t *frame);
  void dropFrames();
};

#endif
#endif
