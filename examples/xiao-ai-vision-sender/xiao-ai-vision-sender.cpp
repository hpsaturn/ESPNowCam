/**************************************************
 * ESPNowCam video Transmitter.
 * ----------------------------
 *
 * XIAO ESP32S3 with the Seeed Studio AI Vision 2 camera module.
 * The AI module does the inference and sends the frames as JPEG, so this
 * sample only needs to stream them like any other ESPNowCam sender.
 *
 * by @hpsaturn Copyright (C) 2024-2026
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <OneButton.h>
#include <ESPNowCam.h>
#include <drivers/CamAIVision2.h>
#include <Utils.h>

CamAIVision2 Camera;  
WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);

OneButton btnB(GPIO_NUM_0, true);

void printDetections() {
  static uint32_t frames = 0;
  if ((++frames % 50) != 0) return; // the console can not follow the stream
  Serial.printf("CAM: %ux%u %u detection(s), infer %ums\r\n", (unsigned int)Camera.fb->width,
                (unsigned int)Camera.fb->height, (unsigned int)Camera.detectionCount(),
                (unsigned int)Camera.perf()[1]);
  for (size_t i = 0; i < Camera.detectionCount(); i++) {
    const boxes_t &box = Camera.detections()[i];
    Serial.printf("  [%u] target %u score %u%% (%u,%u %ux%u)\r\n", (unsigned int)i,
                  (unsigned int)box.target, (unsigned int)box.score, (unsigned int)box.x,
                  (unsigned int)box.y, (unsigned int)box.w, (unsigned int)box.h);
  }
}

void processFrame() {
  if (Camera.get()) {
    // Camera.frameMeta() is the AI results of this frame (nullptr in SAMPLE mode)
    radio.sendDataAndMeta(Camera.fb->buf, Camera.fb->len, Camera.frameMeta(), Camera.frameMetaLen());
    printFPS("CAM:");
    // printDetections(); // optional, the detections are also sent to the receiver
    Camera.free();
  }
}

void shutdown() {
  Serial.println("shutdown..");
  Camera.stopStream(); // stop the AI module tasks before sleeping
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0);
  delay(1000);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  if(psramFound()){
    size_t psram_size = ESP.getPsramSize() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  // Optional M5Core2 receiver B8:F0:09:C6:0E:CC
  const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  radio.setChannel(6);
  radio.init(960);

  // The AI Vision 2 encoded in base64 over its UART (921600 baud ~ 90KB/s) 
  // Keep the 240x240 resolution unless you really need more.
  // 0 -> 240x240 (default), 1 -> 480x480, 2 -> VGA 640x480
  Camera.setResolution(0);

  // The AI results of every frame (the detection boxes, the scores, the
  // inference timings) are only reported by the module in INVOKE mode: the
  // driver parses them and sends them to the receiver with the frame (see
  // FrameMeta.h). In SAMPLE mode only the image is sent.
  Camera.setFrameMode(CamAIVision2::FRAME_INVOKE);

  if (!radio.supportsFrameMetadata()) {
    Serial.println("WARNING: this radio can not carry the AI results, use WiFiRawComm");
  }

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
  }

  // Optional: the module pushes frames as fast as it can, but its buffers
  // overflow when the host can not drain them, so the default (a frame per
  // request) is the recommended one for this radio
  Camera.setStreamMode(CamAIVision2::STREAM_CONTINUOUS);

  // or with any other SSCMA AT command, e.g. the model thresholds:
  // Camera.sendCmd("TSCORE=60");
  // more details: https://github.com/Seeed-Studio/SSCMA-Micro/blob/main/docs/protocol/at-protocol-en_US.md

  btnB.attachClick([]() { shutdown(); });
  delay(100);
}

void loop() {
  processFrame();
  btnB.tick();
}
