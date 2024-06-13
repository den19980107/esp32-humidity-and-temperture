# Project Description

this is a iot project for me to enter the embedded programing world.

the functionality for this project:

1. detect temperture and humidity of the room using the dht11 sensor
2. detect the light intensity of the room and open the night light led if room is too dark
3. upload the sensor data that esp32 readed and upload to the cloud using mqtt

## Hardware

### Project Photo

front:

![截圖 2024-06-13 下午6.27.32](https://i.imgur.com/OM2pc4W.jpeg)

back:

![截圖 2024-06-13 下午6.27.39](https://i.imgur.com/whOgIty.jpeg)

### The Schematic Diegram

![截圖 2024-06-13 下午6.29.00](https://i.imgur.com/LqW8kUU.png)

### The PCB Latout

![截圖 2024-06-13 下午6.30.04](https://i.imgur.com/0F4Rr59.png)



## Software

### Requirement

* platformio cli
* make

### How to run

1. clone the repository

2. plugin the esp32 to your computer

3. run this command to install all dependency this project use

   ```shell
   pio run -t compiledb
   ```

4. run this command, the code should be upload to the esp32 and execute

   ```shell
   make upload
   ```

5. run this command, it will show the output log from esp32

   ```shell
   make monitor
   ```

### State Machine Diagram

this project is design using a pattern called `finite state machine`, which is commonly used in the embedded system.

for this project, i create 4 state machine to implement all feature, which include:

1. read sensor data
2. display sensor data at ssd1306 monitor
3. create a web server for fill up the wifi & device config
4. upload the sensor data to the cloud via mqtt

#### Monitor state machine

this state machine response to display infomation to the monitor, when nothing happen, it should be display the sensor data, but when the led change the status (turn on or turn off), it should display a full frame message to indicate this event

```mermaid
---
title: Monitor State Machine
---
stateDiagram-v2
    IDLE --> SHOW_SENSOR_DATA: when sensor update
    SHOW_SENSOR_DATA --> IDLE: always
    IDLE --> SHOW_LED_STATUS: when led change status
    SHOW_LED_STATUS --> BLOCK: always
    BLOCK --> IDLE: after blocking duration
```

#### Sensor state machine

this state machine response to read the sensor data every second and send by callback function

```mermaid
---
title: Sensor State Machine
---
stateDiagram-v2
    IDLE --> READ: always
    READ --> WAIT: always
    WAIT --> IDLE: after 1 seconds
```

#### Nigh light state machine

this state machine response to detect the photoresister value is below 900 (which means the room is dark), and open the led as a night light

```mermaid
---
title: Night Light State Machine
---
stateDiagram-v2
    OFF --> ON: when photoresister value <= 900
    ON --> WAIT: always
    WAIT --> OFF: after 30 minute
```

#### Server state machine

this state machine response for create a web server and handle the incoming request for fill up the wifi and device config

```mermaid
---
title: Server State Machine
---
stateDiagram-v2
		START_AP --> START_SERVER: always
		START_SERVER --> CHECK_WIFI_CONFIG: always
		CHECK_WIFI_CONFIG --> CONNECT_WIFI: if have wifi config
		CHECK_WIFI_CONFIG --> SCAN_WIFI: if not have wifi config
		SCAN_WIFI --> WAIT_WIFI_CONFIG: always
		WAIT_WIFI_CONFIG --> CONNECT_WIFI: if have wifi config
		CONNECT_WIFI --> WAIT_WIFI_CONNECTED: always
		WAIT_WIFI_CONNECTED --> WAIT_DEVICE_CONFIG: when wifi connected
    WAIT_DEVICE_CONFIG --> CONNECT_MQTT: when have device config
    CONNECT_MQTT --> CHECK_DEVICE_CONFIG_CHANGE: always
    CHECK_DEVICE_CONFIG_CHANGE --> CONNECT_MQTT: when device config change
    CHECK_DEVICE_CONFIG_CHANGE --> CHECK_WIFI_CONFIG_CHANGE: if device config not change
    CHECK_WIFI_CONFIG_CHANGE --> WAIT: if wifi config not change
    CHECK_WIFI_CONFIG_CHANGE --> CONNECT_WIFI: if wifi config change
    WAIT --> CHECK_DEVICE_CONFIG_CHANGE: after 10 second
```
