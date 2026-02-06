#include <Arduino.h>
#include <TM1637.h>
#include <ChainableLED.h>
#include <security_animation.h>

void playSuccessAnimation(TM1637& display, ChainableLED& leds, int secretCode[4]) {
  leds.setColorHSB(0, 0.30, 1.0, 0.5); // TODO: Define colors as constants or enums, this is "blue" for now
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    delay(200);
    display.display(0, secretCode[0]);
    display.display(1, secretCode[1]);
    display.display(2, secretCode[2]);
    display.display(3, secretCode[3]);
    delay(2000);
    leds.setColorHSB(0, 0.0, 0.0, 0.0);
  }
}

void playErrorAnimation(TM1637& display, ChainableLED& leds) {
  leds.setColorHSB(0, 0.0, 1.0, 0.5); // TODO: Define colors as constants or enums, this is "red" for now
  for (int i = 0; i < 4; i++) {
    display.display(0, 0);
    display.display(1, 0);
    display.display(2, 0);
    display.display(3, 0);
    delay(100);
    display.clearDisplay();
    delay(100);
  }
}

