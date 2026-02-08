#include "motion_detector.h"

const int pinPir = 12;

void setupMotion() {
  pinMode(pinPir, INPUT);
}

bool checkMotion() {
  int state = digitalRead(pinPir);

  if (state == HIGH) {
    return true;
  } else {
    return false;
  }
}