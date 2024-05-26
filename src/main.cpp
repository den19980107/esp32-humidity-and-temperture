/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
*********/

#include "dht11.h"
#include "led.h"
#include "mqtt.h"
#include "oled.h"
#include "photoresister.h"
#include "server.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>

uint32_t delayMS;
char *edge_id = "<YOUR EDGE ID>";
char *mqtt_host = "<YOUR MQTT HOST>";
char *mqtt_username = "<YOUR MQTT USER NAME>";
char *mqtt_password = "<YOUR MQTT PASSWORD";

void setup() {
  Serial.begin(115200);

  Serial.println("init led...");
  led_init();
  Serial.println("init led done!");

  Serial.println("init oled ...");
  oled_init();
  Serial.println("init oled done!");

  Serial.println("init dht...");
  delayMS = dht_init();
  Serial.println("init dht done!");

  Serial.println("init photoresister...");
  photoresister_init();
  Serial.println("init photoresister done!");

  digitalWrite(LED_PIN, LOW);
  Serial.println("system init complete!");

  server_init();
  mqtt_connect(edge_id, mqtt_host, mqtt_username, mqtt_password);
}

void loop() {
  delay(delayMS);

  mqtt_loop();
  float temperture = dht_read_temperture();
  float humidity = dht_read_humidity();
  bool is_dark = photoresister_is_dark();

  // display set up
  oled_dispay_setup();

  // Display temperture
  oled_print_temperture(temperture);

  // Display humidity
  oled_print_humidity(humidity);

  // display
  oled_dispay();

  // turn on/off led
  if (is_dark) {
    led_turn_on();
  } else {
    led_turn_off();
  }

  Serial.printf("Temp: %.2f C, Hum: %.2f %\n", temperture, humidity);
  mqtt_publish(temperture, humidity);
}
