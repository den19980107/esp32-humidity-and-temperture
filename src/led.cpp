#include "led.h"
#include <Wire.h>

void led_init() { pinMode(LED_PIN, OUTPUT); }

void led_turn_on() { digitalWrite(LED_PIN, HIGH); }

void led_turn_off() { digitalWrite(LED_PIN, LOW); }