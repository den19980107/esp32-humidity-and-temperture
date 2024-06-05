#include "monitor.h"

Monitor::Monitor(const int width, const int height)
	: display(width, height, &Wire, -1), state(MONITOR_IDLE), lastBlockTime(0), blockDurationMs(0), ledOn(false) {
	// Initialize the display with the I2C address (for example, 0x3C for many displays)
	if (!this->display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {	 // Address 0x3D for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for (;;);  // Don't proceed, loop forever
	}
	// Clear the buffer
	this->display.clearDisplay();
}

void Monitor::update() {
	unsigned long now = millis();

	switch (this->state) {
		case MonitorState::MONITOR_IDLE:
			break;
		case MonitorState::MONITOR_BLOCK:
			if (now - this->lastBlockTime > this->blockDurationMs) {
				this->state = MONITOR_IDLE;
			}
			break;
		case MonitorState::MONTIOR_SHOW_SENSOR_DATA:
			this->prepareDisplay();
			this->display.printf("Temp: %.2f %cC\n", this->sensorData.temperture, (char)247);
			this->display.printf("Humi: %.2f %s\n", this->sensorData.humidity, "%");
			this->display.printf("PR: %.d\n", this->sensorData.photoresisterValue);
			this->display.display();
			this->lastBlockTime = now;
			this->state = MONITOR_IDLE;
			break;
		case MonitorState::MONITOR_SHOW_LED_STATUS:
			this->prepareDisplay();
			this->display.printf("LED: %s\n", this->ledOn ? "ON!" : "OFF!");
			this->display.display();
			this->lastBlockTime = now;
			this->state = MONITOR_BLOCK;
			break;
	}
}

void Monitor::handleSensorData(SensorData data) {
	if (this->state == MONITOR_BLOCK) {
		return;
	}

	this->state = MONTIOR_SHOW_SENSOR_DATA;
	this->sensorData = data;
}

void Monitor::handleLedStatusChange(bool ledOn, int blockDurationMs) {
	this->state = MONITOR_SHOW_LED_STATUS;
	this->blockDurationMs = blockDurationMs;
	this->ledOn = ledOn;
}

void Monitor::prepareDisplay() {
	this->display.clearDisplay();
	this->display.setTextSize(1);
	this->display.setTextColor(WHITE);
	this->display.setCursor(0, 10);
}