#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

void mqtt_connect(char edge_id[], char mqtt_server[], char mqtt_username[],
                  char mqtt_password[]);
void mqtt_loop();
void mqtt_publish(float temperture, float humidity);

void _reconnect();
void _callback(char *topic, byte *payload, unsigned int length);

#endif