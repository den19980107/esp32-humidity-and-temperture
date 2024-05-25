/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
*********/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHT_PIN 13
#define DHT_TYPE DHT11
DHT_Unified dht(DHT_PIN, DHT_TYPE);
uint32_t delayMS;

#define PHOTORESISTER_PIN 39
#define LED_PIN 2

void init_oled();
void init_dht();
void init_photoresister();
void init_led();

void setup() {
  Serial.begin(115200);

  Serial.println("init led...");
  init_led();
  digitalWrite(LED_PIN, HIGH);
  Serial.println("init led done!");

  Serial.println("init oled ...");
  init_oled();
  Serial.println("init oled done!");

  Serial.println("init dht...");
  init_dht();
  Serial.println("init dht done!");

  Serial.println("init photoresister...");
  init_photoresister();
  Serial.println("init photoresister done!");

  digitalWrite(LED_PIN, LOW);
  Serial.println("system init complete!");
}

void loop() {
  delay(delayMS);
  float temperture = NAN;
  float humidity = NAN;
  int photoresisterValue = 0;
  bool ledOn = false;

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    temperture = event.temperature;
  }

  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    humidity = event.relative_humidity;
  }

  photoresisterValue = analogRead(PHOTORESISTER_PIN);
  if (photoresisterValue <= 900) {
    ledOn = true;
  } else {
    ledOn = false;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  // Display temperture
  display.printf("Temp: %.2f %cC\n", temperture, (char)247);
  display.println();

  // Display humidity
  display.printf("Hum: %.2f %\n", humidity);
  display.println();

  // Display humidity
  display.printf("LED: %s (%d)\n", ledOn ? "ON" : "OFF", photoresisterValue);
  display.println();
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);

  // display
  display.display();

  Serial.printf("Temp: %.2f C, Hum: %.2f %, LED: %s (%d)\n", temperture,
                humidity, ledOn ? "ON" : "OFF", photoresisterValue);
}

void init_oled() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("booting ...");
  display.display();
}

void init_dht() {
  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void init_photoresister() { pinMode(PHOTORESISTER_PIN, INPUT); }

void init_led() { pinMode(LED_PIN, OUTPUT); }
