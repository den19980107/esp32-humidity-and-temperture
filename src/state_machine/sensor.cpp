#include "sensor.h"

#include "define.h"

Sensor::Sensor(const int dht_pin, const uint8_t dht_type, const int photoresister_pin, const int led_pin,
			   std::function<void(SensorData)> fn)
	: onSensorDataChange(fn),
	  dht(dht_pin, dht_type),
	  photoresisterPin(photoresister_pin),
	  ledPin(led_pin),
	  state(SENSOR_WAIT),
	  lastReadingTime(0) {
	this->dht.begin();
	pinMode(this->photoresisterPin, INPUT);
}

void Sensor::update() {
	// this->logStateChange();
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

void Sensor::setCallback(std::function<void(SensorData)> callback) {
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
	sensorData.ledState = digitalRead(this->ledPin) == HIGH ? "on" : "off";

	return sensorData;
}

const char *Sensor::stateToString(SensorState state) {
	switch (state) {
		case SENSOR_IDLE:
			return "SENSOR_IDLE";
		case SENSOR_READ:
			return "SENSOR_READ";
		case SENSOR_WAIT:
			return "SENSOR_WAIT";
		default:
			return "UNKNOW";
	}
}

void Sensor::logStateChange() {
	if (this->state != this->previousState) {
		Serial.printf("[Sensor] change from %s to %s\n", this->stateToString(this->previousState),
					  this->stateToString(this->state));
	}
	this->previousState = this->state;
}
