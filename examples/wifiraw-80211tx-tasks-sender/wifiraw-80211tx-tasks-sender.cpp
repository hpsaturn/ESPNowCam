
#include <M5CoreS3.h>
#include "esp_camera.h"
#include "ESPNowCam.h"
#include "Utils.h"

// ---------------------------------------------------------------------
// Global objects
// ---------------------------------------------------------------------
WiFiRawComm wifiRaw;
ESPNowCam radio(&wifiRaw);

int32_t dw, dh;                     // display dimensions
static int frameCount = 0;           // for FPS calculation

// ---------------------------------------------------------------------
// Queue for passing JPEG buffers from compression task to sender task
// ---------------------------------------------------------------------
typedef struct {
    uint8_t* buf;    // JPEG buffer (allocated by frame2jpg)
    size_t   len;    // size of JPEG data
} jpeg_frame_t;

QueueHandle_t jpegQueue;              // handles up to 3 frames

// ---------------------------------------------------------------------
// Compression task (runs on core 1, priority 1)
// ---------------------------------------------------------------------
void compressionTask(void *pvParameters) {
    while (1) {
        // Capture a frame from the camera
        if (CoreS3.Camera.get()) {
            CoreS3.Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
            uint8_t *out_jpg = NULL;
            size_t out_jpg_len = 0;

            // Compress to JPEG (uses malloc, typically from PSRAM)
            frame2jpg(CoreS3.Camera.fb, 24, &out_jpg, &out_jpg_len);

            if (out_jpg) {
                jpeg_frame_t frame = { out_jpg, out_jpg_len };

                // Try to send to queue; if queue is full, discard the frame
                if (xQueueSend(jpegQueue, &frame, 0) != pdPASS) {
                    free(out_jpg);           // queue full – drop frame
                }
            }

            // Always free the camera frame buffer
            CoreS3.Camera.free();
        }

        // Small delay to prevent task from hogging the core if no frame is ready
        vTaskDelay(1);
    }
}

// ---------------------------------------------------------------------
// Sender task (runs on core 0, priority 2 – higher than compression)
// ---------------------------------------------------------------------
void senderTask(void *pvParameters) {
    jpeg_frame_t frame;
    while (1) {
        // Wait indefinitely for a JPEG frame from the queue
        if (xQueueReceive(jpegQueue, &frame, portMAX_DELAY) == pdPASS) {
            radio.sendData(frame.buf, frame.len);
            free(frame.buf);                  // free JPEG buffer after sending
        }
    }
}

// ---------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Initialize M5CoreS3
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    CoreS3.Display.setTextColor(GREEN);
    CoreS3.Display.setTextDatum(middle_center);
    CoreS3.Display.setFont(&fonts::Orbitron_Light_24);
    CoreS3.Display.setTextSize(1);

    dw = CoreS3.Display.width();
    dh = CoreS3.Display.height();

    // Check PSRAM
    if (psramFound()) {
        size_t psram_size = esp_spiram_get_size() / 1048576;
        Serial.printf("PSRAM size: %d MB\r\n", psram_size);
    }

    // Initialize camera
    if (!CoreS3.Camera.begin()) {
        CoreS3.Display.drawString("Camera Init Fail", dw / 2, dh / 2);
        while (1) delay(100);   // halt
    }
    CoreS3.Display.drawString("Camera Init Success", dw / 2, dh / 2);
    delay(500);

    // Set camera resolution (QVGA as in original code)
    CoreS3.Camera.sensor->set_framesize(CoreS3.Camera.sensor, FRAMESIZE_QVGA);

    // Initialize ESP‑NOW radio
    const uint8_t macRecv[6] = {0xB8, 0xF0, 0x09, 0xC6, 0x0E, 0xCC};
    radio.setTarget(macRecv);
    radio.setChannel(6);

    if (radio.init(960)) {   // chunk size in bytes
        Serial.println("ESPNowCam Init Success");
    } else {
        Serial.println("ESPNowCam Init Failed");
    }

    // Create queue (depth = 3 frames)
    jpegQueue = xQueueCreate(3, sizeof(jpeg_frame_t));
    if (jpegQueue == NULL) {
        Serial.println("Failed to create queue");
        while (1) delay(100);
    }

    // Create compression task on core 1 (Arduino usually runs on core 1)
    xTaskCreatePinnedToCore(
        compressionTask,
        "Compress",
        4096,          // stack size (adjust if needed)
        NULL,
        1,             // priority (lower than sender)
        NULL,
        1              // core 1
    );

    // Create sender task on core 0 with higher priority
    xTaskCreatePinnedToCore(
        senderTask,
        "Sender",
        4096,
        NULL,
        2,             // higher priority
        NULL,
        0              // core 0
    );
    // Small delay to let tasks start
    delay(100);
}

void loop() {
  vTaskDelete(NULL);
}