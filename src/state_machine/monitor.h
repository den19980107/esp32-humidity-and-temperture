#ifndef MONITOR_H
#define MONITOR_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "state_machine/sensor.h"

enum MonitorState {
	MONITOR_IDLE,
	MONITOR_BLOCK,
	MONTIOR_SHOW_SENSOR_DATA,
	MONITOR_SHOW_LED_STATUS,
};

class Monitor {
   public:
	Monitor(const int width, const int height);
	void handleSensorData(SensorData data);
	void handleLedStatusChange(bool ledOn, int blockDurationMs);
	void update();

   private:
	void prepareDisplay();

	Adafruit_SSD1306 display;
	MonitorState state;
	SensorData sensorData;
	bool ledOn;
	unsigned long lastBlockTime;
	int blockDurationMs;
};

#endif
