#include "photoresister.h"
#include <Wire.h>

void photoresister_init() { pinMode(PHOTORESISTER_PIN, INPUT); }

bool photoresister_is_dark() {
  int photoresisterValue = analogRead(PHOTORESISTER_PIN);
  if (photoresisterValue <= 900) {
    return true;
  }

  return false;
}