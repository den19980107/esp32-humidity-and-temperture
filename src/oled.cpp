#include "oled.h"
#include <Adafruit_SSD1306.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void oled_init() {
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

void oled_print_humidity(float humidity) {
  display.printf("Hum: %.2f %\n", humidity);
  display.println();
}

void oled_print_temperture(float temperture) {
  display.printf("Temp: %.2f %cC\n", temperture, (char)247);
  display.println();
}

void oled_dispay_setup() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
}

void oled_dispay() { display.display(); }
