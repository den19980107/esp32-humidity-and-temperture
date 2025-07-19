#ifndef HARDWARE_MQTT_CLIENT_H
#define HARDWARE_MQTT_CLIENT_H

#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "../core/interfaces.h"
#include "../core/logger.h"
#include "../core/config.h"

class MQTTClient {
public:
    MQTTClient(const MQTTConfig& config) 
        : config(config), client(wifiClient), connected(false), 
          lastReconnectAttempt(0), manualLedControl(false) {}
    
    ErrorCode initialize() {
        client.setServer(config.broker, config.port);
        client.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->onMessage(topic, payload, length);
        });
        client.setBufferSize(512);
        
        LOG_INFO("MQTT client initialized");
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode connect() {
        if (connected) {
            return ErrorCode::SUCCESS;
        }
        
        LOG_INFOF("Connecting to MQTT broker: %s:%d", config.broker, config.port);
        LOG_INFOF("Using edgeId: %s, username: %s", config.edgeId, config.username);
        
        if (client.connect(config.edgeId, config.username, config.password)) {
            connected = true;
            LOG_INFO("MQTT connected successfully");
            
            // Subscribe to LED control topic
            String ledTopic = String("Advantech/") + config.edgeId + "/led";
            if (client.subscribe(ledTopic.c_str())) {
                LOG_INFOF("Subscribed to: %s", ledTopic.c_str());
            }
            
            // Publish Home Assistant discovery
            publishHomeAssistantDiscovery();
            
            return ErrorCode::SUCCESS;
        } else {
            LOG_ERRORF("MQTT connection failed, rc=%d", client.state());
            return ErrorCode::MQTT_CONNECTION_FAILED;
        }
    }
    
    void update() {
        if (connected) {
            if (!client.connected()) {
                connected = false;
                LOG_WARN("MQTT connection lost");
            } else {
                client.loop();
            }
        } else {
            // Try to reconnect every 5 seconds
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 5000) {
                lastReconnectAttempt = now;
                connect();
            }
        }
    }
    
    ErrorCode publishSensorData(const SensorData& data) {
        if (!connected || !client.connected()) {
            return ErrorCode::MQTT_PUBLISH_FAILED;
        }
        
        // Create JSON payload
        DynamicJsonDocument doc(256);
        doc["temp"] = data.temperture;
        doc["humi"] = data.humidity;
        doc["photoresister"] = data.photoresisterValue;
        doc["ledState"] = data.ledState;
        doc["freeMemory"] = data.freeMemory;
        doc["lowestMemory"] = data.lowestMemory;
        
        String payload;
        serializeJson(doc, payload);
        
        String topic = String("Advantech/") + config.edgeId + "/data";
        
        if (client.publish(topic.c_str(), payload.c_str())) {
            LOG_INFOF("[publish success] topic: %s, payload: %s", topic.c_str(), payload.c_str());
            return ErrorCode::SUCCESS;
        } else {
            LOG_ERRORF("Failed to publish to topic: %s", topic.c_str());
            return ErrorCode::MQTT_PUBLISH_FAILED;
        }
    }
    
    bool isConnected() {
        return connected && client.connected();
    }
    
    void setLedCallback(std::function<void(bool)> callback) {
        ledCallback = callback;
    }
    
private:
    const MQTTConfig& config;
    WiFiClient wifiClient;
    PubSubClient client;
    bool connected;
    unsigned long lastReconnectAttempt;
    bool manualLedControl;
    std::function<void(bool)> ledCallback;
    
    void onMessage(char* topic, byte* payload, unsigned int length) {
        // Convert payload to string
        char message[length + 1];
        memcpy(message, payload, length);
        message[length] = '\0';
        
        String topicStr = String(topic);
        String expectedTopic = String("Advantech/") + config.edgeId + "/led";
        
        if (topicStr == expectedTopic) {
            bool ledOn = (strcmp(message, "on") == 0);
            LOG_INFOF("*** MANUAL LED CONTROL from Home Assistant: %s ***", ledOn ? "ON" : "OFF");
            
            if (ledCallback) {
                ledCallback(ledOn);
            }
        }
    }
    
    void publishHomeAssistantDiscovery() {
        publishTemperatureDiscovery();
        publishHumidityDiscovery();
        publishLightDiscovery();
        publishMemoryDiscovery();
        publishLedDiscovery();
    }
    
    void publishTemperatureDiscovery() {
        DynamicJsonDocument doc(512);
        doc["name"] = String(config.edgeId) + " Temperature";
        doc["device_class"] = "temperature";
        doc["state_topic"] = String("Advantech/") + config.edgeId + "/data";
        doc["unit_of_measurement"] = "°C";
        doc["value_template"] = "{{ value_json.temp }}";
        doc["unique_id"] = String(config.edgeId) + "_temperature";
        
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"] = config.edgeId;
        device["name"] = String("ESP32 Sensor ") + config.edgeId;
        device["model"] = "ESP32 Environmental Monitor";
        device["manufacturer"] = "DIY";
        
        String payload;
        serializeJson(doc, payload);
        
        String topic = String("homeassistant/sensor/") + config.edgeId + "/temperature/config";
        client.publish(topic.c_str(), payload.c_str(), true);
    }
    
    void publishHumidityDiscovery() {
        DynamicJsonDocument doc(512);
        doc["name"] = String(config.edgeId) + " Humidity";
        doc["device_class"] = "humidity";
        doc["state_topic"] = String("Advantech/") + config.edgeId + "/data";
        doc["unit_of_measurement"] = "%";
        doc["value_template"] = "{{ value_json.humi }}";
        doc["unique_id"] = String(config.edgeId) + "_humidity";
        
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"] = config.edgeId;
        device["name"] = String("ESP32 Sensor ") + config.edgeId;
        device["model"] = "ESP32 Environmental Monitor";
        device["manufacturer"] = "DIY";
        
        String payload;
        serializeJson(doc, payload);
        
        String topic = String("homeassistant/sensor/") + config.edgeId + "/humidity/config";
        client.publish(topic.c_str(), payload.c_str(), true);
    }
    
    void publishLightDiscovery() {
        DynamicJsonDocument doc(512);
        doc["name"] = String(config.edgeId) + " Light";
        doc["device_class"] = "illuminance";
        doc["state_topic"] = String("Advantech/") + config.edgeId + "/data";
        doc["unit_of_measurement"] = "lx";
        doc["value_template"] = "{{ value_json.photoresister }}";
        doc["unique_id"] = String(config.edgeId) + "_photoresister";
        
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"] = config.edgeId;
        device["name"] = String("ESP32 Sensor ") + config.edgeId;
        device["model"] = "ESP32 Environmental Monitor";
        device["manufacturer"] = "DIY";
        
        String payload;
        serializeJson(doc, payload);
        
        String topic = String("homeassistant/sensor/") + config.edgeId + "/photoresister/config";
        client.publish(topic.c_str(), payload.c_str(), true);
    }
    
    void publishMemoryDiscovery() {
        // Free Memory
        DynamicJsonDocument doc1(512);
        doc1["name"] = String(config.edgeId) + " Free Memory";
        doc1["state_topic"] = String("Advantech/") + config.edgeId + "/data";
        doc1["unit_of_measurement"] = "bytes";
        doc1["value_template"] = "{{ value_json.freeMemory }}";
        doc1["unique_id"] = String(config.edgeId) + "_free_memory";
        
        JsonObject device1 = doc1.createNestedObject("device");
        device1["identifiers"] = config.edgeId;
        device1["name"] = String("ESP32 Sensor ") + config.edgeId;
        device1["model"] = "ESP32 Environmental Monitor";
        device1["manufacturer"] = "DIY";
        
        String payload1;
        serializeJson(doc1, payload1);
        
        String topic1 = String("homeassistant/sensor/") + config.edgeId + "/freeMemory/config";
        client.publish(topic1.c_str(), payload1.c_str(), true);
        
        // Lowest Memory
        DynamicJsonDocument doc2(512);
        doc2["name"] = String(config.edgeId) + " Lowest Memory";
        doc2["state_topic"] = String("Advantech/") + config.edgeId + "/data";
        doc2["unit_of_measurement"] = "bytes";
        doc2["value_template"] = "{{ value_json.lowestMemory }}";
        doc2["unique_id"] = String(config.edgeId) + "_lowest_memory";
        
        JsonObject device2 = doc2.createNestedObject("device");
        device2["identifiers"] = config.edgeId;
        device2["name"] = String("ESP32 Sensor ") + config.edgeId;
        device2["model"] = "ESP32 Environmental Monitor";
        device2["manufacturer"] = "DIY";
        
        String payload2;
        serializeJson(doc2, payload2);
        
        String topic2 = String("homeassistant/sensor/") + config.edgeId + "/lowestMemory/config";
        client.publish(topic2.c_str(), payload2.c_str(), true);
    }
    
    void publishLedDiscovery() {
        DynamicJsonDocument doc(512);
        doc["name"] = String(config.edgeId) + " LED";
        doc["state_topic"] = String("Advantech/") + config.edgeId + "/data";
        doc["command_topic"] = String("Advantech/") + config.edgeId + "/led";
        doc["payload_on"] = "on";
        doc["payload_off"] = "off";
        doc["state_value_template"] = "{{ value_json.ledState }}";
        doc["unique_id"] = String(config.edgeId) + "_led";
        
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"] = config.edgeId;
        device["name"] = String("ESP32 Sensor ") + config.edgeId;
        device["model"] = "ESP32 Environmental Monitor";
        device["manufacturer"] = "DIY";
        
        String payload;
        serializeJson(doc, payload);
        
        String topic = String("homeassistant/light/") + config.edgeId + "/led/config";
        client.publish(topic.c_str(), payload.c_str(), true);
    }
};

#endif