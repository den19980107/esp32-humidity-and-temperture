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

void oled_print_ap_info(String ssid, IPAddress ip, IPAddress mask) {
  oled_dispay_setup();
  display.println("Please connect to esp32 wifi");
  display.println();
  display.printf("SSID: %s\n", ssid.c_str());
  display.printf("IP: %s\n", ip.toString().c_str());
  display.printf("MASK: %s\n", mask.toString().c_str());
  oled_dispay();
}

void oled_print_connect_wifi_info(String ssid, IPAddress ip, int8_t rssi,
                                  String macAddress) {
  oled_dispay_setup();
  display.println("Connect to wifi success!");
  display.printf("SSID: %s\n", ssid.c_str());
  display.printf("IP: %s\n", ip.toString().c_str());
  display.printf("RSSI: %d dbm\n", rssi);
  display.printf("MAC: %s\n", macAddress.c_str());
  oled_dispay();
  delay(5000);
}

void oled_print_mqtt_connecting() {
  oled_dispay_setup();
  display.println("Connecting to mqtt ...");
  oled_dispay();
}

void oled_print_mqtt_reconnecting() {
  oled_dispay_setup();
  display.println("Reconnecting to mqtt ...");
  oled_dispay();
}

void oled_print_mqtt_connect_success() {
  oled_dispay_setup();
  display.println("connect to mqtt suceess!");
  oled_dispay();
  delay(5000);
}

void oled_print_mqtt_connect_failed() {
  oled_dispay_setup();
  display.println("connect to mqtt failed! retry after 5 second ...");
  oled_dispay();
}

void oled_dispay() { display.display(); }
