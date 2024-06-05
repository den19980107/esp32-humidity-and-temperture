#include <Arduino.h>

#include "app/app.h"

void setup() {
	Serial.begin(115200);

	App app = App();
	app.run();
}

void loop() {
}
