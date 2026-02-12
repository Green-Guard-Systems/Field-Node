#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "camera_setup.h"
#include "sd_card.h"
#include "ml_inference.h"
#include "data_packet.h"

// Global variables
extern int imageCount;

// Function declarations
void save_rgb565_as_bmp(camera_fb_t *fb, const char *path);
void prepare_model_input(camera_fb_t *fb, int8_t* buffer);
void process_image_and_classify();
bool is_system_ready();

// ========== Implementation ==========

int imageCount = 1;

void save_rgb565_as_bmp(camera_fb_t *fb, const char *path) {
    const int width = 96;
    const int height = 96;
    const int row_size = ((width * 3 + 3) / 4) * 4;
    const int pixel_array_size = row_size * height;
    const int file_size = 54 + pixel_array_size;
    
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    
    // BMP Header (14 bytes)
    uint8_t bmp_header[14] = {
        'B', 'M',
        file_size & 0xFF, (file_size >> 8) & 0xFF, 
        (file_size >> 16) & 0xFF, (file_size >> 24) & 0xFF,
        0, 0, 0, 0,
        54, 0, 0, 0
    };
    file.write(bmp_header, 14);
    
    // DIB Header (40 bytes)
    uint8_t dib_header[40] = {
        40, 0, 0, 0,
        width & 0xFF, (width >> 8) & 0xFF, 
        (width >> 16) & 0xFF, (width >> 24) & 0xFF,
        height & 0xFF, (height >> 8) & 0xFF,
        (height >> 16) & 0xFF, (height >> 24) & 0xFF,
        1, 0,
        24, 0,
        0, 0, 0, 0,
        pixel_array_size & 0xFF, (pixel_array_size >> 8) & 0xFF,
        (pixel_array_size >> 16) & 0xFF, (pixel_array_size >> 24) & 0xFF,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    file.write(dib_header, 40);
    
    // Convert RGB565 to RGB888 and write pixel data
    uint8_t row_buffer[row_size];
    for (int y = height - 1; y >= 0; y--) {
        memset(row_buffer, 0, row_size);
        
        for (int x = 0; x < width; x++) {
            int fb_idx = (y * width + x) * 2;
            uint16_t pixel = (fb->buf[fb_idx] << 8) | fb->buf[fb_idx + 1];
            
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            
            row_buffer[x * 3] = b;
            row_buffer[x * 3 + 1] = g;
            row_buffer[x * 3 + 2] = r;
        }
        
        file.write(row_buffer, row_size);
    }
    
    file.close();
    Serial.printf("✅ BMP saved: %s\n", path);
}

void prepare_model_input(camera_fb_t *fb, int8_t* buffer) {
    const int width = 96;
    const int height = 96;
    
    Serial.println("Preparing image for model...");
    
    int idx = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int fb_idx = (y * width + x) * 2;
            uint16_t pixel = (fb->buf[fb_idx] << 8) | fb->buf[fb_idx + 1];
            
            // Extract RGB from RGB565
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            
            // Apply quantization: q = pixel_value - 128
            buffer[idx++] = (int8_t)(r - 128);
            buffer[idx++] = (int8_t)(g - 128);
            buffer[idx++] = (int8_t)(b - 128);
        }
    }
    
    Serial.println("✅ Model input prepared!");
}

void process_image_and_classify() {
    Serial.println("\n==============================");
    Serial.println("📸 Capturing image...");
    
    // Capture image
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ Camera capture failed");
        return;
    }
    
    Serial.printf("✅ Image captured: %dx%d, %d bytes\n", fb->width, fb->height, fb->len);
    
    // Generate filename
    char filename[32];
    sprintf(filename, "/image%d.bmp", imageCount);
    
    // Save to SD card
    save_rgb565_as_bmp(fb, filename);
    
    // Prepare for ML model
    prepare_model_input(fb, input_buffer);
    
    // Release camera buffer
    esp_camera_fb_return(fb);
    
    // Run inference and get classification
    uint8_t health_class = run_inference();
    
    // Build data packet with ML result
    data_packet = build_data_packet(health_class);
    
    // Display packet information
    print_data_packet(data_packet);
    print_packet_binary(data_packet);
    
    // Verify parity
    if (verify_packet_parity(data_packet)) {
        Serial.println("✅ Packet parity check PASSED");
    } else {
        Serial.println("❌ Packet parity check FAILED");
    }
    
    // Increment counter
    imageCount++;
    
    // Now data_packet is ready to be sent to another module
    // Example: send_to_other_module(data_packet);
}

bool is_system_ready() {
    return camera_sign && sd_sign;
}

#endif // IMAGE_PROCESSING_H