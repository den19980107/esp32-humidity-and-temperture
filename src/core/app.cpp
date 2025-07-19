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
        
        // Validate loaded configuration - if key values are empty, use defaults (but keep user's WiFi settings)
        if (strlen(config.mqtt.broker) == 0) {
            LOG_WARN("MQTT broker not configured, applying MQTT defaults");
            // Only reset MQTT settings, preserve WiFi settings
            strcpy(config.mqtt.broker, "192.168.31.21");
            strcpy(config.mqtt.username, "user");
            strcpy(config.mqtt.password, "passwd");
            strcpy(config.mqtt.edgeId, "24dcc3a736ec");
            config.mqtt.port = 1883;
            config.saveToFile(CONFIG_FILE);
            LOG_INFOF("Applied MQTT defaults - WiFi SSID preserved: '%s'", config.wifi.ssid);
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
            if (strlen(config.wifi.ssid) == 0) {
                LOG_INFO("[WiFi] No credentials configured - AP mode active");
            } else {
                LOG_INFOF("[WiFi] Not connected. WiFi Status: %d", WiFi.status());
                LOG_INFOF("[WiFi] Attempting to connect to: %s", config.wifi.ssid);
            }
        }
        lastStatusPrint = millis();
    }
    
    bool currentConnectedState = wifiManager->isConnected();
    if (currentConnectedState != lastConnectedState) {
        if (currentConnectedState) {
            if (wifiManager->isInAPMode()) {
                LOG_INFOF("[AP Mode] *** ACCESS POINT ACTIVE *** IP: %s", wifiManager->getLocalIP().c_str());
                LOG_INFO("[AP Mode] Connect to configure WiFi credentials");
            } else {
                LOG_INFOF("[WiFi] *** CONNECTED! *** IP: %s", wifiManager->getLocalIP().c_str());
                LOG_INFOF("[WiFi] Gateway: %s", WiFi.gatewayIP().toString().c_str());
                LOG_INFOF("[WiFi] DNS: %s", WiFi.dnsIP().toString().c_str());
            }
            
            if (!webServerStarted) {
                webServer->begin();
                webServerStarted = true;
                if (wifiManager->isInAPMode()) {
                    LOG_INFOF("[Web] *** WiFi Config server at: http://%s ***", wifiManager->getLocalIP().c_str());
                } else {
                    LOG_INFOF("[Web] *** Debug server available at: http://%s ***", wifiManager->getLocalIP().c_str());
                }
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
    
    // Only attempt MQTT if WiFi is connected to a network (not in AP mode)
    if (wifiManager->isConnected() && !wifiManager->isInAPMode()) {
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
            if (wifiManager->isInAPMode()) {
                LOG_INFO("[MQTT] WiFi in AP mode, MQTT disabled");
            } else {
                LOG_WARN("[MQTT] WiFi disconnected, stopping MQTT");
            }
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
    // Main page - either WiFi config or status depending on mode
    webServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (wifiManager->isInAPMode()) {
            request->send(200, "text/html", getWiFiConfigHTML());
        } else {
            request->send(200, "text/html", getStatusHTML());
        }
    });
    
    // Status page (always available)
    webServer->on("/status", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", getStatusHTML());
    });
    
    // WiFi configuration submission
    webServer->on("/configure", HTTP_POST, [this](AsyncWebServerRequest *request){
        handleWiFiConfig(request);
    });
    
    // WiFi scan endpoint
    webServer->on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "application/json", scanWiFiNetworks());
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
    if (wifiManager->isInAPMode()) {
        html += "<div class='status info'>";
        html += "<strong>Status:</strong> 📡 Access Point Mode<br>";
        html += "<strong>AP Name:</strong> ESP32-Config-" + WiFi.macAddress().substring(12, 17) + "<br>";
        html += "<strong>AP IP:</strong> " + wifiManager->getLocalIP() + "<br>";
        html += "<strong>Connected Clients:</strong> " + String(WiFi.softAPgetStationNum()) + "<br>";
        html += "<strong>Mode:</strong> Configuration Mode (No WiFi credentials set)";
    } else if (wifiManager->isConnected()) {
        html += "<div class='status success'>";
        html += "<strong>Status:</strong> ✅ Connected to WiFi<br>";
        html += "<strong>SSID:</strong> " + WiFi.SSID() + "<br>";
        html += "<strong>IP Address:</strong> " + wifiManager->getLocalIP() + "<br>";
        html += "<strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm<br>";
        html += "<strong>Gateway:</strong> " + WiFi.gatewayIP().toString();
    } else if (wifiManager->isConnecting()) {
        html += "<div class='status warning'>";
        html += "<strong>Status:</strong> ⏳ Connecting to WiFi...<br>";
        html += "<strong>Target SSID:</strong> " + String(config.wifi.ssid);
    } else {
        html += "<div class='status error'>";
        html += "<strong>Status:</strong> ❌ Disconnected<br>";
        html += "<strong>Target SSID:</strong> " + String(config.wifi.ssid);
    }
    html += "</div>";
    
    // MQTT Status
    html += "<h2>📡 MQTT Status</h2>";
    if (wifiManager->isInAPMode()) {
        html += "<div class='status warning'>";
        html += "<strong>Status:</strong> ⚠️ Not Available (AP Mode)<br>";
        html += "<strong>Info:</strong> MQTT requires WiFi connection<br>";
        html += "<strong>Configure WiFi first to enable MQTT</strong>";
    } else if (mqttClient->isConnected()) {
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
    if (wifiManager->isInAPMode()) {
        html += "<a href='/' class='btn' style='background:#28a745;margin-right:10px'>📶 WiFi Configuration</a>";
        html += "<a href='/status' class='btn'>🔄 Refresh Status</a>";
    } else {
        html += "<a href='/status' class='btn'>🔄 Refresh Now</a>";
    }
    html += "</div>";
    
    html += "<p style='text-align:center;color:#666;font-size:12px'>Auto-refresh every 5 seconds</p>";
    html += "</div>";
    html += "</body></html>";
    
    return html;
}

String App::getWiFiConfigHTML() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>ESP32 WiFi Configuration</title>
    <style>
        body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}
        .container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
        h1{color:#333;text-align:center;margin-bottom:30px}
        .form-group{margin-bottom:15px}
        label{display:block;margin-bottom:5px;font-weight:bold;color:#555}
        input,select{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}
        button{width:100%;padding:12px;background:#007bff;color:white;border:none;border-radius:5px;cursor:pointer;font-size:16px}
        button:hover{background:#0056b3}
        .scan-btn{background:#28a745;margin-bottom:10px}
        .scan-btn:hover{background:#1e7e34}
        .status{text-align:center;margin:10px 0;padding:10px;border-radius:5px}
        .success{background:#d4edda;border:1px solid #c3e6cb;color:#155724}
        .info{background:#d1ecf1;border:1px solid #bee5eb;color:#0c5460}
    </style>
</head>
<body>
    <div class='container'>
        <h1>📶 WiFi Configuration</h1>
        
        <div class='status info'>
            <strong>ESP32 Access Point Active</strong><br>
            Configure WiFi credentials to connect to your network
        </div>
        
        <form action='/configure' method='POST'>
            <div class='form-group'>
                <button type='button' class='scan-btn' onclick='scanNetworks()'>🔍 Scan WiFi Networks</button>
                <select id='ssid' name='ssid' onchange='updateSSID()'>
                    <option value=''>Select a network or enter manually</option>
                </select>
            </div>
            
            <div class='form-group'>
                <label for='ssid_manual'>WiFi Network (SSID):</label>
                <input type='text' id='ssid_manual' name='ssid_manual' placeholder='Enter WiFi network name'>
            </div>
            
            <div class='form-group'>
                <label for='password'>WiFi Password:</label>
                <input type='password' id='password' name='password' placeholder='Enter WiFi password'>
            </div>
            
            <button type='submit'>💾 Save and Connect</button>
        </form>
        
        <div style='text-align:center;margin-top:20px'>
            <a href='/status' style='color:#007bff;text-decoration:none'>📊 View System Status</a>
        </div>
    </div>
    
    <script>
        function scanNetworks() {
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    const select = document.getElementById('ssid');
                    select.innerHTML = '<option value="">Select a network</option>';
                    data.networks.forEach(network => {
                        const option = document.createElement('option');
                        option.value = network.ssid;
                        option.textContent = network.ssid + ' (' + network.rssi + ' dBm)';
                        select.appendChild(option);
                    });
                })
                .catch(err => console.error('Scan failed:', err));
        }
        
        function updateSSID() {
            const select = document.getElementById('ssid');
            const manual = document.getElementById('ssid_manual');
            if (select.value) {
                manual.value = select.value;
            }
        }
        
        // Auto-scan on page load
        window.onload = function() {
            scanNetworks();
        }
    </script>
</body>
</html>
)";
    
    return html;
}

String App::scanWiFiNetworks() {
    String json = "{\"networks\":[";
    
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }
    
    json += "]}";
    WiFi.scanDelete(); // Free memory
    
    return json;
}

void App::handleWiFiConfig(AsyncWebServerRequest *request) {
    String ssid = "";
    String password = "";
    
    // Get SSID from manual input or dropdown
    if (request->hasParam("ssid_manual", true) && request->getParam("ssid_manual", true)->value().length() > 0) {
        ssid = request->getParam("ssid_manual", true)->value();
    } else if (request->hasParam("ssid", true)) {
        ssid = request->getParam("ssid", true)->value();
    }
    
    if (request->hasParam("password", true)) {
        password = request->getParam("password", true)->value();
    }
    
    if (ssid.length() > 0) {
        // Update configuration
        strncpy(config.wifi.ssid, ssid.c_str(), sizeof(config.wifi.ssid) - 1);
        strncpy(config.wifi.password, password.c_str(), sizeof(config.wifi.password) - 1);
        config.wifi.ssid[sizeof(config.wifi.ssid) - 1] = '\0';
        config.wifi.password[sizeof(config.wifi.password) - 1] = '\0';
        
        // Save to file
        ErrorCode saveResult = config.saveToFile(CONFIG_FILE);
        
        if (saveResult == ErrorCode::SUCCESS) {
            LOG_INFOF("[WiFi Config] New credentials saved successfully - SSID: %s", ssid.c_str());
            
            // Force SPIFFS to flush to ensure file is written
            SPIFFS.end();
            SPIFFS.begin(true);
            
            // Send success response
            String response = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>WiFi Configuration Saved</title>
    <style>
        body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;text-align:center}
        .container{max-width:400px;margin:50px auto;background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
        .success{background:#d4edda;border:1px solid #c3e6cb;color:#155724;padding:15px;border-radius:5px;margin:20px 0}
    </style>
    <script>
        setTimeout(function() {
            window.location.href = '/status';
        }, 5000);
    </script>
</head>
<body>
    <div class='container'>
        <h1>✅ Configuration Saved!</h1>
        <div class='success'>
            WiFi credentials have been saved.<br>
            The ESP32 will restart and attempt to connect to: <strong>)" + ssid + R"(</strong>
        </div>
        <p>Restarting in 5 seconds...</p>
    </div>
</body>
</html>
)";
            
            request->send(200, "text/html", response);
            
            // Wait longer to ensure web response is sent and file is written
            delay(3000);
            LOG_INFO("[WiFi Config] Restarting ESP32 to apply new WiFi configuration...");
            ESP.restart();
        } else {
            LOG_ERRORF("[WiFi Config] Failed to save configuration, error: %d", (int)saveResult);
            request->send(500, "text/html", "<h1>Error: Failed to save configuration</h1>");
        }
    } else {
        request->send(400, "text/html", "<h1>Error: SSID is required</h1>");
    }
}