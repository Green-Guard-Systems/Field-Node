#ifndef SLEEP_MODE_H
#define SLEEP_MODE_H

#include "driver/rtc_io.h"
#include "ml_inference.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_bt.h"
#include <Wire.h>

#define SLEEP_DURATION_SECONDS 10

void enter_sleep_mode() {

    // === CAMERA SHUTDOWN ===
    camera_power_down();
    esp_camera_deinit();
    
    // === OTHER PERIPHERALS ===
    SD.end();
    deinit_tinyml();

    // === SM SENSOR GPIO ISOLATION ===
    rtc_gpio_isolate(GPIO_NUM_5);
    rtc_gpio_isolate(GPIO_NUM_6);

    // === TIMER WAKEUP ===
    uint64_t sleep_duration_us = SLEEP_DURATION_SECONDS * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleep_duration_us);

    Serial.printf("Entering deep sleep for %d seconds...\n", SLEEP_DURATION_SECONDS);
    Serial.flush();

    esp_deep_sleep_start();
}

#endif // SLEEP_MODE_H