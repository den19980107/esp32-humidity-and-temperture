#include "mqtt.h"
#include "oled.h"
#include <PubSubClient.h>
#include <WiFi.h>

char *_edge_id;
char *_mqtt_server;
char *_mqtt_username;
char *_mqtt_password;

WiFiClient espClient;
PubSubClient client(espClient);

void mqtt_connect(char edge_id[], char mqtt_server[], char mqtt_username[],
                  char mqtt_password[]) {
  oled_print_mqtt_connecting();
  _edge_id = edge_id;
  _mqtt_server = mqtt_server;
  _mqtt_username = mqtt_username;
  _mqtt_password = mqtt_password;

  client.setServer(mqtt_server, 1883);
  client.setCallback(_callback);

  if (client.connect(_edge_id, _mqtt_username, _mqtt_password)) {
    oled_print_mqtt_connect_success();
  } else {
    oled_print_mqtt_connect_failed();
  }
}

void mqtt_loop() {
  if (!client.connected()) {
    _reconnect();
  }
  client.loop();
}

void mqtt_publish(float temperture, float humidity) {
  Serial.println("mqtt publish");
  char topic[32];
  sprintf(topic, "Advantech.%s.data", _edge_id);

  char msg[256];
  sprintf(msg, "{\"temp\":%.2f, \"hum\":%.2f}", temperture, humidity);

  Serial.printf("[publish] topic: %s, patload: %s", topic, msg);

  client.publish(topic, msg);
}

void _reconnect() {
  oled_print_mqtt_reconnecting();
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(_edge_id, _mqtt_username, _mqtt_password)) {
      Serial.println("connected");
      oled_print_mqtt_connect_success();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      oled_print_mqtt_connect_failed();
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void _callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}