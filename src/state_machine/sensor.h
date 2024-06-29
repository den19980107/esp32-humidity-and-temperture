#ifndef SENSOR_H
#define SENSOR_H

#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>

#include <cstdint>
#include <functional>  // for std::function

struct SensorData {
	float temperture;
	float humidity;
	int photoresisterValue;
	const char* ledState;
	const char* toJson() {
		JsonDocument doc;
		doc["temp"] = this->temperture;
		doc["humi"] = this->humidity;
		doc["photoresister"] = this->photoresisterValue;
		doc["ledState"] = this->ledState;

		size_t payloadSize = measureJson(doc) + 1;
		char* payload = new char[payloadSize];
		serializeJson(doc, payload, payloadSize);
		return payload;
	}
};

enum SensorState {
	SENSOR_IDLE,
	SENSOR_READ,
	SENSOR_WAIT,
};

class Sensor {
   public:
	Sensor(const int dht_pin, const uint8_t dht_type, const int photoresister_pin, const int led_pin,
		   std::function<void(SensorData)> fn);
	void update();
	void setCallback(std::function<void(SensorData)> callback);

   private:
	unsigned long lastReadingTime;
	SensorState state;
	SensorState previousState;
	SensorData readSensor();
	std::function<void(SensorData)> onSensorDataChange;
	DHT_Unified dht;
	int photoresisterPin;
	int ledPin;
	void logStateChange();
	const char* stateToString(SensorState state);
};

#endif
