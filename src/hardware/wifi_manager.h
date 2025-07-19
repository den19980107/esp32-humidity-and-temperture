#ifndef HARDWARE_WIFI_MANAGER_H
#define HARDWARE_WIFI_MANAGER_H

#include <WiFi.h>
#include "../core/interfaces.h"
#include "../core/logger.h"
#include "../core/config.h"

enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED,
    AP_MODE
};

class WiFiManager {
public:
    WiFiManager(const WiFiConfig& config) 
        : config(config), state(WiFiState::DISCONNECTED), 
          lastConnectionAttempt(0), connectionTimeout(30000) {}
    
    ErrorCode initialize() {
        WiFi.mode(WIFI_STA);
        LOG_INFO("WiFi manager initialized");
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode connect() {
        if (state == WiFiState::CONNECTED) {
            return ErrorCode::SUCCESS;
        }
        
        if (state == WiFiState::AP_MODE) {
            return ErrorCode::SUCCESS; // AP mode is also a valid "connected" state
        }
        
        if (state == WiFiState::CONNECTING) {
            // Check if connection timed out
            if (millis() - lastConnectionAttempt > connectionTimeout) {
                LOG_WARN("WiFi connection timeout");
                state = WiFiState::FAILED;
                return ErrorCode::WIFI_CONNECTION_FAILED;
            }
            return ErrorCode::PENDING;
        }
        
        // Check if WiFi credentials are configured
        if (strlen(config.ssid) == 0) {
            LOG_INFO("No WiFi credentials configured, starting Access Point mode");
            return startAccessPointMode();
        }
        
        LOG_INFOF("Connecting to WiFi: %s", config.ssid);
        
        if (strlen(config.password) > 0) {
            WiFi.begin(config.ssid, config.password);
        } else {
            WiFi.begin(config.ssid);
        }
        
        state = WiFiState::CONNECTING;
        lastConnectionAttempt = millis();
        
        return ErrorCode::PENDING;
    }
    
    void update() {
        switch (state) {
            case WiFiState::CONNECTING:
                if (WiFi.status() == WL_CONNECTED) {
                    state = WiFiState::CONNECTED;
                    LOG_INFOF("[wifi connected] ip: %s", WiFi.localIP().toString().c_str());
                } else if (millis() - lastConnectionAttempt > connectionTimeout) {
                    state = WiFiState::FAILED;
                    LOG_WARN("WiFi connection failed - timeout");
                }
                break;
                
            case WiFiState::CONNECTED:
                if (WiFi.status() != WL_CONNECTED) {
                    state = WiFiState::DISCONNECTED;
                    LOG_WARN("WiFi connection lost");
                }
                break;
                
            case WiFiState::FAILED:
                // Retry after 10 seconds, but only if we have credentials
                if (millis() - lastConnectionAttempt > 10000) {
                    if (strlen(config.ssid) > 0) {
                        state = WiFiState::DISCONNECTED;
                    } else {
                        // No credentials, go to AP mode
                        startAccessPointMode();
                    }
                }
                break;
                
            case WiFiState::AP_MODE:
                // AP mode is stable, nothing to update
                break;
                
            default:
                break;
        }
    }
    
    bool isConnected() const {
        return (state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED) ||
               (state == WiFiState::AP_MODE);
    }
    
    bool isConnecting() const {
        return state == WiFiState::CONNECTING;
    }
    
    bool isInAPMode() const {
        return state == WiFiState::AP_MODE;
    }
    
    String getLocalIP() const {
        if (state == WiFiState::AP_MODE) {
            return WiFi.softAPIP().toString();
        }
        return WiFi.localIP().toString();
    }
    
private:
    const WiFiConfig& config;
    WiFiState state;
    unsigned long lastConnectionAttempt;
    unsigned long connectionTimeout;
    
    ErrorCode startAccessPointMode() {
        WiFi.mode(WIFI_AP);
        
        // Create AP name based on MAC address for uniqueness
        String apName = "ESP32-Config-" + WiFi.macAddress().substring(12, 17);
        apName.replace(":", "");
        
        if (WiFi.softAP(apName.c_str())) {
            state = WiFiState::AP_MODE;
            LOG_INFOF("[AP Mode] Started access point: %s", apName.c_str());
            LOG_INFOF("[AP Mode] IP address: %s", WiFi.softAPIP().toString().c_str());
            LOG_INFO("[AP Mode] Connect to configure WiFi credentials");
            return ErrorCode::SUCCESS;
        } else {
            LOG_ERROR("[AP Mode] Failed to start access point");
            return ErrorCode::WIFI_CONNECTION_FAILED;
        }
    }
};

#endif