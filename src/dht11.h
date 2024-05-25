#include <Adafruit_Sensor.h>
#ifndef DHT11_H
#define DHT11_H

#define DHT_PIN 13
#define DHT_TYPE DHT11

uint32_t dht_init();
float dht_read_humidity();
float dht_read_temperture();

#endif