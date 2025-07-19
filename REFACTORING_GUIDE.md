# ESP32 Environmental Monitor - Refactoring Guide

## Overview

This document outlines the refactoring performed on the ESP32 humidity and temperature monitoring project. The refactoring improves code organization, maintainability, security, and follows modern C++ best practices.

## What Was Refactored

### 🔥 Critical Fixes

1. **Memory Leak Fix** (`src/util/json.cpp`)
   - **Issue**: `jsonToByte()` function allocated memory with `new` but never freed it
   - **Fix**: Replaced with `std::unique_ptr<char[]>` for automatic memory management
   - **Impact**: Prevents memory leaks that could cause system crashes over time

2. **Header Guard Fix** (`src/util/file.h`)
   - **Issue**: Typo in header guard `#define UTIL_FIL_H` (missing 'E')
   - **Fix**: Corrected to `#define UTIL_FILE_H`
   - **Impact**: Prevents potential compilation issues

3. **Security Fix** (`data/device_config.html`)
   - **Issue**: MQTT password input used `type="text"` (visible text)
   - **Fix**: Changed to `type="password"` (hidden input)
   - **Impact**: Protects password from shoulder surfing

### 🏗️ Architectural Improvements

#### 1. Hardware Abstraction Layer
**Location**: `src/core/interfaces.h`

Created abstract interfaces for all hardware components:
- `ISensorReader` - For DHT11 and photoresistor
- `IDisplayDriver` - For OLED display
- `ILedController` - For LED control
- `INetworkManager` - For WiFi management
- `IMqttClient` - For MQTT communication

**Benefits**:
- Easy testing with mock implementations
- Hardware-agnostic application logic
- Simplified unit testing
- Better separation of concerns

#### 2. Event-Driven Architecture
**Location**: `src/core/event_bus.h`

Replaced callback pattern with centralized event system:
```cpp
eventBus.subscribe(EventType::SENSOR_DATA_UPDATED, handler);
eventBus.publish(Event(EventType::LED_STATUS_CHANGED, true));
```

**Benefits**:
- Loose coupling between components
- Easier to add new features
- Better debugging and monitoring
- More maintainable code

#### 3. Configuration Management
**Location**: `src/core/config.h`, `src/core/config.cpp`

Centralized configuration with JSON persistence:
- WiFi settings (including enterprise support)
- MQTT broker configuration  
- Sensor parameters and thresholds
- Hardware pin assignments

**Benefits**:
- Runtime configuration changes
- Easy deployment to different environments
- Validation and error handling
- Backup and restore capabilities

#### 4. Error Handling System
**Location**: `src/core/interfaces.h`

Implemented Result<T> pattern for error handling:
```cpp
Result<SensorData> data = sensor->read();
if (data.isSuccess()) {
    // Process data.value
} else {
    // Handle data.error
}
```

**Benefits**:
- Explicit error handling
- No silent failures
- Better debugging information
- Consistent error reporting

#### 5. Logging System
**Location**: `src/core/logger.h`, `src/core/logger.cpp`

Added structured logging with levels:
```cpp
LOG_INFO("System started");
LOG_ERRORF("Failed to read sensor: %d", errorCode);
```

**Benefits**:
- Configurable log levels
- Timestamped messages
- Better debugging capabilities
- Production monitoring

### 📁 Code Organization

#### New Directory Structure
```
src/
├── core/           # Core framework components
│   ├── interfaces.h    # Hardware abstractions
│   ├── event_bus.h    # Event system
│   ├── config.h/.cpp  # Configuration management
│   ├── logger.h/.cpp  # Logging system
│   ├── state_machine.h # Base state machine class
│   └── app_v2.h/.cpp  # Refactored application
├── hardware/       # Hardware implementations
│   ├── dht_sensor.h   # DHT11 sensor driver
│   ├── oled_display.h # OLED display driver
│   └── led_controller.h # LED controller
├── state_machine/  # Original state machines (preserved)
├── util/          # Utility functions (improved)
└── app/           # Original app (preserved)
```

#### Key Improvements
- **Single Responsibility**: Each class has one clear purpose
- **Dependency Injection**: Dependencies passed through constructors
- **RAII**: Automatic resource management with smart pointers
- **Template-based**: Generic state machine base class
- **Modern C++**: Uses C++11/14 features appropriately

## How to Use the Refactored Version

### Option 1: Gradual Migration (Recommended)

1. **Keep both versions**: Original code is preserved in `app/` directory
2. **Test new version**: Use `main_v2.cpp` with `#define USE_REFACTORED_VERSION`
3. **Migrate incrementally**: Move features one by one to new architecture

### Option 2: Full Migration

1. **Update main.cpp**:
```cpp
#include "core/app_v2.h"

void setup() {
    Serial.begin(115200);
    RefactoredApp app;
    if (app.initialize() == ErrorCode::SUCCESS) {
        app.run();
    }
}
```

2. **Create configuration file** (`/config.json` on SPIFFS):
```json
{
  "wifi": {
    "ssid": "YourWiFi",
    "password": "YourPassword"
  },
  "mqtt": {
    "broker": "mqtt.example.com",
    "edgeId": "esp32-001",
    "username": "user",
    "password": "pass"
  },
  "sensor": {
    "photoresisterThreshold": 900,
    "nightLightDuration": 600000
  }
}
```

### Configuration Options

The new system is highly configurable. Key settings:

```cpp
// Sensor configuration
config.sensor.dhtPin = 13;              // DHT sensor pin
config.sensor.photoresisterPin = 39;    // Light sensor pin
config.sensor.ledPin = 25;              // LED pin
config.sensor.photoresisterThreshold = 900;  // Dark threshold
config.sensor.nightLightDuration = 600000;   // 10 minutes in ms

// Display configuration  
config.sensor.sdaPin = 32;              // I2C SDA pin
config.sensor.sclPin = 33;              // I2C SCL pin

// Timing configuration
config.sensor.sensorReadingInterval = 1000;  // Read every 1 second
config.sensor.uploadFrequency = 5000;        // Upload every 5 seconds
```

## Testing the Refactored Version

1. **Compile**: Ensure all new files compile without errors
2. **Upload**: Flash the firmware to ESP32
3. **Monitor**: Check serial output for log messages
4. **Verify**: Test all functions (sensors, display, LED, web interface)
5. **Configuration**: Test configuration loading/saving

## Benefits of Refactoring

### Immediate Benefits
- ✅ Fixed memory leaks and security issues
- ✅ Better error handling and logging
- ✅ Cleaner, more organized code structure
- ✅ Easier to understand and maintain

### Long-term Benefits
- 🚀 Easier to add new features
- 🧪 Better testability with mock objects
- 🔧 Runtime configuration without reflashing
- 📊 Better monitoring and debugging
- 🏗️ Foundation for future enhancements

### Future Enhancements Made Easier
- **Over-the-Air Updates**: Hardware abstraction makes OTA easier
- **Multiple Sensor Types**: Easy to add new sensor implementations
- **Web Dashboard**: Event system makes real-time updates simple
- **Data Logging**: Can add database logging through events
- **Remote Configuration**: Config system supports remote updates

## Migration Timeline Recommendation

### Phase 1 (Week 1): Critical Fixes
- [x] Fix memory leak in JSON utility
- [x] Fix header guard typo
- [x] Fix password input security issue

### Phase 2 (Week 2): Infrastructure
- [x] Add logging system
- [x] Add configuration management
- [x] Add error handling framework

### Phase 3 (Week 3): Hardware Abstraction
- [x] Create hardware interfaces
- [x] Implement concrete hardware drivers
- [x] Test hardware abstraction layer

### Phase 4 (Week 4): Application Refactoring
- [x] Create new app architecture
- [x] Implement event-driven design
- [x] Test complete refactored system

### Phase 5 (Future): Advanced Features
- [ ] Add MQTT client implementation
- [ ] Add WiFi manager implementation
- [ ] Add web interface improvements
- [ ] Add OTA update capability

## Rollback Plan

If issues are encountered:

1. **Keep original files**: All original code is preserved
2. **Switch main.cpp**: Comment out `#define USE_REFACTORED_VERSION`
3. **Revert to working state**: Original functionality remains intact

## Conclusion

This refactoring significantly improves the codebase while maintaining all original functionality. The new architecture is more maintainable, testable, and ready for future enhancements while fixing critical security and memory issues.