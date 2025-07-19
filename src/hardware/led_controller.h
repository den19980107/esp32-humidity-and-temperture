#ifndef HARDWARE_LED_CONTROLLER_H
#define HARDWARE_LED_CONTROLLER_H

#include <Arduino.h>
#include "../core/interfaces.h"
#include "../core/logger.h"

class LEDController : public ILedController {
public:
    LEDController(int pin) : ledPin(pin), currentState(false) {
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
    }
    
    ErrorCode turnOn() override {
        digitalWrite(ledPin, HIGH);
        currentState = true;
        LOG_DEBUG("LED turned ON");
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode turnOff() override {
        digitalWrite(ledPin, LOW);
        currentState = false;
        LOG_DEBUG("LED turned OFF");
        return ErrorCode::SUCCESS;
    }
    
    bool isOn() override {
        return currentState;
    }
    
private:
    int ledPin;
    bool currentState;
};

#endif