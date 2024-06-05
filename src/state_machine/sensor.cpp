#include "sensor.h"

#include "define.h"

Sensor::Sensor(const int dht_pin, const uint8_t dht_type, const int photoresister_pin, SensorCallBackFunction fn)
	: onSensorDataChange(fn),
	  dht(dht_pin, dht_type),
	  photoresisterPin(photoresister_pin),
	  state(SENSOR_WAIT),
	  lastReadingTime(0) {
	this->dht.begin();
	pinMode(this->photoresisterPin, INPUT);
}

void Sensor::update() {
	unsigned long now = millis();

	switch (this->state) {
		case SensorState::SENSOR_IDLE:
			this->state = SensorState::SENSOR_READ;
			break;
		case SensorState::SENSOR_READ:
			this->onSensorDataChange(this->readSensor());
			this->state = SensorState::SENSOR_WAIT;
			this->lastReadingTime = now;
			break;
		case SensorState::SENSOR_WAIT:
			if (now - this->lastReadingTime > SENSOR_READING_INTERVAL) {
				this->state = SensorState::SENSOR_IDLE;
			}
			break;
	}
}

void Sensor::setCallback(Sensor::SensorCallBackFunction callback) {
	this->onSensorDataChange = callback;
}

SensorData Sensor::readSensor() {
	SensorData sensorData = {};
	sensors_event_t event;

	dht.humidity().getEvent(&event);
	if (!isnan(event.relative_humidity)) {
		sensorData.humidity = event.relative_humidity;
	}

	dht.temperature().getEvent(&event);
	if (!isnan(event.temperature)) {
		sensorData.temperture = event.temperature;
	}

	sensorData.photoresisterValue = analogRead(this->photoresisterPin);

	return sensorData;
}
