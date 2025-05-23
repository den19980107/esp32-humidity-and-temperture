#include "monitor.h"

#include "define.h"

Monitor::Monitor(const int width, const int height)
	: display(width, height, &Wire, -1), state(MONITOR_IDLE), lastBlockTime(0), blockDurationMs(0), ledOn(false) {
	// Set custom I2C pins (SDA = GPIO32, SCL = GPIO33)
	Wire.begin(SDA_PIN, SCL_PIN);

	// Initialize the display with the I2C address (for example, 0x3C for many displays)
	if (!this->display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {	 // Address 0x3D for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for (;;);  // Don't proceed, loop forever
	}
	// Clear the buffer
	this->display.clearDisplay();
}

void Monitor::update() {
	this->logStateChange();
	unsigned long now = millis();

	switch (this->state) {
		case MonitorState::MONITOR_IDLE:
			break;
		case MonitorState::MONITOR_BLOCK:
			if (now - this->lastBlockTime > this->blockDurationMs) {
				this->state = MONITOR_IDLE;
			}
			break;
		case MonitorState::MONITOR_SHOW_SENSOR_DATA:
			this->prepareDisplay();
			this->display.printf(" %.1f%cC\n", this->sensorData.temperture, (char)247);
			this->display.println();
			this->display.printf(" %.1f%s\n", this->sensorData.humidity, "%");
			this->drawVerticalBar(this->sensorData.photoresisterValue);
			this->display.display();
			this->lastBlockTime = now;
			this->state = MONITOR_IDLE;
			break;
		case MonitorState::MONITOR_SHOW_LED_STATUS:
			this->prepareDisplay();

			const char* message = this->ledOn ? "LED ON" : "LED OFF";
			int16_t x1, y1;
			uint16_t w, h;
			display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);

			// Calculate the starting position to center the text
			int16_t x = (SCREEN_WIDTH - w) / 2;
			int16_t y = (SCREEN_HEIGHT - h) / 2;

			// Set the cursor to the calculated position
			display.setCursor(x, y);

			// Print the message
			display.print(message);

			// Display the message on the screen
			this->display.display();

			this->lastBlockTime = now;
			this->state = MONITOR_BLOCK;
			break;
	}
}

bool Monitor::isIdle() {
	return this->state == MonitorState::MONITOR_IDLE;
}

void Monitor::handleSensorData(SensorData data) {
	if (this->state == MONITOR_BLOCK) {
		return;
	}

	this->state = MONITOR_SHOW_SENSOR_DATA;
	this->sensorData = data;
}

void Monitor::handleLedStatusChange(bool ledOn, int blockDurationMs) {
	this->state = MONITOR_SHOW_LED_STATUS;
	this->blockDurationMs = blockDurationMs;
	this->ledOn = ledOn;
}

void Monitor::prepareDisplay() {
	this->display.clearDisplay();
	this->display.setTextSize(2);
	this->display.setTextColor(WHITE);
	this->display.setCursor(0, 10);
}

const char* Monitor::stateToString(MonitorState state) {
	switch (state) {
		case MonitorState::MONITOR_IDLE:
			return "MONITOR_IDLE";
		case MonitorState::MONITOR_BLOCK:
			return "MONITOR_BLOCK";
		case MonitorState::MONITOR_SHOW_SENSOR_DATA:
			return "MONITOR_SHOW_SENSOR_DATA";
		case MonitorState::MONITOR_SHOW_LED_STATUS:
			return "MONITOR_SHOW_LED_STATUS";
		default:
			return "UNKNOW";
	}
}

void Monitor::logStateChange() {
	if (this->state != this->previousState) {
		Serial.printf("[Monitor] change from %s to %s\n", this->stateToString(this->previousState),
					  this->stateToString(this->state));
	}
	this->previousState = this->state;
}

void Monitor::drawVerticalBar(uint16_t value) {
	float ratio = value / 4096.0;
	int barHeight = SCREEN_HEIGHT * ratio;
	int barWidth = 8;
	int x = SCREEN_WIDTH - barWidth;
	int y = SCREEN_HEIGHT - barHeight;

	// Draw the vertical bar
	this->display.fillRect(x, y, barWidth, barHeight, SSD1306_WHITE);
}
