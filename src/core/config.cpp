#include "config.h"
#include <SPIFFS.h>
#include <string>

std::string readFile(fs::FS &fs, const char* path) {
    File file = fs.open(path, "r");
    if (!file || file.isDirectory()) {
        return std::string();
    }
    
    std::string content;
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    return content;
}

ErrorCode Config::loadFromFile(const char* filename) {
    std::string content = readFile(SPIFFS, filename);
    if (content.empty()) {
        return ErrorCode::FILE_READ_FAILED;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content.c_str());
    if (error) {
        return ErrorCode::FILE_READ_FAILED;
    }
    
    JsonObject root = doc.as<JsonObject>();
    
    ErrorCode result = parseWiFiConfig(root);
    if (result != ErrorCode::SUCCESS) return result;
    
    result = parseMQTTConfig(root);
    if (result != ErrorCode::SUCCESS) return result;
    
    result = parseSensorConfig(root);
    if (result != ErrorCode::SUCCESS) return result;
    
    return ErrorCode::SUCCESS;
}

ErrorCode Config::saveToFile(const char* filename) {
    JsonDocument doc;
    serializeToJson(doc);
    
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        return ErrorCode::FILE_READ_FAILED;
    }
    
    serializeJson(doc, file);
    file.close();
    return ErrorCode::SUCCESS;
}

bool Config::validate() {
    return wifi.isValid() && mqtt.isValid();
}

void Config::setDefaults() {
    // WiFi defaults - no hardcoded credentials, must be configured via web UI
    strcpy(wifi.ssid, "");
    strcpy(wifi.password, "");
    strcpy(wifi.username, "");
    wifi.isEnterprise = false;
    
    // MQTT defaults - matching original working config
    strcpy(mqtt.broker, "192.168.31.21");
    strcpy(mqtt.username, "user");
    strcpy(mqtt.password, "passwd");
    strcpy(mqtt.edgeId, "24dcc3a736ec");
    mqtt.port = 1883;
    
    // Sensor defaults from original define.h
    sensor.dhtPin = 13;
    sensor.dhtType = 11;
    sensor.photoresisterPin = 39;
    sensor.ledPin = 25;
    sensor.sdaPin = 32;
    sensor.sclPin = 33;
    sensor.photoresisterThreshold = 800;
    sensor.sensorReadingInterval = 200;
    sensor.uploadFrequency = 5000;
    sensor.nightLightDuration = 600000; // 10 minutes
}

ErrorCode Config::parseWiFiConfig(JsonObject& obj) {
    if (obj.containsKey("wifi")) {
        JsonObject wifiObj = obj["wifi"];
        if (wifiObj.containsKey("ssid")) {
            strncpy(wifi.ssid, wifiObj["ssid"], sizeof(wifi.ssid) - 1);
        }
        if (wifiObj.containsKey("password")) {
            strncpy(wifi.password, wifiObj["password"], sizeof(wifi.password) - 1);
        }
        if (wifiObj.containsKey("username")) {
            strncpy(wifi.username, wifiObj["username"], sizeof(wifi.username) - 1);
        }
        if (wifiObj.containsKey("isEnterprise")) {
            wifi.isEnterprise = wifiObj["isEnterprise"];
        }
    }
    return ErrorCode::SUCCESS;
}

ErrorCode Config::parseMQTTConfig(JsonObject& obj) {
    if (obj.containsKey("mqtt")) {
        JsonObject mqttObj = obj["mqtt"];
        if (mqttObj.containsKey("broker")) {
            strncpy(mqtt.broker, mqttObj["broker"], sizeof(mqtt.broker) - 1);
        }
        if (mqttObj.containsKey("username")) {
            strncpy(mqtt.username, mqttObj["username"], sizeof(mqtt.username) - 1);
        }
        if (mqttObj.containsKey("password")) {
            strncpy(mqtt.password, mqttObj["password"], sizeof(mqtt.password) - 1);
        }
        if (mqttObj.containsKey("edgeId")) {
            strncpy(mqtt.edgeId, mqttObj["edgeId"], sizeof(mqtt.edgeId) - 1);
        }
        if (mqttObj.containsKey("port")) {
            mqtt.port = mqttObj["port"];
        }
    }
    return ErrorCode::SUCCESS;
}

ErrorCode Config::parseSensorConfig(JsonObject& obj) {
    if (obj.containsKey("sensor")) {
        JsonObject sensorObj = obj["sensor"];
        if (sensorObj.containsKey("dhtPin")) {
            sensor.dhtPin = sensorObj["dhtPin"];
        }
        if (sensorObj.containsKey("dhtType")) {
            sensor.dhtType = sensorObj["dhtType"];
        }
        if (sensorObj.containsKey("photoresisterPin")) {
            sensor.photoresisterPin = sensorObj["photoresisterPin"];
        }
        if (sensorObj.containsKey("ledPin")) {
            sensor.ledPin = sensorObj["ledPin"];
        }
        if (sensorObj.containsKey("sdaPin")) {
            sensor.sdaPin = sensorObj["sdaPin"];
        }
        if (sensorObj.containsKey("sclPin")) {
            sensor.sclPin = sensorObj["sclPin"];
        }
        if (sensorObj.containsKey("photoresisterThreshold")) {
            sensor.photoresisterThreshold = sensorObj["photoresisterThreshold"];
        }
        if (sensorObj.containsKey("sensorReadingInterval")) {
            sensor.sensorReadingInterval = sensorObj["sensorReadingInterval"];
        }
        if (sensorObj.containsKey("uploadFrequency")) {
            sensor.uploadFrequency = sensorObj["uploadFrequency"];
        }
        if (sensorObj.containsKey("nightLightDuration")) {
            sensor.nightLightDuration = sensorObj["nightLightDuration"];
        }
    }
    return ErrorCode::SUCCESS;
}

void Config::serializeToJson(JsonDocument& doc) {
    JsonObject wifiObj = doc["wifi"].to<JsonObject>();
    wifiObj["ssid"] = wifi.ssid;
    wifiObj["password"] = wifi.password;
    wifiObj["username"] = wifi.username;
    wifiObj["isEnterprise"] = wifi.isEnterprise;
    
    JsonObject mqttObj = doc["mqtt"].to<JsonObject>();
    mqttObj["broker"] = mqtt.broker;
    mqttObj["username"] = mqtt.username;
    mqttObj["password"] = mqtt.password;
    mqttObj["edgeId"] = mqtt.edgeId;
    mqttObj["port"] = mqtt.port;
    
    JsonObject sensorObj = doc["sensor"].to<JsonObject>();
    sensorObj["dhtPin"] = sensor.dhtPin;
    sensorObj["dhtType"] = sensor.dhtType;
    sensorObj["photoresisterPin"] = sensor.photoresisterPin;
    sensorObj["ledPin"] = sensor.ledPin;
    sensorObj["sdaPin"] = sensor.sdaPin;
    sensorObj["sclPin"] = sensor.sclPin;
    sensorObj["photoresisterThreshold"] = sensor.photoresisterThreshold;
    sensorObj["sensorReadingInterval"] = sensor.sensorReadingInterval;
    sensorObj["uploadFrequency"] = sensor.uploadFrequency;
    sensorObj["nightLightDuration"] = sensor.nightLightDuration;
}