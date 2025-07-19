#ifndef CORE_EVENT_BUS_H
#define CORE_EVENT_BUS_H

#include <functional>
#include <vector>
#include <map>
#include "interfaces.h"

enum class EventType {
    SENSOR_DATA_UPDATED,
    LED_STATUS_CHANGED,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    DISPLAY_UPDATE_REQUIRED,
    ERROR_OCCURRED
};

struct Event {
    EventType type;
    SensorData sensorData;
    bool boolValue;
    ErrorCode errorCode;
    const char* message;
    
    Event(EventType t) : type(t), boolValue(false), errorCode(ErrorCode::SUCCESS), message(nullptr) {}
    Event(EventType t, const SensorData& data) : type(t), sensorData(data), boolValue(false), errorCode(ErrorCode::SUCCESS), message(nullptr) {}
    Event(EventType t, bool value) : type(t), boolValue(value), errorCode(ErrorCode::SUCCESS), message(nullptr) {}
    Event(EventType t, ErrorCode error, const char* msg = nullptr) : type(t), boolValue(false), errorCode(error), message(msg) {}
};

class EventBus {
public:
    using EventHandler = std::function<void(const Event&)>;
    
    void subscribe(EventType type, EventHandler handler) {
        handlers[type].push_back(handler);
    }
    
    void publish(const Event& event) {
        auto it = handlers.find(event.type);
        if (it != handlers.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }
    
    void clear() {
        handlers.clear();
    }
    
private:
    std::map<EventType, std::vector<EventHandler>> handlers;
};

#endif