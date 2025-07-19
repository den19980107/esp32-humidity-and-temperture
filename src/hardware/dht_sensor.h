#ifndef HARDWARE_DHT_SENSOR_H
#define HARDWARE_DHT_SENSOR_H

#include <DHT.h>
#include "../core/interfaces.h"
#include "../core/logger.h"

class DHTSensor : public ISensorReader {
public:
    DHTSensor(int pin, int type, int photoresisterPin, int ledPin)
        : dht(pin, type), photoPin(photoresisterPin), ledPin(ledPin), 
          lastReadTime(0), readInterval(1000) {
        pinMode(ledPin, OUTPUT);
        pinMode(photoresisterPin, INPUT);
    }
    
    Result<SensorData> read() override {
        unsigned long now = millis();
        if (now - lastReadTime < readInterval) {
            return Result<SensorData>(lastData);
        }
        
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();
        
        if (isnan(humidity) || isnan(temperature)) {
            LOG_ERROR("Failed to read from DHT sensor");
            return Result<SensorData>(ErrorCode::SENSOR_READ_FAILED);
        }
        
        int photoValue = analogRead(photoPin);
        bool ledOn = digitalRead(ledPin);
        String ledState = ledOn ? "on" : "off";
        unsigned long freeHeap = ESP.getFreeHeap();
        unsigned long minFreeHeap = ESP.getMinFreeHeap();
        
        lastData = SensorData(temperature, humidity, photoValue, ledOn, ledState, freeHeap, minFreeHeap);
        lastReadTime = now;
        
        LOG_DEBUGF("Sensor read - Temp: %.1f°C, Humidity: %.1f%%, Light: %d", 
                   temperature, humidity, photoValue);
        
        return Result<SensorData>(lastData);
    }
    
    bool isReady() override {
        return millis() - lastReadTime >= readInterval;
    }
    
    void setReadInterval(unsigned long interval) {
        readInterval = interval;
    }
    
private:
    DHT dht;
    int photoPin;
    int ledPin;
    unsigned long lastReadTime;
    unsigned long readInterval;
    SensorData lastData;
};

#endif