#include <Arduino.h>
#include <WiFi.h>
#include "core/app.h"
#include "core/logger.h"

void setup() {
    Serial.begin(115200);
    
    // Wait for serial to be ready
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("========================================");
    Serial.println("Starting ESP32 Environmental Monitor");
    Serial.println("MAC Address: " + WiFi.macAddress());
    Serial.println("========================================");
    
    App app;
    ErrorCode result = app.initialize();
    
    if (result != ErrorCode::SUCCESS) {
        Serial.println("Failed to initialize application");
        Serial.println("========================================");
        return;
    }
    
    Serial.println("App initialized successfully, starting main loop...");
    app.run();
}

void loop() {
    // Everything is handled in setup() with the while loop
}