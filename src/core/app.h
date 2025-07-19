#ifndef CORE_APP_H
#define CORE_APP_H

#include <memory>
#include <SPIFFS.h>
#include "interfaces.h"
#include "event_bus.h"
#include "config.h"
#include "logger.h"
#include "../hardware/dht_sensor.h"
#include "../hardware/oled_display.h"
#include "../hardware/led_controller.h"
#include "../hardware/wifi_manager.h"
#include "../hardware/mqtt_client.h"
#include <ESPAsyncWebServer.h>

class App {
public:
    App();
    ~App();
    
    ErrorCode initialize();
    void run();
    
private:
    // Core components
    EventBus eventBus;
    Config config;
    
    // Hardware components
    std::unique_ptr<ISensorReader> sensor;
    std::unique_ptr<IDisplayDriver> display;
    std::unique_ptr<ILedController> ledController;
    std::unique_ptr<WiFiManager> wifiManager;
    std::unique_ptr<MQTTClient> mqttClient;
    std::unique_ptr<AsyncWebServer> webServer;
    
    // State tracking
    bool initialized;
    unsigned long lastSensorRead;
    unsigned long lastDisplayUpdate;
    unsigned long lastMqttPublish;
    unsigned long ledOnTime;
    bool ledTimerActive;
    bool showingLedStatus;
    unsigned long ledStatusShowTime;
    bool manualLedControl;
    
    // Configuration
    static const char* CONFIG_FILE;
    static const unsigned long LED_STATUS_DISPLAY_DURATION = 1000;
    
    // Initialization methods
    ErrorCode initializeFileSystem();
    ErrorCode initializeHardware();
    ErrorCode setupEventHandlers();
    
    // Event handlers
    void onSensorDataUpdated(const Event& event);
    void onLedStatusChanged(const Event& event);
    void onErrorOccurred(const Event& event);
    
    // Core loop methods
    void updateSensor();
    void updateDisplay();
    void updateLedController();
    void updateWiFi();
    void updateMQTT();
    void checkLedTimer();
    
    // Helper methods
    bool shouldReadSensor();
    bool shouldUpdateDisplay();
    bool shouldPublishMqtt();
    void handleLedAutoControl(const SensorData& data);
    void onLedControlMessage(bool ledOn);
    ErrorCode publishSensorData(const SensorData& data);
    void setupWebServer();
    String getStatusHTML();
};

#endif