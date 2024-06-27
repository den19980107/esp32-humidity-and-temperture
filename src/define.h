#ifndef DEFINE_H
#define DEFINE_H

#define SENSOR_READING_INTERVAL 1000

#define DHT_PIN 13
#define DHT_TYPE DHT11
#define PHOTORESISTER_PIN 39

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define PHOTORESISTER_THRESHOLD 900

#define LED_PIN 2
#define NIGHT_LIGHT_ON_DURATION 10 * 60 * 1000
#define NIGHT_LIGHT_BLOCK_DISPLAY_DURATION 3 * 1000

#define CHECK_WIFI_DEVICE_CONFIG_INTERVAL 10 * 1000

#define WIFI_CONNECT_TIMEOUT 30 * 1000

#define UPLOAD_FRQUENCY 5 * 1000

#define HA_CONFIG_TEMPERTURE_TOPIC "homeassistant/sensor/%s/temperture/config"
#define HA_CONFIG_HUMIDITY_TOPIC "homeassistant/sensor/%s/humidity/config"
#define HA_CONFIG_PHOTORESISTER_TOPIC "homeassistant/sensor/%s/photoresister/config"
#define HA_CONFIG_LED_TOPIC "homeassistant/light/%s/led/config"

#endif
