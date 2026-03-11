#ifndef SLEEP_MODE_H
#define SLEEP_MODE_H

#include "driver/rtc_io.h"

#define WAKEUP_GPIO GPIO_NUM_1
#define SLEEP_DURATION_SECONDS 30

void enter_sleep_mode() {
    // Deinitialize peripherals to save power
    SD.end();
    esp_camera_deinit();

    // Configure timer wakeup for 30 seconds
    uint64_t sleep_duration_us = SLEEP_DURATION_SECONDS * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleep_duration_us);

    // Optional: Keep GPIO wakeup as a manual override
    // Comment out these lines if you don't want manual wakeup
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);
    rtc_gpio_pullup_dis(WAKEUP_GPIO);
    rtc_gpio_pulldown_en(WAKEUP_GPIO);

    Serial.printf("Entering deep sleep for %d seconds...\n", SLEEP_DURATION_SECONDS);
    delay(100); // Give serial time to finish

    esp_deep_sleep_start();
}

#endif // SLEEP_MODE_H
