#include "night_light.h"

#include <Adafruit_Sensor.h>
#include <Wire.h>

NightLight::NightLight(uint8_t ledPin) : ledPin(ledPin), state(NIGHT_LIGHT_OFF), lastTurnOnTime(0), turnOnDuration(0) {
	pinMode(ledPin, OUTPUT);
}

void NightLight::turnOnForDuration(int ms) {
	this->state = NIGHT_LIGHT_ON;
	this->turnOnDuration = ms;
}

void NightLight::turnOff() {
	this->state = NIGHT_LIGHT_OFF;
}

void NightLight::update() {
	// this->logStateChange();
	unsigned long now = millis();

	switch (this->state) {
		case NIGHT_LIGHT_OFF:
			digitalWrite(this->ledPin, LOW);
			this->state = NIGHT_LIGHT_IDLE;
			break;
		case NIGHT_LIGHT_ON:
			digitalWrite(this->ledPin, HIGH);
			this->state = NIGHT_LIGHT_WIAT;
			this->lastTurnOnTime = now;
			break;
		case NIGHT_LIGHT_WIAT:
			if (now - this->lastTurnOnTime > this->turnOnDuration) {
				this->state = NIGHT_LIGHT_OFF;
			}
			break;
		case NIGHT_LIGHT_IDLE:
			break;
	}
}

const char *NightLight::stateToString(NightLightState state) {
	switch (state) {
		case NIGHT_LIGHT_OFF:
			return "NIGHT_LIGHT_OFF";
		case NIGHT_LIGHT_ON:
			return "NIGHT_LIGHT_ON";
		case NIGHT_LIGHT_WIAT:
			return "NIGHT_LIGHT_WIAT";
		case NIGHT_LIGHT_IDLE:
			return "NIGHT_LIGHT_IDLE";
		default:
			return "UNKNOW";
	}
}

void NightLight::logStateChange() {
	if (this->state != this->previousState) {
		Serial.printf("[NightLight] change from %s to %s\n", this->stateToString(this->previousState),
					  this->stateToString(this->state));
	}
	this->previousState = this->state;
}
