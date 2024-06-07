#include "server.h"

#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "define.h"
#include "esp_wpa2.h"
#include "util/file.h"

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();  // In case 'to' contains 'from', like replacing
								   // 'x' with 'yx'
	}
}

WebServer::WebServer()
	: apConfig("esp32", IPAddress(192, 168, 7, 1), IPAddress(255, 255, 255, 0)),
	  wifiConfig(nullptr),
	  deviceConfig(nullptr),
	  state(ServerState::START_AP),
	  wifiClient(),
	  pubSubClient(wifiClient),
	  server(80) {
}

void WebServer::update() {
	this->logStateChange();
	unsigned long now = millis();
	switch (this->state) {
		case START_AP:
			this->startAp();
			this->state = START_SERVER;
			break;
		case START_SERVER:
			this->registRouter();
			this->state = CHECK_WIFI_CONFIG;
			break;
		case CHECK_WIFI_CONFIG:
			if (this->wifiConfig == nullptr) {
				this->state = SCAN_WIFI;
			} else {
				this->state = CONNECT_WIFI;
			}
			break;
		case SCAN_WIFI:
			this->scannedWifis = this->scanWifi();
			this->state = WAIT_WIFI_CONFIG;
			break;
		case WAIT_WIFI_CONFIG:
			if (this->wifiConfig != nullptr) {
				this->state = CONNECT_WIFI;
			}
			break;
		case CONNECT_WIFI:
			this->connectWifi();
			this->lastConnectWifiTime = now;
			this->state = WAIT_WIFI_CONNECTED;
			break;
		case WAIT_WIFI_CONNECTED:
			if (WiFi.status() == WL_CONNECTED) {
				Serial.printf("[wifi connected] ip: %s\n", WiFi.localIP().toString().c_str());
				this->state = WAIT_DEVICE_CONFIG;
			}

			if (now - this->lastConnectWifiTime > WIFI_CONNECT_TIMEOUT) {
				Serial.println("[connect wifi time out]");
				this->wifiConfig = nullptr;
				this->state = WAIT_WIFI_CONFIG;
			}
			break;
		case WAIT_DEVICE_CONFIG:
			if (this->deviceConfig != nullptr) {
				this->state = CONNECT_MQTT;
			}
			break;
		case CONNECT_MQTT:
			if (this->connectMQTT()) {
				this->state = CHECK_DEVICE_CONFIG_CHANGE;
				return;
			}

			Serial.println("[connect mqtt failed]");
			this->deviceConfig = nullptr;
			this->state = WAIT_DEVICE_CONFIG;
			break;
		case CHECK_DEVICE_CONFIG_CHANGE:
			this->state = CHECK_WIFI_CONFIG_CHANGE;
			break;
		case CHECK_WIFI_CONFIG_CHANGE:
			this->state = WAIT;
			break;
		case WAIT:
			if (now - this->lastCheckTime > CHECK_WIFI_DEVICE_CONFIG_INTERVAL) {
				this->lastCheckTime = now;
				this->state = CHECK_DEVICE_CONFIG_CHANGE;
			}
			break;
	}
}

void WebServer::publish(SensorData data) {
	if (this->deviceConfig == nullptr) {
		return;
	}

	char topic[32];
	sprintf(topic, "Advantech.%s.data", this->deviceConfig->edgeId);

	char msg[256];
	sprintf(msg, "{\"temp\":%.2f, \"humi\":%.2f, \"photoresister\":%d}", data.temperture, data.humidity,
			data.photoresisterValue);

	Serial.printf("[publish] topic: %s, patload: %s\n", topic, msg);

	this->pubSubClient.publish(topic, msg);
}

void WebServer::registRouter() {
	this->server.on("/", HTTP_GET, [=](AsyncWebServerRequest *request) {
		std::string anchor = "{{options}}";
		std::string optionsHtml = "";

		for (int i = 0; i < this->scannedWifis.size(); i++) {
			char buff[500];
			int n = sprintf(buff, "<option value=\"%s\">%s</option>", this->scannedWifis[i].ssid,
							this->scannedWifis[i].ssid);
			optionsHtml += buff;
		}

		std::string index_html = readFile(SPIFFS, "/index.html");

		replaceAll(index_html, anchor, optionsHtml);

		request->send(200, "text/html", String(index_html.c_str()));
	});

	this->server.on("/connectWifi", HTTP_POST, [=](AsyncWebServerRequest *request) {
		int paramsNr = request->params();
		const char *ssid, *username, *password;

		for (int i = 0; i < paramsNr; i++) {
			AsyncWebParameter *p = request->getParam(i);

			if (p->name() == "ssid") {
				ssid = p->value().c_str();
			} else if (p->name() == "username") {
				username = p->value().c_str();
			} else if (p->name() == "password") {
				password = p->value().c_str();
			} else {
				continue;
			}
		}

		std::string change_wifi = readFile(SPIFFS, "/change_wifi.html");
		request->send(200, "text/html", String(change_wifi.c_str()));

		this->wifiConfig = new WifiConfig(ssid, username, password);
	});

	this->server.on("/deviceConfig", HTTP_GET, [=](AsyncWebServerRequest *request) {
		std::string device_config_html = readFile(SPIFFS, "/device_config.html");
		request->send(200, "text/html", String(device_config_html.c_str()));
	});

	this->server.on("/connectCloud", HTTP_POST, [=](AsyncWebServerRequest *request) {
		int paramsNr = request->params();
		const char *edgeId, *mqttHost, *mqttUserName, *mqttPassword;

		for (int i = 0; i < paramsNr; i++) {
			AsyncWebParameter *p = request->getParam(i);

			if (p->name() == "edgeId") {
				edgeId = p->value().c_str();
			} else if (p->name() == "mqttHost") {
				mqttHost = p->value().c_str();
			} else if (p->name() == "mqttUserName") {
				mqttUserName = p->value().c_str();
			} else if (p->name() == "mqttPassword") {
				mqttPassword = p->value().c_str();
			} else {
				continue;
			}
		}

		this->deviceConfig = new DeviceConfig(edgeId, mqttHost, mqttUserName, mqttPassword);
		request->send(200, "text/html", "finish");
	});

	server.begin();
}

void WebServer::startAp() {
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP(this->apConfig.ssid);
	WiFi.softAPConfig(this->apConfig.ip, this->apConfig.ip, this->apConfig.mask);
}

void WebServer::connectWifi() {
	WiFi.disconnect();

	if (this->wifiConfig->username == nullptr || this->wifiConfig->username[0] == '\0') {
		Serial.printf("connect to %s with passwrod: %s\n", this->wifiConfig->ssid, this->wifiConfig->password);
		WiFi.begin(this->wifiConfig->ssid, this->wifiConfig->password);
	} else {
		Serial.printf("connect to %s with username: %s and passwrod: %s\n", this->wifiConfig->ssid,
					  this->wifiConfig->username, this->wifiConfig->password);
		WiFi.begin(this->wifiConfig->ssid);
		WiFi.setHostname("ESP32");
		esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)this->wifiConfig->username, strlen(this->wifiConfig->username));
		esp_wifi_sta_wpa2_ent_set_username((uint8_t *)this->wifiConfig->username, strlen(this->wifiConfig->username));
		esp_wifi_sta_wpa2_ent_set_password((uint8_t *)this->wifiConfig->password, strlen(this->wifiConfig->password));
		esp_wifi_sta_wpa2_ent_enable();
	}
}

std::vector<ScannedWifi> WebServer::scanWifi() {
	Serial.println("Scanning for WiFi networks...");

	std::vector<ScannedWifi> wifis;

	// Perform the Wi-Fi scan
	int n = WiFi.scanNetworks();

	if (n == 0) {
		Serial.println("No networks found");
		return wifis;
	}

	for (int i = 0; i < n; ++i) {
		char *buf = new char[WiFi.SSID(i).length() + 1];
		std::strcpy(buf, WiFi.SSID(i).c_str());
		wifis.push_back(ScannedWifi(buf, WiFi.encryptionType(i) == WIFI_AUTH_OPEN,
									WiFi.encryptionType(i) == WIFI_AUTH_WPA2_ENTERPRISE));
	}

	// Clear the results from the scan to free up memory
	WiFi.scanDelete();

	return wifis;
}

bool WebServer::connectMQTT() {
	Serial.printf("connect to %s using edgeId: %s, username: %s, password: %s\n", this->deviceConfig->mqttHost,
				  this->deviceConfig->edgeId, this->deviceConfig->mqttUserName, this->deviceConfig->mqttPassword);
	this->pubSubClient.setServer(this->deviceConfig->mqttHost, 1883);
	return this->pubSubClient.connect(this->deviceConfig->edgeId, this->deviceConfig->mqttUserName,
									  this->deviceConfig->mqttPassword);
}

const char *WebServer::serverStateToString(ServerState state) {
	switch (state) {
		case START_AP:
			return "START_AP";
		case START_SERVER:
			return "START_SERVER";
		case CHECK_WIFI_CONFIG:
			return "CHECK_WIFI_CONFIG";
		case SCAN_WIFI:
			return "SCAN_WIFI";
		case WAIT_WIFI_CONFIG:
			return "WAIT_WIFI_CONFIG";
		case CONNECT_WIFI:
			return "CONNECT_WIFI";
		case WAIT_WIFI_CONNECTED:
			return "WAIT_WIFI_CONNECTED";
		case WAIT_DEVICE_CONFIG:
			return "WAIT_DEVICE_CONFIG";
		case CONNECT_MQTT:
			return "CONNECT_MQTT";
		case CHECK_DEVICE_CONFIG_CHANGE:
			return "CHECK_DEVICE_CONFIG_CHANGE";
		case CHECK_WIFI_CONFIG_CHANGE:
			return "CHECK_WIFI_CONFIG_CHANGE";
		case WAIT:
			return "WAIT";
		default:
			return "UNKNOW";
	}
}

void WebServer::logStateChange() {
	if (this->state != this->previousState) {
		Serial.printf("change from %s to %s\n", this->serverStateToString(this->previousState),
					  this->serverStateToString(this->state));
	}
	this->previousState = this->state;
}
