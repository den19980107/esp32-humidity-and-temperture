#ifndef SERVER_H
#define SERVER_H

#include <ArduinoJson.h>

#include <vector>

#include "ESPAsyncWebServer.h"
#include "IPAddress.h"
#include "PubSubClient.h"
#include "state_machine/sensor.h"

enum ServerState {
	START_AP,
	START_SERVER,
	CHECK_WIFI_CONFIG,
	SCAN_WIFI,
	WAIT_WIFI_CONFIG,
	CONNECT_WIFI,
	WAIT_WIFI_CONNECTED,
	WAIT_DEVICE_CONFIG,
	CONNECT_MQTT,
	PUBLISH_HOME_ASSISTANT_DISCOVERY,
	CHECK_DEVICE_CONFIG_CHANGE,
	CHECK_WIFI_CONFIG_CHANGE,
	WAIT
};

struct WifiConfig {
	char* ssid;
	char* username;
	char* password;

	// Constructor
	WifiConfig(const char* ssid, const char* username, const char* password)
		: ssid(strdup(ssid)), username(strdup(username)), password(strdup(password)) {
	}

	// Destructor to free allocated memory
	~WifiConfig() {
		free(ssid);
		free(username);
		free(password);
	}
};

struct DeviceConfig {
	char* edgeId;
	char* mqttHost;
	char* mqttUserName;
	char* mqttPassword;

	// Constructor
	DeviceConfig(const char* edgeId, const char* mqttHost, const char* mqttUserName, const char* mqttPassword)
		: edgeId(strdup(edgeId)),
		  mqttHost(strdup(mqttHost)),
		  mqttUserName(strdup(mqttUserName)),
		  mqttPassword(strdup(mqttPassword)) {
	}

	// Destructor to free allocated memory
	~DeviceConfig() {
		free(edgeId);
		free(mqttHost);
		free(mqttUserName);
		free(mqttPassword);
	}
};

struct ApConfig {
	char* ssid;
	IPAddress ip;
	IPAddress mask;

	// Constructor
	ApConfig(const char* ssid, IPAddress ip, IPAddress mask) : ssid(strdup(ssid)), ip(ip), mask(mask) {
	}

	// Destructor to free allocated memory
	~ApConfig() {
		free(ssid);
	}
};

struct ScannedWifi {
	char* ssid;
	bool encrypted;	  // require password
	bool enterprise;  // require username and password

	ScannedWifi(const char* ssid, const bool encrypted, const bool enterprise)
		: ssid(strdup(ssid)), encrypted(encrypted), enterprise(enterprise) {
	}

	// ~ScannedWifi() {
	// 	free(ssid);
	// }
};

class WebServer {
   public:
	WebServer();
	void update();
	void publish(SensorData data);
	void setCallback(std::function<void(bool)> callback);

   private:
	ServerState state;
	ServerState previousState;
	ApConfig apConfig;
	WifiConfig* wifiConfig;
	DeviceConfig* deviceConfig;
	AsyncWebServer server;
	WiFiClient wifiClient;
	PubSubClient pubSubClient;
	std::vector<ScannedWifi> scannedWifis;
	void registRouter();
	void startAp();
	void connectWifi();
	std::vector<ScannedWifi> scanWifi();
	bool connectMQTT();
	void logStateChange();
	void publishHomeAssistantDiscovery();
	void mqttCallBack(char* topic, byte* payload, unsigned int length);
	const char* stateToString(ServerState state);
	unsigned long lastCheckTime;
	unsigned long lastConnectWifiTime;
	std::function<void(bool)> ledCallBackFunction;
};

struct HADeviceConfig {
	const char* name;
	const char* identifiers;
	void toJson(JsonDocument& doc) {
		doc["name"] = this->name;
		doc["identifiers"] = this->identifiers;
	};
};

struct HASensorConfig {
	const char* name;
	const char* unique_id;
	const char* state_topic;
	const char* unit_of_measurement;
	const char* value_template;
	HADeviceConfig* device;
	void toJson(JsonDocument& doc) {
		doc["name"] = this->name;
		doc["unique_id"] = this->unique_id;
		doc["state_topic"] = this->state_topic;
		doc["unit_of_measurement"] = this->unit_of_measurement;
		doc["value_template"] = this->value_template;

		JsonDocument deviceJson;
		this->device->toJson(deviceJson);
		doc["device"] = deviceJson;
	};
};

struct HALightConfig {
	const char* name;
	const char* unique_id;
	const char* command_topic;
	const char* state_topic;
	const char* state_value_template;
	const char* payload_on;
	const char* payload_off;
	bool optimistic;
	HADeviceConfig* device;
	void toJson(JsonDocument& doc) {
		doc["name"] = this->name;
		doc["unique_id"] = this->unique_id;
		doc["command_topic"] = this->command_topic;
		doc["state_topic"] = this->state_topic;
		doc["state_value_template"] = this->state_value_template;
		doc["payload_on"] = this->payload_on;
		doc["payload_off"] = this->payload_off;
		doc["optimistic"] = this->optimistic;

		JsonDocument deviceJson;
		this->device->toJson(deviceJson);
		doc["device"] = deviceJson;
	};
};

#endif
