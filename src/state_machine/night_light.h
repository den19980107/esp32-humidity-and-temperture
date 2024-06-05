#ifndef NIGHT_LIGHT_H
#define NIGHT_LIGHT_H

#include <cstdint>

enum NightLightState {
	NIGHT_LIGHT_ON,
	NIGHT_LIGHT_WIAT,
	NIGHT_LIGHT_OFF,
};

class NightLight {
   public:
	NightLight(uint8_t ledPin);
	void turnOnForDuration(int ms);
	void turnOff();
	void update();

   private:
	NightLightState state;
	unsigned long lastTurnOnTime;
	int turnOnDuration;
	uint8_t ledPin;
};

#endif
