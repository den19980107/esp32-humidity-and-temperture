#include "app.h"

#include "define.h"
#include "state_machine/monitor.h"

App::App()
	: monitorSM(SCREEN_WIDTH, SCREEN_HEIGHT),
	  sensorSM(DHT_PIN, DHT_TYPE, PHOTORESISTER_PIN, nullptr),
	  nightLightSM(LED_PIN),
	  previousSensorData(nullptr) {
	Sensor::SensorCallBackFunction fn = [this](SensorData data) { this->sensorCallBackFn(data); };
	this->sensorSM.setCallback(fn);
};

void App::run() {
	while (true) {
		this->sensorSM.update();
		this->monitorSM.update();
		this->nightLightSM.update();
	}
}

void App::sensorCallBackFn(SensorData data) {
	Serial.printf("[get sensor data] temp: %.2f humi: %.2f, photoresister: %d\n", data.temperture, data.humidity,
				  data.photoresisterValue);

	// handle first read
	if (this->previousSensorData == nullptr) {
		this->monitorSM.handleSensorData(data);
		this->previousSensorData = new SensorData(data);
		return;
	}

	if (this->previousSensorData->humidity != data.humidity ||
		this->previousSensorData->temperture != data.temperture) {
		this->monitorSM.handleSensorData(data);
	}

	bool previousLedOn = previousSensorData->photoresisterValue <= PHOTORESISTER_THRESHOLD;
	bool currentLedOn = data.photoresisterValue <= PHOTORESISTER_THRESHOLD;

	if (previousLedOn != currentLedOn) {
		this->monitorSM.handleLedStatusChange(currentLedOn, NIGHT_LIGHT_BLOCK_DISPLAY_DURATION);
	}

	if (currentLedOn) {
		this->nightLightSM.turnOnForDuration(NIGHT_LIGHT_ON_DURATION);
	} else {
		this->nightLightSM.turnOff();
	}

	*this->previousSensorData = data;
}
