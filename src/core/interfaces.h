#ifndef CORE_INTERFACES_H
#define CORE_INTERFACES_H

#include <functional>
#include <memory>
#include <Arduino.h>

struct SensorData {
    float temperture;
    float humidity;
    int photoresisterValue;
    bool ledOn;
    String ledState;
    unsigned long freeMemory;
    unsigned long lowestMemory;
    
    SensorData() : temperture(0), humidity(0), photoresisterValue(0), 
                   ledOn(false), ledState("off"), freeMemory(0), lowestMemory(0) {}
    
    SensorData(float temp, float hum, int photo, bool led, String state, unsigned long free, unsigned long lowest)
        : temperture(temp), humidity(hum), photoresisterValue(photo), 
          ledOn(led), ledState(state), freeMemory(free), lowestMemory(lowest) {}
};

struct DisplayData {
    SensorData sensorData;
    bool showLedStatus;
    bool ledStatus;
    unsigned long displayDuration;
    bool showLedTimer;
    unsigned long ledTimerRemaining;  // Remaining seconds
    
    DisplayData() : showLedStatus(false), ledStatus(false), displayDuration(0), 
                   showLedTimer(false), ledTimerRemaining(0) {}
};

enum class ErrorCode {
    SUCCESS,
    PENDING,
    SENSOR_READ_FAILED,
    DISPLAY_INIT_FAILED,
    WIFI_CONNECTION_FAILED,
    MQTT_CONNECTION_FAILED,
    MQTT_PUBLISH_FAILED,
    FILE_READ_FAILED,
    MEMORY_ALLOCATION_FAILED
};

template<typename T>
class Result {
public:
    ErrorCode error;
    T value;
    
    Result(ErrorCode err) : error(err), value(T{}) {}
    Result(T val) : error(ErrorCode::SUCCESS), value(val) {}
    
    bool isSuccess() const { return error == ErrorCode::SUCCESS; }
    bool isError() const { return error != ErrorCode::SUCCESS; }
};

class ISensorReader {
public:
    virtual ~ISensorReader() = default;
    virtual Result<SensorData> read() = 0;
    virtual bool isReady() = 0;
};

class IDisplayDriver {
public:
    virtual ~IDisplayDriver() = default;
    virtual ErrorCode initialize() = 0;
    virtual ErrorCode show(const DisplayData& data) = 0;
    virtual ErrorCode clear() = 0;
};

class ILedController {
public:
    virtual ~ILedController() = default;
    virtual ErrorCode turnOn() = 0;
    virtual ErrorCode turnOff() = 0;
    virtual bool isOn() = 0;
};

class INetworkManager {
public:
    virtual ~INetworkManager() = default;
    virtual ErrorCode connectWiFi(const char* ssid, const char* password) = 0;
    virtual ErrorCode startAccessPoint(const char* ssid) = 0;
    virtual bool isConnected() = 0;
    virtual const char* getIPAddress() = 0;
};

class IMqttClient {
public:
    virtual ~IMqttClient() = default;
    virtual ErrorCode connect(const char* broker, const char* username, const char* password) = 0;
    virtual ErrorCode publish(const char* topic, const char* payload) = 0;
    virtual ErrorCode subscribe(const char* topic, std::function<void(const char*)> callback) = 0;
    virtual bool isConnected() = 0;
};

#endif