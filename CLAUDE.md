# CLAUDE.md - Development History & Context

This file documents the major refactoring and development work done on the ESP32 Environmental Monitor project, providing context for future development sessions.

## 🏗️ Major Refactoring Session

**Date:** July 2025  
**Scope:** Complete architectural overhaul from state machines to event-driven design

### 🎯 Primary Objectives Completed

1. **Architectural Transformation**
   - ✅ Replaced finite state machine pattern with event-driven architecture
   - ✅ Implemented EventBus pattern for component communication
   - ✅ Created hardware abstraction interfaces (ISensorReader, IDisplayDriver, ILedController)
   - ✅ Modular design with clean separation of concerns

2. **WiFi Configuration System**
   - ✅ Web-based WiFi configuration interface with network scanning
   - ✅ Access Point (AP) mode fallback when no WiFi credentials configured
   - ✅ Persistent configuration storage using SPIFFS
   - ✅ Automatic restart and WiFi connection after configuration
   - ✅ Responsive web UI accessible at http://192.168.4.1 in AP mode

3. **Home Assistant Integration**
   - ✅ MQTT auto-discovery for seamless Home Assistant integration
   - ✅ LED control via Home Assistant dashboard
   - ✅ Real-time sensor data publishing
   - ✅ Proper entity creation (temperature, humidity, light, LED control)

4. **Bug Fixes & Improvements**
   - ✅ Fixed MQTT connection attempts in AP mode (now properly disabled)
   - ✅ Improved WiFi credential persistence with SPIFFS flush
   - ✅ Enhanced status page with mode-specific information display
   - ✅ Comprehensive error handling and logging

## 📂 Architecture Overview

### Core Components

```
src/
├── core/
│   ├── app.cpp              # Main application orchestrator
│   ├── app.h                # Application class definition
│   ├── config.cpp           # Configuration management with SPIFFS
│   ├── config.h             # Configuration structures
│   ├── event_bus.cpp        # Publish/subscribe event system
│   ├── event_bus.h          # EventBus interface
│   ├── interfaces.h         # Hardware abstraction interfaces
│   └── logger.cpp           # Structured logging system
├── hardware/
│   ├── dht_sensor.cpp       # DHT11 temperature/humidity sensor
│   ├── dht_sensor.h         # Sensor interface implementation
│   ├── oled_display.cpp     # SSD1306 OLED display driver
│   ├── oled_display.h       # Display interface implementation
│   ├── led_controller.cpp   # LED control with auto/manual modes
│   ├── led_controller.h     # LED interface implementation
│   ├── wifi_manager.h       # WiFi connection & AP mode management
│   └── mqtt_client.h        # MQTT communication & Home Assistant
└── main.cpp                 # Entry point and setup
```

### Event-Driven Flow

```
Sensor Reading → EventBus → Display Update
                    ↓
                MQTT Publish → Home Assistant
                    ↓
            LED Control ← Manual Override
```

## 🔧 Key Technical Decisions

### 1. **Configuration Management**
- **File:** `/config.json` stored in SPIFFS
- **Structure:** Separate sections for WiFi, MQTT, and sensor settings
- **Security:** No hardcoded WiFi credentials (must be configured via web UI)
- **Persistence:** SPIFFS flush ensures data is written before restart

### 2. **WiFi Strategy**
- **AP Mode:** `ESP32-Config-36EC` (MAC-based naming for uniqueness)
- **Station Mode:** Normal WiFi connection after configuration
- **Fallback:** Automatic AP mode if no credentials or connection fails
- **Reset:** SPIFFS erase (`pio run -e upesy_wroom -t erase`) clears all config

### 3. **MQTT Integration**
- **Broker:** 192.168.31.21:1883 (configurable in code)
- **Client ID:** `24dcc3a736ec` (MAC-based)
- **Topics:**
  - Data: `Advantech/24dcc3a736ec/data`
  - LED Control: `Advantech/24dcc3a736ec/led`
  - Discovery: `homeassistant/*/24dcc3a736ec/*/config`

### 4. **Hardware Pins**
```cpp
#define DHT_PIN           13  // DHT11 sensor
#define PHOTORESISTOR_PIN 39  // Light sensor (analog)
#define LED_PIN           25  // Night light LED
#define SDA_PIN           32  // I2C data (OLED)
#define SCL_PIN           33  // I2C clock (OLED)
```

## 🚀 Development Commands

### Build & Upload
```bash
# Build project
pio run -e upesy_wroom

# Upload to ESP32
pio run -e upesy_wroom -t upload

# Monitor serial output
pio monitor -e upesy_wroom

# Clean build
pio run -e upesy_wroom -t clean

# Erase flash (reset WiFi config)
pio run -e upesy_wroom -t erase
```

### Git Workflow
```bash
# Check status
git status

# Stage changes
git add .

# Commit with message
git commit -m "description"

# Push to remote
git push origin vibe-coding-refactoring
```

## 🐛 Common Issues & Solutions

### WiFi Configuration Problems

**Issue:** ESP32 not saving WiFi credentials  
**Solution:** Check SPIFFS initialization and ensure `config.saveToFile()` success  
**Debug:** Monitor serial output for configuration save errors  

**Issue:** Can't find ESP32 WiFi network  
**Solution:** Wait 30 seconds after power-on for AP mode to start  
**Network:** Look for `ESP32-Config-36EC` in WiFi list  

**Issue:** WiFi credentials saved but not connecting  
**Solution:** Verify password accuracy and network availability  
**Reset:** Use `pio run -e upesy_wroom -t erase` to clear config  

### MQTT Connection Issues

**Issue:** MQTT attempting connection in AP mode  
**Solution:** ✅ Fixed - MQTT now disabled in AP mode  
**Code:** `updateMQTT()` checks `!wifiManager->isInAPMode()`  

**Issue:** Home Assistant not discovering device  
**Solution:** Verify MQTT broker running and credentials correct  
**Topics:** Check discovery topics published on first connection  

### Development Issues

**Issue:** Upload failing with serial errors  
**Solution:** Try multiple upload attempts or check USB connection  
**Alternative:** Use `pio run -e upesy_wroom -t erase` then upload  

**Issue:** Java runtime errors in terminal  
**Solution:** Ignore - doesn't affect ESP32 functionality  
**Note:** Related to local environment, not project code  

## 📊 Web Interface Features

### WiFi Configuration Page (`http://192.168.4.1`)
- **Network Scanning:** Automatic WiFi network detection
- **Manual Entry:** Option to type SSID manually
- **Password Input:** Secure password field
- **Responsive Design:** Works on mobile devices
- **Auto-restart:** ESP32 restarts after successful configuration

### Status Dashboard
- **System Info:** MAC address, uptime, memory usage
- **WiFi Status:** Connection state, IP address, signal strength
- **MQTT Status:** Broker connection, client ID, topics
- **Sensor Data:** Real-time temperature, humidity, light level
- **LED Control:** Current state (auto/manual mode)

## 🔄 Event System Details

### EventBus Pattern
```cpp
// Publishing events
eventBus.publish(Event(EventType::SENSOR_DATA_UPDATED, sensorData));

// Subscribing to events
eventBus.subscribe(EventType::SENSOR_DATA_UPDATED, 
    [this](const Event& e) { onSensorDataUpdated(e); });
```

### Event Types
- `SENSOR_DATA_UPDATED` - New sensor readings available
- `LED_STATUS_CHANGED` - LED state changed (auto or manual)
- `ERROR_OCCURRED` - System error requiring attention

## 🏠 Home Assistant Entities

Auto-discovered entities created:
```yaml
sensor.esp32_temperature      # °C
sensor.esp32_humidity         # %
sensor.esp32_light_level      # Raw ADC value
sensor.esp32_memory_free      # Bytes
sensor.esp32_memory_total     # Bytes
light.esp32_led               # On/Off control
```

## 💡 Future Development Ideas

### High Priority
- [ ] Web-based MQTT configuration interface
- [ ] OTA (Over-The-Air) firmware updates
- [ ] WiFi reset button (physical or web-based)

### Medium Priority
- [ ] Additional sensor support (BME280, DS18B20)
- [ ] Data logging to SD card or local storage
- [ ] Custom web dashboard themes
- [ ] Multiple WiFi network support

### Low Priority
- [ ] Battery power optimization
- [ ] Multi-language web interface
- [ ] Advanced MQTT topics configuration
- [ ] Mobile app integration

## 🎨 Code Style & Patterns

### Naming Conventions
- **Classes:** PascalCase (`WiFiManager`, `EventBus`)
- **Methods:** camelCase (`isConnected()`, `publishSensorData()`)
- **Constants:** UPPER_SNAKE_CASE (`CONFIG_FILE`, `LED_PIN`)
- **Variables:** camelCase (`lastSensorRead`, `config`)

### Error Handling
- **Return codes:** `ErrorCode` enum for operation results
- **Logging:** Structured logging with levels (INFO, WARN, ERROR)
- **Graceful degradation:** System continues operating when possible

### Memory Management
- **Smart pointers:** `std::unique_ptr` for hardware components
- **Stack allocation:** Prefer stack over heap when possible
- **SPIFFS:** Proper file system management with error checking

## 📝 Testing Strategy

### Manual Testing Checklist
- [ ] Power-on starts AP mode (no WiFi config)
- [ ] Web interface accessible at 192.168.4.1
- [ ] WiFi configuration saves and connects
- [ ] MQTT connects after WiFi established
- [ ] Home Assistant discovers device automatically
- [ ] LED control works from Home Assistant
- [ ] Sensor data updates in real-time
- [ ] Status page shows correct information
- [ ] SPIFFS erase resets to AP mode

### Serial Monitor Key Indicators
```
[INFO] *** STARTING MAIN APPLICATION LOOP ***
[INFO] [AP Mode] Started access point: ESP32-Config-36EC
[INFO] [WiFi] *** CONNECTED! *** IP: 192.168.x.x
[INFO] [MQTT] *** CONNECTED SUCCESSFULLY! ***
[INFO] [Sensor] *** READ SUCCESS *** Temp: 24.5°C, Humidity: 60.2%, Light: 1500
```

## 📚 References & Resources

### Documentation
- **ESP32 Arduino Core:** https://docs.espressif.com/projects/arduino-esp32/
- **PlatformIO:** https://docs.platformio.org/
- **Home Assistant MQTT Discovery:** https://www.home-assistant.io/docs/mqtt/discovery/

### Libraries Used
- **DHT sensor library:** v1.4.6
- **Adafruit SSD1306:** v2.5.10
- **ESPAsyncWebServer:** v3.2.2
- **PubSubClient:** v2.8.0
- **ArduinoJson:** v7.1.0

### Hardware Datasheets
- **ESP32-DOIT-DEVKIT-V1:** Pin configuration and specifications
- **DHT11:** Temperature/humidity sensor specifications
- **SSD1306:** OLED display I2C communication protocol

---

## 🤖 AI Development Notes

This project was significantly refactored using Claude Code (Anthropic's CLI). Key collaboration aspects:

- **Architecture Design:** Event-driven pattern replaced state machines
- **Code Quality:** Clean interfaces, proper error handling, comprehensive logging  
- **User Experience:** Web-based configuration, responsive design, clear status indicators
- **Integration:** Seamless Home Assistant connectivity with auto-discovery
- **Documentation:** Comprehensive README and this CLAUDE.md file

**Next Developer:** Use this file as context when working with Claude Code or other AI tools. Include relevant sections when describing issues or requesting enhancements.

---

*Last updated: July 2025*  
*Project Status: Production Ready ✅*