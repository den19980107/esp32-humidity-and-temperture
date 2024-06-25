#ifndef SERVER_H
#define SERVER_H

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
	const char* stateToString(ServerState state);
	unsigned long lastCheckTime;
	unsigned long lastConnectWifiTime;
};

#endif
