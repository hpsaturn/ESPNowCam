/**************************************************
 * ESPNowCam detections overlay (M5Unified samples)
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
 *
 * The senders of this library can attach the AI results of every frame to the
 * frame itself (see FrameMeta.h), here they are painted over the received
 * image: the boxes of the detections, their class (target) and their score.
 *
 * The boxes are in the resolution of the sender frame, so they are scaled to
 * the rectangle used to paint the image: use the same rectangle that you gave
 * to M5.Display.drawJpg().
 **************************************************/

#ifndef DETECTIONS_OVERLAY_H
#define DETECTIONS_OVERLAY_H

#include <M5Unified.h>

#include <stdio.h>

#include "FrameMeta.h"

/// one color per detection class (out of range classes wrap around)
static const uint16_t overlayColors[] = {TFT_RED,     TFT_GREEN, TFT_CYAN, TFT_YELLOW,
                                         TFT_MAGENTA, TFT_ORANGE, TFT_BLUE, TFT_LIGHTGREY};
#define OVERLAY_COLORS (sizeof(overlayColors) / sizeof(overlayColors[0]))

/// @brief paint the detections of the frame metadata over the painted image
/// @param canvas display or canvas that painted the frame
/// @param meta metadata of the frame, see the senders (Camera.frameMeta())
/// @param lenght length of the metadata
/// @param x,y,w,h rectangle where the frame was painted with drawJpg()
/// @return number of painted boxes (0 when the frame has no detection)
static inline size_t drawDetectionsOverlay(M5GFX &canvas, const uint8_t *meta, uint32_t lenght,
                                           int32_t x, int32_t y, int32_t w, int32_t h) {
  FrameMetaReader reader(meta, lenght);
  if (!reader.valid() || reader.type() != FRAME_META_TYPE_DETECTIONS) {
    return 0;
  }
  if (reader.width() == 0 || reader.height() == 0) {
    return 0;
  }

  /// the metadata tells the resolution of the frame that the boxes refer to
  float scaleX = (float)w / (float)reader.width();
  float scaleY = (float)h / (float)reader.height();

  FrameMetaBox box;
  size_t painted = 0;
  for (size_t i = 0; i < reader.count(); i++) {
    if (!reader.box(i, box)) {
      break;
    }
    int32_t bx = x + (int32_t)(box.x * scaleX);
    int32_t by = y + (int32_t)(box.y * scaleY);
    int32_t bw = (int32_t)(box.w * scaleX);
    int32_t bh = (int32_t)(box.h * scaleY);
    if (bw < 2 || bh < 2) {
      continue; // too small to be seen
    }
    uint16_t color = overlayColors[box.target % OVERLAY_COLORS];
    canvas.drawRect(bx - bw/2, by - bh/2, bw, bh, color);
    /// the label is painted over the image, the small font with its own
    /// background is the cheapest way to be readable (no extra fills, the
    /// frame rate of the stream would pay for them)
    char label[16];
    snprintf(label, sizeof(label), "%u %u%%", (unsigned int)box.target, (unsigned int)box.score);
    canvas.setTextSize(1);
    canvas.setTextColor(color, TFT_BLACK);
    canvas.drawString(label, bx + 2, (by >= 12) ? (by - 12) : (by + 1));
    painted++;
  }
  return painted;
}

#endif  // DETECTIONS_OVERLAY_H
