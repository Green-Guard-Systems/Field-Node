#ifndef DATA_PACKET_H
#define DATA_PACKET_H

#include <Arduino.h>
#include "Adafruit_seesaw.h"

// Sensor objects
Adafruit_seesaw ss;

// Pin configuration for sensors
#define BATTERY_PIN 3        // ADC pin for battery voltage monitoring
#define SEESAW_ADDR 0x36     // I2C address for Adafruit soil sensor

// Node configuration (CHANGE THESE FOR EACH NODE)
#define NODE_TYPE 0b001      // 3 bits: Node type (0-7)
#define NODE_ID 0b00000001   // 8 bits: Node ID (0-255)

// Soil moisture calibration values (adjust based on your sensor)
// These are typical ranges for the Adafruit capacitive sensor
#define SOIL_MIN 200    // Sensor reading in water (very wet)
#define SOIL_MAX 2000   // Sensor reading in air (very dry)

// Data packet bit positions
#define NODE_TYPE_BITS    3   // Bits 0-2
#define NODE_ID_BITS      8   // Bits 3-10
#define HEALTH_CLASS_BITS 2   // Bits 11-12
#define BATTERY_BITS      7   // Bits 13-19
#define SOIL_MOIST_BITS   11  // Bits 20-30
#define PARITY_BIT        1   // Bit 31

// Health classification mapping
enum HealthClass {
    BACTERIAL = 0b00,  // 0
    HEALTHY   = 0b01,  // 1
    INVALID   = 0b10   // 2
};

// Global variables
extern uint32_t data_packet;
extern bool soil_sensor_available;

// Function declarations
void init_sensors();
uint8_t read_battery_percentage();
uint16_t read_soil_moisture();
uint8_t calculate_parity(uint32_t data);
uint32_t build_data_packet(uint8_t health_class);
void print_data_packet(uint32_t packet);
void print_packet_binary(uint32_t packet);
bool verify_packet_parity(uint32_t packet);

// ========== Implementation ==========

uint32_t data_packet = 0;
bool soil_sensor_available = false;

void init_sensors() {
    Serial.println("\n[Sensor Setup]");
    
    // Initialize Adafruit soil sensor
    if (!ss.begin(SEESAW_ADDR)) {
        Serial.println("⚠️  WARNING! Seesaw soil sensor not found");
        Serial.println("   Soil moisture readings will default to 0");
        soil_sensor_available = false;
    } else {
        Serial.print("✅ Seesaw soil sensor initialized! Version: 0x");
        Serial.println(ss.getVersion(), HEX);
        soil_sensor_available = true;
    }
    
    // Configure battery monitoring ADC pin
    pinMode(BATTERY_PIN, INPUT);
    
    // ESP32-S3 ADC configuration
    analogReadResolution(12); // 12-bit resolution (0-4095)
    analogSetAttenuation(ADC_11db); // Full range ~0-3.3V
    
    Serial.println("✅ Sensors initialized");
}

uint8_t read_battery_percentage() {
    // Read battery voltage
    // Assuming battery monitoring circuit outputs 0-3.3V proportional to 0-100%
    // Adjust this based on your actual battery monitoring circuit
    
    int raw_value = analogRead(BATTERY_PIN);
    
    // Convert 12-bit ADC (0-4095) to percentage (0-100)
    uint8_t percentage = map(raw_value, 0, 4095, 0, 100);
    
    // Clamp to 0-100 range
    if (percentage > 100) percentage = 100;
    
    return percentage;
}

uint16_t read_soil_moisture() {
    // Read capacitive soil moisture from Adafruit sensor
    
    if (!soil_sensor_available) {
        Serial.println("⚠️  Soil sensor not available, returning minimum value");
        return SOIL_MIN;
    }
    
    // Read capacitance value from sensor
    uint16_t capread = ss.touchRead(0);
    
    Serial.print("📊 Raw capacitance reading: ");
    Serial.println(capread);
    
    // Clamp to expected range (200-2000)
    // Lower capacitance = drier soil
    // Higher capacitance = wetter soil
    if (capread < SOIL_MIN) capread = SOIL_MIN;
    if (capread > SOIL_MAX) capread = SOIL_MAX;
    
    return capread;
}

uint8_t calculate_parity(uint32_t data) {
    // Count the number of 1s in the lower 31 bits
    uint8_t count = 0;
    uint32_t temp = data & 0x7FFFFFFF; // Mask out bit 31
    
    while (temp) {
        count += temp & 1;
        temp >>= 1;
    }
    
    // Even parity: return 1 if count is odd, 0 if even
    return (count & 1);
}

uint32_t build_data_packet(uint8_t health_class) {
    uint32_t packet = 0;
    
    // Read sensor values
    uint8_t battery = read_battery_percentage();
    uint16_t soil = read_soil_moisture();
    
    Serial.println("\n[Building Data Packet]");
    Serial.printf("  Battery: %d%%\n", battery);
    Serial.printf("  Soil Moisture: %d\n", soil);
    
    // Build the packet bit by bit
    // Bits 0-2: Node Type (3 bits)
    packet |= (NODE_TYPE & 0x07);
    
    // Bits 3-10: Node ID (8 bits)
    packet |= ((uint32_t)(NODE_ID & 0xFF)) << 3;
    
    // Bits 11-12: Health Classification (2 bits)
    packet |= ((uint32_t)(health_class & 0x03)) << 11;
    
    // Bits 13-19: Battery Percentage (7 bits, 0-100)
    packet |= ((uint32_t)(battery & 0x7F)) << 13;
    
    // Bits 20-30: Soil Moisture (11 bits, 200-2000)
    // Store as offset from 200 to fit in 11 bits (range 0-1800)
    uint16_t soil_offset = soil - 200;
    packet |= ((uint32_t)(soil_offset & 0x7FF)) << 20;
    
    // Bit 31: Even Parity
    uint8_t parity = calculate_parity(packet);
    packet |= ((uint32_t)parity) << 31;
    
    return packet;
}

void print_data_packet(uint32_t packet) {
    Serial.println("\n┌─────────────────────────────────────┐");
    Serial.println("│       DATA PACKET BREAKDOWN         │");
    Serial.println("├─────────────────────────────────────┤");
    
    // Extract each field
    uint8_t node_type = packet & 0x07;
    uint8_t node_id = (packet >> 3) & 0xFF;
    uint8_t health = (packet >> 11) & 0x03;
    uint8_t battery = (packet >> 13) & 0x7F;
    uint16_t soil = ((packet >> 20) & 0x7FF) + 200;
    uint8_t parity = (packet >> 31) & 0x01;
    
    // Print each field
    Serial.printf("│ Node Type (0-2):       %d            │\n", node_type);
    Serial.printf("│ Node ID (3-10):        %d            │\n", node_id);
    Serial.print("│ Health (11-12):        ");
    if (health == BACTERIAL) Serial.println("Bacterial    │");
    else if (health == HEALTHY) Serial.println("Healthy      │");
    else if (health == INVALID) Serial.println("Invalid      │");
    else Serial.println("Unknown     │");
    
    Serial.printf("│ Battery (13-19):       %d%%           │\n", battery);
    Serial.printf("│ Soil Moisture (20-30): %d          │\n", soil);
    Serial.printf("│ Parity Bit (31):       %d            │\n", parity);
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Full Packet (hex):     0x%08X   │\n", packet);
    Serial.printf("│ Full Packet (dec):     %u    │\n", packet);
    Serial.println("└─────────────────────────────────────┘\n");
}

void print_packet_binary(uint32_t packet) {
    Serial.println("Binary representation (MSB first):");
    Serial.print("Bit 31 (Parity):      ");
    for (int i = 31; i >= 31; i--) {
        Serial.print((packet >> i) & 1);
    }
    Serial.println();
    
    Serial.print("Bits 20-30 (Soil):    ");
    for (int i = 30; i >= 20; i--) {
        Serial.print((packet >> i) & 1);
        if (i == 25) Serial.print(" "); // Space for readability
    }
    Serial.println();
    
    Serial.print("Bits 13-19 (Battery): ");
    for (int i = 19; i >= 13; i--) {
        Serial.print((packet >> i) & 1);
    }
    Serial.println();
    
    Serial.print("Bits 11-12 (Health):  ");
    for (int i = 12; i >= 11; i--) {
        Serial.print((packet >> i) & 1);
    }
    Serial.println();
    
    Serial.print("Bits 3-10 (Node ID):  ");
    for (int i = 10; i >= 3; i--) {
        Serial.print((packet >> i) & 1);
        if (i == 7) Serial.print(" "); // Space for readability
    }
    Serial.println();
    
    Serial.print("Bits 0-2 (Node Type): ");
    for (int i = 2; i >= 0; i--) {
        Serial.print((packet >> i) & 1);
    }
    Serial.println("\n");
}

// Utility function to verify parity
bool verify_packet_parity(uint32_t packet) {
    uint8_t received_parity = (packet >> 31) & 0x01;
    uint8_t calculated_parity = calculate_parity(packet);
    return (received_parity == calculated_parity);
}

#endif // DATA_PACKET_H