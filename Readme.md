# ESP32 Environmental Monitor

A comprehensive IoT project for environmental monitoring with Home Assistant integration, featuring an event-driven architecture and web-based configuration interface.

## 🌟 Features

### Core Functionality
1. **Environmental Monitoring**: DHT11 sensor for temperature and humidity detection
2. **Smart Night Light**: Automatic LED control based on ambient light levels with photoresistor
3. **OLED Display**: Real-time sensor data visualization with status indicators
4. **Cloud Integration**: MQTT-based data upload with Home Assistant auto-discovery
5. **Remote Control**: LED control via Home Assistant or MQTT commands
6. **Web Configuration**: User-friendly WiFi setup interface with network scanning

### Advanced Features
- **Automatic Fallback**: Access Point mode when WiFi credentials are not configured
- **Persistent Configuration**: SPIFFS-based storage for WiFi and device settings  
- **Event-Driven Architecture**: Modular design with EventBus pattern
- **Hardware Abstraction**: Clean interfaces for sensors, display, and connectivity
- **Real-time Monitoring**: Web-based status dashboard with live updates
- **Error Handling**: Comprehensive logging and graceful error recovery

## 🎥 Demo

https://www.youtube.com/shorts/sdgEsP3yd80

## 🔧 Hardware

### Project Photos

**Front View:**
![Front View](https://i.imgur.com/OM2pc4W.jpeg)

**Back View:**
![Back View](https://i.imgur.com/whOgIty.jpeg)

### Component List
- **ESP32 Development Board** (ESP32-DOIT-DEVKIT-V1)
- **DHT11** - Temperature and humidity sensor
- **SSD1306 OLED Display** - 128x64 I2C display
- **Photoresistor** - Light intensity detection
- **LED** - Night light and status indicator
- **Resistors** - Pull-up and current limiting

### Pin Configuration
```cpp
// Sensor Pins
DHT11_PIN = 13        // Temperature/Humidity sensor
PHOTORESISTOR_PIN = 39 // Light sensor (analog)
LED_PIN = 25          // Night light LED

// I2C Display
SDA_PIN = 32          // I2C Data
SCL_PIN = 33          // I2C Clock
```

### Schematic Diagram
![Schematic](https://i.imgur.com/LqW8kUU.png)

### PCB Layout
![PCB Layout](https://i.imgur.com/0F4Rr59.png)

## 💾 Software Architecture

### Event-Driven Design
The system is built using a modern event-driven architecture replacing the original state machine approach:

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Sensor Layer  │    │  Hardware Layer │    │    Core App     │
│                 │    │                 │    │                 │
│ • DHTSensor     │───▶│ • WiFiManager   │───▶│ • EventBus      │
│ • Photoresistor │    │ • MQTTClient    │    │ • Config        │
│                 │    │ • LEDController │    │ • Logger        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
┌─────────────────┐             │              ┌─────────────────┐
│ Display Layer   │             │              │   Web Server    │
│                 │             │              │                 │
│ • OLEDDisplay   │◀────────────┼──────────────│ • Status Page   │
│ • Status Info   │             │              │ • WiFi Config   │
│                 │             │              │ • Network Scan  │
└─────────────────┘             ▼              └─────────────────┘
                    ┌─────────────────┐
                    │ Home Assistant  │
                    │                 │
                    │ • Auto Discovery│
                    │ • MQTT Topics   │
                    │ • LED Control   │
                    └─────────────────┘
```

### Key Components

#### Core Classes
- **`App`**: Main application orchestrator with event handling
- **`EventBus`**: Publish/subscribe system for component communication
- **`Config`**: Configuration management with SPIFFS persistence

#### Hardware Abstractions
- **`ISensorReader`**: Interface for sensor data reading
- **`IDisplayDriver`**: Interface for display operations  
- **`ILedController`**: Interface for LED control
- **`WiFiManager`**: WiFi connection and Access Point management
- **`MQTTClient`**: MQTT communication with Home Assistant integration

## 🚀 Getting Started

### Prerequisites
- **PlatformIO CLI** - Development environment
- **ESP32 Development Board** - Target hardware
- **USB Cable** - For programming and power

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd "esp32 humidity and temperture"
   ```

2. **Install dependencies**
   ```bash
   pio run -t compiledb
   ```

3. **Build and upload**
   ```bash
   pio run -e upesy_wroom -t upload
   ```

4. **Monitor serial output**
   ```bash
   pio monitor -e upesy_wroom
   ```

### Initial Setup

1. **Power on the ESP32** - It will start in Access Point mode
2. **Connect to WiFi** - Look for "ESP32-Config-36EC" network  
3. **Open configuration page** - Navigate to http://192.168.4.1
4. **Configure WiFi** - Select your network and enter password
5. **Automatic restart** - ESP32 will connect to your WiFi network
6. **Access status page** - Monitor at the assigned IP address

## 📱 Web Interface

### WiFi Configuration
![WiFi Config](https://i.imgur.com/BxlIvDF.jpeg)

**Features:**
- Network scanning and selection
- Manual SSID entry support
- Password input with validation
- Automatic restart after configuration
- Responsive design for mobile devices

### Status Dashboard
- Real-time sensor readings
- WiFi connection status
- MQTT connection health
- System information (uptime, memory)
- LED control status
- Home Assistant integration status

## 🏠 Home Assistant Integration

### Automatic Discovery
The device automatically registers with Home Assistant using MQTT Discovery:

```yaml
# Automatically created entities:
sensor.esp32_temperature
sensor.esp32_humidity  
sensor.esp32_light_level
sensor.esp32_memory_free
sensor.esp32_memory_total
light.esp32_led
```

### MQTT Topics
```
# Data Publishing
Advantech/24dcc3a736ec/data

# LED Control  
Advantech/24dcc3a736ec/led

# Discovery Topics
homeassistant/sensor/24dcc3a736ec/temperature/config
homeassistant/sensor/24dcc3a736ec/humidity/config
homeassistant/light/24dcc3a736ec/led/config
```

## ⚙ Configuration

### Default Settings
```json
{
  "wifi": {
    "ssid": "",
    "password": ""
  },
  "mqtt": {
    "broker": "192.168.31.21",
    "port": 1883,
    "username": "user",
    "password": "passwd",
    "edgeId": "24dcc3a736ec"
  },
  "sensor": {
    "dhtPin": 13,
    "photoresisterPin": 39,
    "ledPin": 25,
    "photoresisterThreshold": 800,
    "uploadFrequency": 5000,
    "nightLightDuration": 600000
  }
}
```

### Customization
- Modify `src/core/config.cpp` for default values
- Use web interface for WiFi configuration
- MQTT settings configurable via code (future: web interface)

## 🔧 Development

### Project Structure
```
src/
├── core/
│   ├── app.cpp              # Main application logic
│   ├── config.cpp           # Configuration management
│   ├── event_bus.cpp        # Event system
│   ├── interfaces.h         # Hardware abstractions
│   └── logger.cpp           # Logging system
├── hardware/
│   ├── dht_sensor.cpp       # DHT11 sensor driver
│   ├── oled_display.cpp     # SSD1306 display driver
│   ├── led_controller.cpp   # LED control
│   ├── wifi_manager.h       # WiFi management
│   └── mqtt_client.h        # MQTT communication
└── main.cpp                 # Entry point
```

### Build Commands
```bash
# Build only
pio run -e upesy_wroom

# Upload to device  
pio run -e upesy_wroom -t upload

# Monitor serial output
pio monitor -e upesy_wroom

# Clean build
pio run -e upesy_wroom -t clean

# Erase flash (reset WiFi config)
pio run -e upesy_wroom -t erase
```

## 🐛 Troubleshooting

### WiFi Issues
- **Can't find ESP32 network**: Wait 30 seconds after power-on for AP mode
- **Configuration not saving**: Check serial logs for SPIFFS errors
- **Won't connect to WiFi**: Verify password and network availability

### MQTT Issues  
- **Not connecting**: Check broker IP and credentials in config
- **Home Assistant not discovering**: Verify MQTT broker is running
- **LED control not working**: Check MQTT topic subscription

### Hardware Issues
- **No sensor readings**: Check DHT11 wiring and pin configuration
- **Display not working**: Verify I2C connections (SDA/SCL pins)
- **LED not responding**: Check LED pin and current limiting resistor

## 📈 Future Enhancements

- [ ] Web-based MQTT configuration
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Additional sensor support (BME280, etc.)
- [ ] Data logging to SD card
- [ ] Battery power optimization
- [ ] Custom dashboard themes
- [ ] Multi-language support

## 📄 License

This project is open source and available under the MIT License.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

---

*This project serves as a comprehensive example of modern ESP32 development with clean architecture, web interfaces, and IoT integration.*