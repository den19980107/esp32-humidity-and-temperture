#ifndef OLED_H
#define OLED_H

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

void oled_init();
void oled_dispay_setup();
void oled_print_humidity(float humidity);
void oled_print_temperture(float temperture);
void oled_dispay();

#endif
