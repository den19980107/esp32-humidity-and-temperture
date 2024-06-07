#ifndef APP_H
#define APP_H

#include "state_machine/monitor.h"
#include "state_machine/night_light.h"
#include "state_machine/sensor.h"
#include "state_machine/server.h"

class App {
   public:
	App();
	void run();

   private:
	Monitor monitorSM;
	Sensor sensorSM;
	NightLight nightLightSM;
	WebServer serverSM;
	void sensorCallBackFn(SensorData data);
	SensorData *previousSensorData;
};

#endif
