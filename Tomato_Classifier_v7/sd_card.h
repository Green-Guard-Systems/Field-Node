#ifndef SD_CARD_H
#define SD_CARD_H

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Global variables
extern bool sd_sign;

// Function declarations
void init_sd_card();

// ========== Implementation ==========
bool sd_sign = false;

void init_sd_card() {
    Serial.println("\n[SD Card Setup]");
    Serial.println("SD Card initializing...");
    
    if(!SD.begin(21)){
        Serial.println("❌ SD Card mount failed");
        return;
    }
    
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        Serial.println("❌ No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC) Serial.println("MMC");
    else if(cardType == CARD_SD) Serial.println("SDSC");
    else if(cardType == CARD_SDHC) Serial.println("SDHC");
    else Serial.println("UNKNOWN");

    sd_sign = true;
    Serial.println("✅ SD Card initialized");
}

#endif // SD_CARD_H