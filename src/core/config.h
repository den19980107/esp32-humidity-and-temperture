#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include <ArduinoJson.h>

#include "interfaces.h"

struct WiFiConfig {
	char ssid[64];
	char password[64];
	char username[64];	// For enterprise WiFi
	bool isEnterprise;

	WiFiConfig() : isEnterprise(false) {
		ssid[0] = '\0';
		password[0] = '\0';
		username[0] = '\0';
	}

	bool isValid() const {
		return strlen(ssid) > 0 && strlen(password) > 0;
	}
};

struct MQTTConfig {
	char broker[128];
	char username[64];
	char password[64];
	char edgeId[32];
	int port;

	MQTTConfig() : port(1883) {
		broker[0] = '\0';
		username[0] = '\0';
		password[0] = '\0';
		edgeId[0] = '\0';
	}

	bool isValid() const {
		return strlen(broker) > 0 && strlen(edgeId) > 0;
	}
};

struct SensorConfig {
	int dhtPin;
	int dhtType;
	int photoresisterPin;
	int ledPin;
	int sdaPin;
	int sclPin;
	int photoresisterThreshold;
	unsigned long sensorReadingInterval;
	unsigned long uploadFrequency;
	unsigned long nightLightDuration;

	SensorConfig()
		: dhtPin(13),
		  dhtType(11),
		  photoresisterPin(39),
		  ledPin(25),
		  sdaPin(32),
		  sclPin(33),
		  photoresisterThreshold(800),
		  sensorReadingInterval(1000),
		  uploadFrequency(5000),
		  nightLightDuration(600000) {
	}
};

class Config {
   public:
	WiFiConfig wifi;
	MQTTConfig mqtt;
	SensorConfig sensor;

	ErrorCode loadFromFile(const char* filename);
	ErrorCode saveToFile(const char* filename);
	bool validate();
	void setDefaults();

   private:
	ErrorCode parseWiFiConfig(JsonObject& obj);
	ErrorCode parseMQTTConfig(JsonObject& obj);
	ErrorCode parseSensorConfig(JsonObject& obj);
	void serializeToJson(JsonDocument& doc);
};

#endif
