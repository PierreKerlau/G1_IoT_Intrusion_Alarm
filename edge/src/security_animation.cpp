#include <Arduino.h>
#include <ChainableLED.h>
#include <TM1637.h>
#include <security_animation.h>

/**
 * Turn on the LED and display the current combination for a few seconds, then turn off the LED and clear the display.
 * This is called when the correct code is entered.
 */
void playSuccessAnimation(TM1637& display, ChainableLED& leds, std::array<int, 4> currentCombination) {
  leds.setColorHSB(0, 0.30, 1.0, 0.5); // TODO: Define colors as constants or enums, this is "blue" for now
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    delay(200);
    display.display(0, currentCombination[0]);
    display.display(1, currentCombination[1]);
    display.display(2, currentCombination[2]);
    display.display(3, currentCombination[3]);
    delay(2000);
    leds.setColorHSB(0, 0.0, 0.0, 0.0);
  }
}

/**
 * Blink the LED and display an error pattern "Err" on the screen a few times.
 * This is called when the wrong code is entered.
 */
void playErrorAnimation(TM1637& display, ChainableLED& leds) {
  leds.setColorHSB(0, 0.0, 1.0, 0.5); // TODO: Define colors as constants or enums, this is "red" for now
  for (int i = 0; i < 4; i++) {
    display.display(0, 'E');
    display.display(1, 'r');
    display.display(2, 'r');
    display.display(3, 0x7f);
    delay(100);
    display.clearDisplay();
    delay(100);
  }
}
