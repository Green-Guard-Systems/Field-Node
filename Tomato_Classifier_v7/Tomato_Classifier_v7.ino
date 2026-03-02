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
    
    // If this is the first boot, go straight to sleep
    if (wakeup_reason == 0) {
        Serial.println("First boot - entering deep sleep");
        enter_sleep_mode();
    }
    

    BANANBASNBASNBANSBANSBNASBNSb

    // Initialize if woken up by GPIO trigger
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        espSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
        Serial.println("\n__Starting Green🌱Guard Classifier__");
    
        // Initialize all systems
        init_tinyml();
        init_camera();
        init_sd_card();
        init_sensors();  // Initialize soil moisture and battery sensors
        
        Serial.println("\n🌱System ready!");
    }
}

void loop() {
    if(is_system_ready()){
        process_image_and_classify();
        // The data_packet variable now contains the 32-bit data
        send_data_packet(data_packet);
        delay(1000);
        // Enter deep sleep after sending data packet
        enter_sleep_mode(); 
    } else {
        Serial.println("❌Camera or SD card not ready, sleep mode not entered");
        delay(5000); // Wait before retrying
    }
}
