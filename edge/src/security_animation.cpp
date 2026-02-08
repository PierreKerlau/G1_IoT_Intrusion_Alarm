#include "security_animation.h"
#include <Arduino.h>
#include <ChainableLED.h>
#include <TM1637.h>

int       successAnimationStep    = 0;
bool      successAnimationState   = true; // Whether we are currently in the "on" or "off" state of the blinking animation
const int SUCCESS_ANIMATION_STEPS = 7;    // Number of steps in the success animation (number of times to blink the combination)
const int SUCCESS_ANIMATION_DELAY = 150;  // Duration to display the success animation in milliseconds

unsigned long lastSuccessAnimationTime = 0;

void startSuccessAnimation() {
  successAnimationStep     = 0;
  successAnimationState    = true; // Start with the combination visible
  lastSuccessAnimationTime = millis();
}

/**
 * This is called when the correct combination is entered.
 * Blink the display with the combination. Will not block the main loop (still 200ms delay).
 * Make sure to call startSuccessAnimation() before to initialize the animation variables.
 */
void playSuccessAnimation(TM1637& display, ChainableLED& leds, std::array<int, 4> currentCombination) {
  if (successAnimationStep < SUCCESS_ANIMATION_STEPS) {
    if (millis() - lastSuccessAnimationTime > SUCCESS_ANIMATION_DELAY) {
      successAnimationStep++;
      successAnimationState    = !successAnimationState; // Toggle blink state
      lastSuccessAnimationTime = millis();

      if (successAnimationState) {
        display.display(0, currentCombination[0]);
        display.display(1, currentCombination[1]);
        display.display(2, currentCombination[2]);
        display.display(3, currentCombination[3]);
      } else {
        display.clearDisplay();
      }
    }
  }
}

/**
 * Blink the LED and display an error pattern "Err" on the screen a few times.
 * This is called when the wrong combination is entered.
 */
void playErrorAnimation(TM1637& display, ChainableLED& leds) {
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
