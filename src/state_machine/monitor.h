#ifndef MONITOR_H
#define MONITOR_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "state_machine/sensor.h"

enum MonitorState {
	MONITOR_IDLE,
	MONITOR_BLOCK,
	MONITOR_SHOW_SENSOR_DATA,
	MONITOR_SHOW_LED_STATUS,
};

class Monitor {
   public:
	Monitor(const int width, const int height);
	void handleSensorData(SensorData data);
	void handleLedStatusChange(bool ledOn, int blockDurationMs);
	void drawVerticalBar(uint16_t value);
	void update();
	bool isIdle();

   private:
	void prepareDisplay();
	Adafruit_SSD1306 display;
	MonitorState state;
	MonitorState previousState;
	SensorData sensorData;
	bool ledOn;
	unsigned long lastBlockTime;
	int blockDurationMs;
	void logStateChange();
	const char* stateToString(MonitorState state);
};

#endif
