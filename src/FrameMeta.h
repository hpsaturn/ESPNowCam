/**************************************************
 * ESPNowCam frame metadata
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
 *
 * A frame (a JPEG, a PCM buffer, any payload) can carry a small metadata blob
 * with it: what an AI module found inside the image, the resolution of the
 * source frame, the state of the frame, etc. ESPNowCam::sendData() sends it
 * with the last chunk of the frame, so the metadata and its frame arrive
 * atomically: no extra radio message, no ordering problem and no change of
 * the frame rate (the blob is a few tens of bytes against the KBs of a frame).
 *
 * The layout is little endian and versioned: a receiver must ignore the
 * metadata that it does not know instead of failing, so new types can be
 * added without breaking the existing ones (nanopb skips the unknown fields,
 * so an old receiver simply ignores the metadata of a new sender).
 *
 *   offset  size  field
 *   0       2     magic (0xA1 0x7C, "A|" as AI)
 *   2       1     version (1 at this moment)
 *   3       1     type (see FRAME_META_TYPE_*)
 *   4       2     source frame width, the frame the entries refer to
 *   6       2     source frame height
 *   8       1     number of entries (boxes)
 *   9       1     flags (see FRAME_META_FLAG_*)
 *   10      1     perf: preprocess time (ms)
 *   11      1     perf: inference time (ms)
 *   12      1     perf: postprocess time (ms)
 *   13      3     reserved (zero)
 *   16      10*n  entries of FRAME_META_TYPE_DETECTIONS:
 *                 x, y, w, h (uint16 little endian), score (0-100), target
 *
 * FRAME_META_MAX_SIZE must match the max_size of the meta field of
 * src/frame.proto (regenerate the protobuf files if it changes).
 **************************************************/

#ifndef FRAME_META_H
#define FRAME_META_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// the metadata is only accepted when it starts with these two bytes
#define FRAME_META_MAGIC0 0xA1
#define FRAME_META_MAGIC1 0x7C

/// layout version, the receivers only accept the version they know
#define FRAME_META_VERSION 1

/// kinds of metadata (a frame can only carry one of them)
#define FRAME_META_TYPE_DETECTIONS 1

/// maximum number of entries of a metadata blob
#define FRAME_META_MAX_BOXES 16

/// bytes of one detection: x,y,w,h (uint16) + score + target
#define FRAME_META_BOX_SIZE 10

/// fixed part of the metadata
#define FRAME_META_HEADER_SIZE 16

/// maximum size of a metadata blob (it must fit in src/frame.proto)
#define FRAME_META_MAX_SIZE \
  (FRAME_META_HEADER_SIZE + (FRAME_META_MAX_BOXES * FRAME_META_BOX_SIZE))

/// flags of the frame that carries the metadata
#define FRAME_META_FLAG_PARTIAL 0x01  ///< the frame was cut and completed here

/// one detection as it is sent on the air (the layout of the boxes)
struct FrameMetaBox {
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  uint8_t score;
  uint8_t target;
};

/**************************************************
 * W R I T E R  ( s e n d e r   s i d e )
 **************************************************/
class FrameMetaWriter {
 public:
  FrameMetaWriter(uint8_t *buffer, size_t size) : _buffer(buffer), _size(size), _used(0) {}

  /// start a new blob of the given type, it writes the header and the source
  /// frame size, the entries are added with addBox()
  /// @return false when there is no room for even an empty blob
  bool begin(uint8_t type, uint16_t width, uint16_t height) {
    if (_buffer == nullptr || _size < FRAME_META_HEADER_SIZE) {
      return false;
    }
    memset(_buffer, 0, FRAME_META_HEADER_SIZE);
    _buffer[0] = FRAME_META_MAGIC0;
    _buffer[1] = FRAME_META_MAGIC1;
    _buffer[2] = FRAME_META_VERSION;
    _buffer[3] = type;
    put16(_buffer + 4, width);
    put16(_buffer + 6, height);
    _used = FRAME_META_HEADER_SIZE;
    return true;
  }

  /// @brief mark the state of the frame that carries this metadata
  void setFlags(uint8_t flags) {
    if (_used >= FRAME_META_HEADER_SIZE) {
      _buffer[9] = flags;
    }
  }

  /// @brief timings of the AI inference of this frame in milliseconds
  void setPerf(uint8_t preprocess, uint8_t inference, uint8_t postprocess) {
    if (_used >= FRAME_META_HEADER_SIZE) {
      _buffer[10] = preprocess;
      _buffer[11] = inference;
      _buffer[12] = postprocess;
    }
  }

  /// @brief append one detection of this frame
  /// @return false when there is no room left (the blob is still valid)
  bool addBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t score, uint8_t target) {
    if (_buffer == nullptr || (_used + FRAME_META_BOX_SIZE) > _size) {
      return false;
    }
    uint8_t *box = _buffer + _used;
    put16(box, x);
    put16(box + 2, y);
    put16(box + 4, w);
    put16(box + 6, h);
    box[8] = score;
    box[9] = target;
    _used += FRAME_META_BOX_SIZE;
    _buffer[8] = (uint8_t)((_used - FRAME_META_HEADER_SIZE) / FRAME_META_BOX_SIZE);
    return true;
  }

  /// @brief size of the blob built so far, 0 when nothing was written
  size_t length() { return _used; }

 private:
  uint8_t *_buffer;
  size_t _size;
  size_t _used;

  static void put16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xFF);
    out[1] = (uint8_t)(value >> 8);
  }
};

/**************************************************
 * R E A D E R  ( r e c e i v e r   s i d e )
 **************************************************/
class FrameMetaReader {
 public:
  FrameMetaReader(const uint8_t *meta, size_t length) : _meta(meta), _length(length) {}

  /// @brief true when the blob is a metadata of the known version
  bool valid() {
    return _meta != nullptr && _length >= FRAME_META_HEADER_SIZE &&
           _meta[0] == FRAME_META_MAGIC0 && _meta[1] == FRAME_META_MAGIC1 &&
           _meta[2] == FRAME_META_VERSION;
  }

  uint8_t type() { return valid() ? _meta[3] : 0; }

  /// @brief resolution of the frame the entries refer to (useful to scale)
  uint16_t width() { return valid() ? get16(_meta + 4) : 0; }
  uint16_t height() { return valid() ? get16(_meta + 6) : 0; }

  /// @brief number of entries, it is clamped to the received data
  size_t count() {
    if (!valid()) {
      return 0;
    }
    size_t available = (_length - FRAME_META_HEADER_SIZE) / FRAME_META_BOX_SIZE;
    size_t count = _meta[8];
    return (count < available) ? count : available;
  }

  uint8_t flags() { return valid() ? _meta[9] : 0; }
  uint8_t perfPreprocess() { return valid() ? _meta[10] : 0; }
  uint8_t perfInference() { return valid() ? _meta[11] : 0; }
  uint8_t perfPostprocess() { return valid() ? _meta[12] : 0; }

  /// @brief read one entry of FRAME_META_TYPE_DETECTIONS
  /// @return false when the index is out of the received data
  bool box(size_t index, FrameMetaBox &out) {
    if (index >= count()) {
      return false;
    }
    const uint8_t *box = _meta + FRAME_META_HEADER_SIZE + (index * FRAME_META_BOX_SIZE);
    out.x = get16(box);
    out.y = get16(box + 2);
    out.w = get16(box + 4);
    out.h = get16(box + 6);
    out.score = box[8];
    out.target = box[9];
    return true;
  }

 private:
  const uint8_t *_meta;
  size_t _length;

  static uint16_t get16(const uint8_t *in) { return (uint16_t)(in[0] | (in[1] << 8)); }
};

#endif  // FRAME_META_H
