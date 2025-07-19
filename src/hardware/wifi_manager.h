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
    FAILED
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
        
        if (state == WiFiState::CONNECTING) {
            // Check if connection timed out
            if (millis() - lastConnectionAttempt > connectionTimeout) {
                LOG_WARN("WiFi connection timeout");
                state = WiFiState::FAILED;
                return ErrorCode::WIFI_CONNECTION_FAILED;
            }
            return ErrorCode::PENDING;
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
                // Retry after 10 seconds
                if (millis() - lastConnectionAttempt > 10000) {
                    state = WiFiState::DISCONNECTED;
                }
                break;
                
            default:
                break;
        }
    }
    
    bool isConnected() const {
        return state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
    }
    
    bool isConnecting() const {
        return state == WiFiState::CONNECTING;
    }
    
    String getLocalIP() const {
        return WiFi.localIP().toString();
    }
    
private:
    const WiFiConfig& config;
    WiFiState state;
    unsigned long lastConnectionAttempt;
    unsigned long connectionTimeout;
};

#endif