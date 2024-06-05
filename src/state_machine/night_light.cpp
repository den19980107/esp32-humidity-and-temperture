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
	unsigned long now = millis();

	switch (this->state) {
		case NIGHT_LIGHT_OFF:
			digitalWrite(this->ledPin, LOW);
			this->state = NIGHT_LIGHT_WIAT;
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
	}
}
