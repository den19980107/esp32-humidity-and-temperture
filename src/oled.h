#ifndef OLED_H
#define OLED_H
#include <AsyncTCP.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

void oled_init();
void oled_dispay_setup();
void oled_print_humidity(float humidity);
void oled_print_temperture(float temperture);
void oled_print_ap_info(String ssid, IPAddress ip, IPAddress mask);
void oled_print_connect_wifi_info(String ssid, IPAddress ip, int8_t rssi,
                                  String macAddress);
void oled_print_mqtt_connecting();
void oled_print_mqtt_reconnecting();
void oled_print_mqtt_connect_success();
void oled_print_mqtt_connect_failed();
void oled_dispay();

#endif
