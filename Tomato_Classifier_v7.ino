#include <Arduino.h>
#include "camera_setup.h"
#include "sd_card.h"
#include "image_processing.h"
#include "ml_inference.h"

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("\n__Starting 🍅 Camera Classifier__");
    
    // Initialize all systems
    init_tinyml();
    init_camera();
    init_sd_card();
    
    Serial.println("\n✨ System ready!");
    Serial.println("📋 Press any key in Serial Monitor to capture and classify an image\n");
}

void loop() {
    // Check if data is available in the serial buffer
    if (Serial.available() > 0) {
        // Read and discard the character
        Serial.read();
        
        // Clear any remaining characters in the buffer
        while(Serial.available() > 0) {
            Serial.read();
        }
        
        if(is_system_ready()){
            process_image_and_classify();
            Serial.println("📋 Press any key to capture another image\n");
        } else {
            Serial.println("❌ Camera or SD card not ready");
        }
    }
}