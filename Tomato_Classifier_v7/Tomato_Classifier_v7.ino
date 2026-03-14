#include <Arduino.h>
#include "camera_setup.h"
#include "sd_card.h"
#include "data_packet.h"
#include "image_processing.h"
#include "ml_inference.h"
#include "sleep_mode.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    // Print wakeup reason for debugging
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Woke up from timer");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Woke up from GPIO");
            break;
        default:
            Serial.println("First boot or other wakeup reason");
            break;
    }

    // Initialize on every wakeup
    espSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
    Serial.println("\n__Starting Green🌱Guard Classifier__");

    // Initialize all systems
    init_tinyml();
    init_camera();
    init_sd_card();
    init_sensors();  // Initialize soil moisture and battery sensors

    // Make sure camera isn't in standy mode
    camera_power_up();
    
    Serial.println("\n🌱System ready!");
}

void loop() {
    if(is_system_ready()){
        // Process image and classify
        process_image_and_classify();
        
        // Send the data packet
        send_data_packet(data_packet);
        
        Serial.println("✅ Task complete, entering sleep...");
        
        // Enter deep sleep for 10 seconds
        enter_sleep_mode();
    } else {
        Serial.println("❌Camera or SD card not ready, retrying in 30s");
        // Still enter sleep mode even if not ready
        enter_sleep_mode(); 
    }
}