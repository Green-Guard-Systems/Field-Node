#ifndef SLEEP_MODE_H
#define SLEEP_MODE_H

#include "driver/rtc_io.h"

#define WAKEUP_GPIO GPIO_NUM_1

void enter_sleep_mode() {
  // Deinitialize peripherals to save power
  SD.end();
  esp_camera_deinit();

  // Configure GPIO1 for ext0 wakeup (wake on HIGH)
  esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);

  // Configure pull-down so GPIO1 defaults to LOW
  rtc_gpio_pullup_dis(WAKEUP_GPIO);
  rtc_gpio_pulldown_en(WAKEUP_GPIO);

  Serial.println("Entering deep sleep. Trigger GPIO1 HIGH to wake...");
  delay(100); // Give serial time to finish

  esp_deep_sleep_start();
}

#endif // SLEEP_MODE_H