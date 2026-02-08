#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include "esp_heap_caps.h"

#define TFLM_USE_PSRAM
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include "classifier_96_int8f.h"

// TinyML Configuration
#define ARENA_SIZE      2 * 1024 * 1024
#define TF_NUM_OPS      10
#define TF_NUM_INPUTS   27648 //96*96*3
#define TF_NUM_OUTPUTS  3

using EloquentTF = Eloquent::TF::Sequential<TF_NUM_OPS, ARENA_SIZE>;

// Global variables
extern EloquentTF* tf;
extern int8_t* input_buffer;
extern const char* class_names[3];

// Function declarations
void init_tinyml();
void run_inference();
void emptyBuffer(int8_t* bufferName, size_t bufferSize);

// ========== Implementation ==========

EloquentTF* tf = nullptr;
int8_t* input_buffer = nullptr;
const char* class_names[] = {"Bacterial", "Healthy", "Invalid"};

void init_tinyml() {
    Serial.println("\n[TinyML Setup]");
    
    // Allocate TF object in PSRAM
    void* tf_memory = heap_caps_malloc(sizeof(EloquentTF), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (tf_memory == NULL) {
        Serial.println("❌ Failed to allocate TF object in PSRAM!");
        while(1);
    }
    tf = new (tf_memory) EloquentTF();
    Serial.println("✅ TF Object allocated in PSRAM");

    // Allocate input buffer in PSRAM
    input_buffer = (int8_t*)heap_caps_malloc(TF_NUM_INPUTS*sizeof(int8_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (input_buffer == NULL) {
        Serial.println("❌ Failed to allocate Input Buffer in PSRAM!");
        while(1);
    }
    Serial.println("✅ Input Buffer allocated in PSRAM");

    // Configure model
    tf->setNumInputs(TF_NUM_INPUTS);
    tf->setNumOutputs(TF_NUM_OUTPUTS);
    tf->resolver.AddPad();
    tf->resolver.AddConv2D();
    tf->resolver.AddMul();
    tf->resolver.AddAdd();
    tf->resolver.AddLogistic();
    tf->resolver.AddStridedSlice();
    tf->resolver.AddFullyConnected();
    tf->resolver.AddSoftmax();
    tf->resolver.AddMean();
    tf->resolver.AddConcatenation();

    // Initialize model
    Serial.println("Initializing TinyML model...");
    while(!tf->begin(full_integer_quant_tflite).isOk()) {
        Serial.println(tf->exception.toString());
        delay(1000);
    }
    Serial.println("✅ TinyML model initialized");
}

void run_inference() {
    Serial.println("\n🧠 Running inference...");
    
    if(!tf->predict(input_buffer).isOk()) {
        Serial.println("❌ Inference failed:");
        Serial.println(tf->exception.toString());
        return;
    }
    
    // Print results
    Serial.println("-------------------------------");
    Serial.print("🎯 Predicted Class: ");
    Serial.print(tf->classification);
    Serial.print(" (");
    Serial.print(class_names[tf->classification]);
    Serial.println(")");
    
    Serial.print("⏱️  Inference time: ");
    Serial.print(tf->benchmark.microseconds());
    Serial.println(" μs");
    
    // Print confidence scores for all classes
    // Serial.println("\nConfidence scores:");
    // for(int i = 0; i < TF_NUM_OUTPUTS; i++) {
    //     Serial.print("  ");
    //     Serial.print(class_names[i]);
    //     Serial.print(": ");
    //     Serial.println(tf->proba(i));
    // }
    
    Serial.println("==============================\n");
    
    // Clear buffer
    emptyBuffer(input_buffer, TF_NUM_INPUTS*sizeof(int8_t));
}

void emptyBuffer(int8_t* bufferName, size_t bufferSize) {
    if (bufferName != NULL) {
        memset(bufferName, 0, bufferSize);
    }
}

#endif // ML_INFERENCE_H