#include "app.h"

const char* App::CONFIG_FILE = "/config.json";

App::App() 
    : initialized(false), lastSensorRead(0), lastDisplayUpdate(0), 
      lastMqttPublish(0), ledOnTime(0), ledTimerActive(false),
      showingLedStatus(false), ledStatusShowTime(0), manualLedControl(false) {
}

App::~App() {
    // Cleanup handled by smart pointers
}

ErrorCode App::initialize() {
    Logger::setLevel(LogLevel::INFO);
    LOG_INFO("Starting ESP32 Environmental Monitor");
    
    ErrorCode result = initializeFileSystem();
    if (result != ErrorCode::SUCCESS) {
        LOG_ERROR("Failed to initialize file system");
        return result;
    }
    
    // Load configuration or set defaults
    LOG_INFOF("Loading configuration from: %s", CONFIG_FILE);
    result = config.loadFromFile(CONFIG_FILE);
    if (result != ErrorCode::SUCCESS) {
        LOG_WARNF("Failed to load config (error: %d), using defaults", (int)result);
        config.setDefaults();
        LOG_INFOF("Set defaults - WiFi SSID: '%s', MQTT Broker: '%s'", config.wifi.ssid, config.mqtt.broker);
        config.saveToFile(CONFIG_FILE);
    } else {
        LOG_INFO("Configuration loaded successfully");
        LOG_INFOF("Loaded config - WiFi SSID: '%s', MQTT Broker: '%s'", config.wifi.ssid, config.mqtt.broker);
        
        // Validate loaded configuration - if key values are empty, use defaults
        if (strlen(config.wifi.ssid) == 0 || strlen(config.mqtt.broker) == 0) {
            LOG_WARN("Loaded config has empty critical values, applying defaults");
            config.setDefaults();
            LOG_INFOF("Applied defaults - WiFi SSID: '%s', MQTT Broker: '%s'", config.wifi.ssid, config.mqtt.broker);
            config.saveToFile(CONFIG_FILE);
        }
    }
    
    result = initializeHardware();
    if (result != ErrorCode::SUCCESS) {
        LOG_ERROR("Failed to initialize hardware");
        return result;
    }
    
    result = setupEventHandlers();
    if (result != ErrorCode::SUCCESS) {
        LOG_ERROR("Failed to setup event handlers");
        return result;
    }
    
    initialized = true;
    LOG_INFO("App initialization completed successfully");
    return ErrorCode::SUCCESS;
}

void App::run() {
    if (!initialized) {
        LOG_ERROR("App not initialized, cannot run");
        return;
    }
    
    LOG_INFO("*** STARTING MAIN APPLICATION LOOP ***");
    
    // Try to connect to WiFi initially
    wifiManager->connect();
    
    unsigned long lastHeartbeat = 0;
    
    while (true) {
        // Print heartbeat every 60 seconds
        if (millis() - lastHeartbeat > 60000) {
            LOG_INFOF("*** HEARTBEAT *** Uptime: %lu seconds, Free heap: %u bytes", 
                     millis() / 1000, ESP.getFreeHeap());
            lastHeartbeat = millis();
        }
        
        updateWiFi();
        updateMQTT();
        updateSensor();
        updateDisplay();
        updateLedController();
        checkLedTimer();
        
        delay(50); // Small delay to prevent watchdog issues
    }
}

ErrorCode App::initializeFileSystem() {
    if (!SPIFFS.begin(true)) {
        return ErrorCode::FILE_READ_FAILED;
    }
    LOG_INFO("SPIFFS initialized");
    return ErrorCode::SUCCESS;
}

ErrorCode App::initializeHardware() {
    // Initialize sensor
    sensor.reset(new DHTSensor(
        config.sensor.dhtPin, 
        config.sensor.dhtType,
        config.sensor.photoresisterPin,
        config.sensor.ledPin
    ));
    
    // Initialize display
    display.reset(new OLEDDisplay(
        128, 64, // OLED dimensions
        config.sensor.sdaPin,
        config.sensor.sclPin
    ));
    
    ErrorCode result = display->initialize();
    if (result != ErrorCode::SUCCESS) {
        return result;
    }
    
    // Initialize LED controller
    ledController.reset(new LEDController(config.sensor.ledPin));
    
    // Initialize WiFi and MQTT
    wifiManager.reset(new WiFiManager(config.wifi));
    mqttClient.reset(new MQTTClient(config.mqtt));
    
    result = wifiManager->initialize();
    if (result != ErrorCode::SUCCESS) {
        return result;
    }
    
    result = mqttClient->initialize();
    if (result != ErrorCode::SUCCESS) {
        return result;
    }
    
    // Set LED control callback for MQTT
    mqttClient->setLedCallback([this](bool ledOn) {
        this->onLedControlMessage(ledOn);
    });
    
    // Initialize web server for debugging
    webServer.reset(new AsyncWebServer(80));
    setupWebServer();
    
    LOG_INFO("Hardware initialized successfully");
    return ErrorCode::SUCCESS;
}

ErrorCode App::setupEventHandlers() {
    eventBus.subscribe(EventType::SENSOR_DATA_UPDATED, 
        [this](const Event& e) { onSensorDataUpdated(e); });
    
    eventBus.subscribe(EventType::LED_STATUS_CHANGED, 
        [this](const Event& e) { onLedStatusChanged(e); });
    
    eventBus.subscribe(EventType::ERROR_OCCURRED, 
        [this](const Event& e) { onErrorOccurred(e); });
    
    LOG_INFO("Event handlers setup completed");
    return ErrorCode::SUCCESS;
}

void App::updateSensor() {
    static unsigned long lastDebugPrint = 0;
    
    if (!shouldReadSensor()) return;
    
    auto result = sensor->read();
    if (result.isSuccess()) {
        lastSensorRead = millis();
        
        // Debug print every 30 seconds
        if (millis() - lastDebugPrint > 30000) {
            LOG_INFOF("[Sensor] *** READ SUCCESS *** Temp: %.1f°C, Humidity: %.1f%%, Light: %d", 
                     result.value.temperture, result.value.humidity, result.value.photoresisterValue);
            lastDebugPrint = millis();
        }
        
        handleLedAutoControl(result.value);
        eventBus.publish(Event(EventType::SENSOR_DATA_UPDATED, result.value));
    } else {
        LOG_ERRORF("[Sensor] *** READ FAILED *** Error: %d", (int)result.error);
        eventBus.publish(Event(EventType::ERROR_OCCURRED, result.error, "Sensor read failed"));
    }
}

void App::updateDisplay() {
    if (!shouldUpdateDisplay()) return;
    
    // Get current sensor data
    auto sensorResult = sensor->read();
    if (!sensorResult.isSuccess()) return;
    
    DisplayData displayData;
    displayData.sensorData = sensorResult.value;
    
    // Check if we should show LED status
    if (showingLedStatus && millis() - ledStatusShowTime < LED_STATUS_DISPLAY_DURATION) {
        displayData.showLedStatus = true;
        displayData.ledStatus = ledController->isOn();
    } else {
        showingLedStatus = false;
        displayData.showLedStatus = false;
    }
    
    display->show(displayData);
    lastDisplayUpdate = millis();
}

void App::updateLedController() {
    // LED control is handled through events and auto-control
    // This method can be used for additional LED logic if needed
}

void App::checkLedTimer() {
    if (ledTimerActive && millis() - ledOnTime >= config.sensor.nightLightDuration) {
        ledController->turnOff();
        ledTimerActive = false;
        eventBus.publish(Event(EventType::LED_STATUS_CHANGED, false));
        LOG_INFO("LED timer expired, turning off");
    }
}

bool App::shouldReadSensor() {
    return millis() - lastSensorRead >= config.sensor.sensorReadingInterval;
}

bool App::shouldUpdateDisplay() {
    return millis() - lastDisplayUpdate >= 100; // Update display at 10Hz
}

bool App::shouldPublishMqtt() {
    return millis() - lastMqttPublish >= config.sensor.uploadFrequency;
}

void App::handleLedAutoControl(const SensorData& data) {
    // Skip automatic control if manually controlled
    if (manualLedControl) {
        return;
    }
    
    bool shouldTurnOn = data.photoresisterValue < config.sensor.photoresisterThreshold;
    bool currentlyOn = ledController->isOn();
    
    // Always turn off immediately if light is bright enough
    if (!shouldTurnOn && currentlyOn) {
        ledController->turnOff();
        ledTimerActive = false;  // Cancel timer when turning off due to bright light
        eventBus.publish(Event(EventType::LED_STATUS_CHANGED, false));
        LOG_INFOF("Auto-turning LED OFF (room is bright) - Light: %d >= %d", data.photoresisterValue, config.sensor.photoresisterThreshold);
    } else if (shouldTurnOn && !currentlyOn) {
        ledController->turnOn();
        ledOnTime = millis();
        ledTimerActive = true;
        eventBus.publish(Event(EventType::LED_STATUS_CHANGED, true));
        LOG_INFOF("Auto-turning LED ON (dark room detected) - Light: %d < %d", data.photoresisterValue, config.sensor.photoresisterThreshold);
    }
}

ErrorCode App::publishSensorData(const SensorData& data) {
    if (!mqttClient->isConnected()) {
        return ErrorCode::MQTT_PUBLISH_FAILED;
    }
    
    return mqttClient->publishSensorData(data);
}

void App::onSensorDataUpdated(const Event& event) {
    // Publish to MQTT if it's time
    if (shouldPublishMqtt()) {
        if (mqttClient->isConnected()) {
            LOG_INFOF("[MQTT] Publishing sensor data - Temp: %.1f°C, Humidity: %.1f%%, Light: %d", 
                     event.sensorData.temperture, event.sensorData.humidity, event.sensorData.photoresisterValue);
            ErrorCode result = publishSensorData(event.sensorData);
            if (result == ErrorCode::SUCCESS) {
                LOG_INFO("[MQTT] Sensor data published successfully");
            } else {
                LOG_ERROR("[MQTT] Failed to publish sensor data");
            }
        } else {
            LOG_WARN("[MQTT] Cannot publish - not connected");
        }
        lastMqttPublish = millis();
    }
}

void App::onLedStatusChanged(const Event& event) {
    // Show LED status on display
    showingLedStatus = true;
    ledStatusShowTime = millis();
    LOG_INFOF("LED status changed to: %s", event.boolValue ? "ON" : "OFF");
}

void App::onErrorOccurred(const Event& event) {
    LOG_ERRORF("Error occurred: %s", event.message ? event.message : "Unknown error");
    // Could implement error recovery logic here
}

void App::updateWiFi() {
    static bool lastConnectedState = false;
    static bool webServerStarted = false;
    static unsigned long lastStatusPrint = 0;
    
    wifiManager->update();
    
    // Print WiFi status every 10 seconds for debugging
    if (millis() - lastStatusPrint > 10000) {
        if (wifiManager->isConnecting()) {
            LOG_INFO("[WiFi] Still connecting...");
        } else if (!wifiManager->isConnected()) {
            LOG_INFOF("[WiFi] Not connected. WiFi Status: %d", WiFi.status());
            LOG_INFOF("[WiFi] Attempting to connect to: %s", config.wifi.ssid);
        }
        lastStatusPrint = millis();
    }
    
    bool currentConnectedState = wifiManager->isConnected();
    if (currentConnectedState != lastConnectedState) {
        if (currentConnectedState) {
            LOG_INFOF("[WiFi] *** CONNECTED! *** IP: %s", wifiManager->getLocalIP().c_str());
            LOG_INFOF("[WiFi] Gateway: %s", WiFi.gatewayIP().toString().c_str());
            LOG_INFOF("[WiFi] DNS: %s", WiFi.dnsIP().toString().c_str());
            if (!webServerStarted) {
                webServer->begin();
                webServerStarted = true;
                LOG_INFOF("[Web] *** Debug server available at: http://%s ***", wifiManager->getLocalIP().c_str());
            }
        } else {
            LOG_WARN("[WiFi] *** DISCONNECTED ***");
        }
        lastConnectedState = currentConnectedState;
    }
}

void App::updateMQTT() {
    static bool lastMqttConnectedState = false;
    static unsigned long lastConnectionAttempt = 0;
    static unsigned long lastStatusPrint = 0;
    
    if (wifiManager->isConnected()) {
        bool currentMqttState = mqttClient->isConnected();
        
        // Print MQTT status every 15 seconds for debugging
        if (millis() - lastStatusPrint > 15000) {
            if (!currentMqttState) {
                LOG_INFOF("[MQTT] Not connected. Broker: %s:%d", config.mqtt.broker, config.mqtt.port);
                LOG_INFOF("[MQTT] EdgeId: %s, Username: %s", config.mqtt.edgeId, config.mqtt.username);
            }
            lastStatusPrint = millis();
        }
        
        if (!currentMqttState && millis() - lastConnectionAttempt > 5000) {
            LOG_INFO("[MQTT] Attempting to connect...");
            ErrorCode result = mqttClient->connect();
            if (result != ErrorCode::SUCCESS) {
                LOG_ERRORF("[MQTT] Connection attempt failed with error: %d", (int)result);
            }
            lastConnectionAttempt = millis();
        }
        
        if (currentMqttState != lastMqttConnectedState) {
            if (currentMqttState) {
                LOG_INFO("[MQTT] *** CONNECTED SUCCESSFULLY! ***");
            } else {
                LOG_WARN("[MQTT] *** DISCONNECTED ***");
            }
            lastMqttConnectedState = currentMqttState;
        }
        
        mqttClient->update();
    } else {
        if (lastMqttConnectedState) {
            LOG_WARN("[MQTT] WiFi disconnected, stopping MQTT");
            lastMqttConnectedState = false;
        }
    }
}

void App::onLedControlMessage(bool ledOn) {
    LOG_INFOF("*** MANUAL LED CONTROL from Home Assistant: %s ***", ledOn ? "ON" : "OFF");
    this->manualLedControl = true;  // Enable manual control mode
    
    if (ledOn) {
        // Turn on indefinitely when manually controlled
        ledController->turnOn();
        ledOnTime = millis();
        ledTimerActive = true;
        // Set a very long duration for manual control
        eventBus.publish(Event(EventType::LED_STATUS_CHANGED, true));
    } else {
        ledController->turnOff();
        ledTimerActive = false;
        eventBus.publish(Event(EventType::LED_STATUS_CHANGED, false));
        // When manually turned off, go back to automatic mode
        this->manualLedControl = false;
        LOG_INFO("*** Returning to automatic light sensor control ***");
    }
}

void App::setupWebServer() {
    webServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", getStatusHTML());
    });
    
    webServer->on("/status", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", getStatusHTML());
    });
    
    LOG_INFO("Web server configured (will start when WiFi connects)");
}

String App::getStatusHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>ESP32 Debug Status</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
    html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += ".status{padding:10px;margin:10px 0;border-radius:5px}";
    html += ".success{background:#d4edda;border:1px solid #c3e6cb;color:#155724}";
    html += ".warning{background:#fff3cd;border:1px solid #ffeaa7;color:#856404}";
    html += ".error{background:#f8d7da;border:1px solid #f5c6cb;color:#721c24}";
    html += ".info{background:#d1ecf1;border:1px solid #bee5eb;color:#0c5460}";
    html += "h1{color:#333;text-align:center}";
    html += "h2{color:#666;border-bottom:2px solid #eee;padding-bottom:5px}";
    html += ".refresh{text-align:center;margin:20px 0}";
    html += ".btn{background:#007bff;color:white;padding:10px 20px;text-decoration:none;border-radius:5px;display:inline-block}";
    html += "</style>";
    html += "<script>setTimeout(function(){location.reload()},5000)</script>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🔧 ESP32 Debug Status</h1>";
    
    // System Info
    html += "<h2>📊 System Information</h2>";
    html += "<div class='status info'>";
    html += "<strong>MAC Address:</strong> " + WiFi.macAddress() + "<br>";
    html += "<strong>Uptime:</strong> " + String(millis() / 1000) + " seconds<br>";
    html += "<strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes<br>";
    html += "<strong>Firmware:</strong> ESP32 Environmental Monitor (Refactored)<br>";
    html += "</div>";
    
    // WiFi Status
    html += "<h2>📶 WiFi Status</h2>";
    if (wifiManager->isConnected()) {
        html += "<div class='status success'>";
        html += "<strong>Status:</strong> ✅ Connected<br>";
        html += "<strong>SSID:</strong> " + WiFi.SSID() + "<br>";
        html += "<strong>IP Address:</strong> " + wifiManager->getLocalIP() + "<br>";
        html += "<strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm<br>";
    } else if (wifiManager->isConnecting()) {
        html += "<div class='status warning'>";
        html += "<strong>Status:</strong> ⏳ Connecting...";
    } else {
        html += "<div class='status error'>";
        html += "<strong>Status:</strong> ❌ Disconnected<br>";
        html += "<strong>Target SSID:</strong> " + String(config.wifi.ssid);
    }
    html += "</div>";
    
    // MQTT Status
    html += "<h2>📡 MQTT Status</h2>";
    if (mqttClient->isConnected()) {
        html += "<div class='status success'>";
        html += "<strong>Status:</strong> ✅ Connected<br>";
        html += "<strong>Broker:</strong> " + String(config.mqtt.broker) + ":" + String(config.mqtt.port) + "<br>";
        html += "<strong>Client ID:</strong> " + String(config.mqtt.edgeId) + "<br>";
        html += "<strong>Username:</strong> " + String(config.mqtt.username);
    } else {
        html += "<div class='status error'>";
        html += "<strong>Status:</strong> ❌ Disconnected<br>";
        html += "<strong>Target Broker:</strong> " + String(config.mqtt.broker) + ":" + String(config.mqtt.port) + "<br>";
        html += "<strong>Client ID:</strong> " + String(config.mqtt.edgeId) + "<br>";
        html += "<strong>Username:</strong> " + String(config.mqtt.username);
    }
    html += "</div>";
    
    // Sensor Data
    html += "<h2>🌡️ Sensor Data</h2>";
    auto sensorResult = sensor->read();
    if (sensorResult.isSuccess()) {
        SensorData data = sensorResult.value;
        html += "<div class='status success'>";
        html += "<strong>Temperature:</strong> " + String(data.temperture, 1) + "°C<br>";
        html += "<strong>Humidity:</strong> " + String(data.humidity, 1) + "%<br>";
        html += "<strong>Light Level:</strong> " + String(data.photoresisterValue) + "<br>";
        html += "<strong>LED State:</strong> " + (data.ledOn ? String("🟢 ON") : String("🔴 OFF")) + "<br>";
        html += "<strong>Manual Control:</strong> " + (manualLedControl ? String("✋ Active") : String("🤖 Auto"));
    } else {
        html += "<div class='status error'>";
        html += "<strong>Status:</strong> ❌ Sensor Read Failed";
    }
    html += "</div>";
    
    // MQTT Topics
    html += "<h2>📋 MQTT Topics</h2>";
    html += "<div class='status info'>";
    html += "<strong>Data Topic:</strong> Advantech/" + String(config.mqtt.edgeId) + "/data<br>";
    html += "<strong>LED Control:</strong> Advantech/" + String(config.mqtt.edgeId) + "/led<br>";
    html += "<strong>Discovery:</strong> homeassistant/sensor/" + String(config.mqtt.edgeId) + "/*/config";
    html += "</div>";
    
    html += "<div class='refresh'>";
    html += "<a href='/' class='btn'>🔄 Refresh Now</a>";
    html += "</div>";
    
    html += "<p style='text-align:center;color:#666;font-size:12px'>Auto-refresh every 5 seconds</p>";
    html += "</div>";
    html += "</body></html>";
    
    return html;
}