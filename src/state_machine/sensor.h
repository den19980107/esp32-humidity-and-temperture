#ifndef SENSOR_H
#define SENSOR_H

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <cstdint>
#include <functional>  // for std::function

struct SensorData {
	float temperture;
	float humidity;
	int photoresisterValue;
};

enum SensorState {
	SENSOR_IDLE,
	SENSOR_READ,
	SENSOR_WAIT,
};

class Sensor {
   public:
	using SensorCallBackFunction = std::function<void(SensorData)>;

	Sensor(const int dht_pin, const uint8_t dht_type, const int photoresister_pin, SensorCallBackFunction fn);
	void update();
	void setCallback(SensorCallBackFunction callback);

   private:
	unsigned long lastReadingTime;
	SensorState state;
	SensorData readSensor();
	SensorCallBackFunction onSensorDataChange;
	DHT_Unified dht;
	int photoresisterPin;
};

#endif
