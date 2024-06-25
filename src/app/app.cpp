#include "app.h"

#include <SPIFFS.h>

#include "define.h"
#include "state_machine/monitor.h"

App::App()
	: monitorSM(SCREEN_WIDTH, SCREEN_HEIGHT),
	  sensorSM(DHT_PIN, DHT_TYPE, PHOTORESISTER_PIN, nullptr),
	  nightLightSM(LED_PIN),
	  serverSM(),
	  previousSensorData(nullptr) {
	Sensor::SensorCallBackFunction fn = [this](SensorData data) { this->sensorCallBackFn(data); };
	this->sensorSM.setCallback(fn);
};

void App::run() {
	if (!SPIFFS.begin(true)) {
		Serial.println("An error has occurred while mounting SPIFFS");
		return;
	}

	while (true) {
		this->sensorSM.update();
		this->monitorSM.update();
		this->nightLightSM.update();
		this->serverSM.update();
	}
}

void App::sensorCallBackFn(SensorData data) {
	unsigned long now = millis();

	if (now - this->lastUploadTime > UPLOAD_FRQUENCY) {
		this->serverSM.publish(data);
		this->lastUploadTime = now;
	}

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

		if (currentLedOn) {
			this->nightLightSM.turnOnForDuration(NIGHT_LIGHT_ON_DURATION);
		} else {
			this->nightLightSM.turnOff();
		}
	}

	*this->previousSensorData = data;
}
