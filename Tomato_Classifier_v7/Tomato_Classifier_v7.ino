#include <Arduino.h>
#include "camera_setup.h"
#include "sd_card.h"
#include "data_packet.h"
#include "image_processing.h"
#include "ml_inference.h"

void setup() {
    Serial.begin(115200);
    espSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
    delay(3000);
    Serial.println("\n__Starting Green🌱Guard Classifier__");
    
    // Initialize all systems
    init_tinyml();
    init_camera();
    init_sd_card();
    init_sensors();  // Initialize soil moisture and battery sensors
    
    Serial.println("\n🌱System ready!");
    Serial.println("📋Press any key in Serial Monitor to capture and classify an image...\n");
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
            // The data_packet variable now contains the 32-bit data
            send_data_packet(data_packet); 
            
            Serial.println("📋 Press any key to capture another image\n");
        } else {
            Serial.println("❌ Camera or SD card not ready");
        }
    }
}