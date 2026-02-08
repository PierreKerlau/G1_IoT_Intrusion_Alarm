#ifndef SECURITY_ANIMATION_H
#define SECURITY_ANIMATION_H

#include <ChainableLED.h>
#include <TM1637.h>
#include <array>

void startSuccessAnimation();
void playSuccessAnimation(TM1637& display, ChainableLED& leds, std::array<int, 4> currentCombination);
void playErrorAnimation(TM1637& display, ChainableLED& leds);

#endif // SECURITY_ANIMATION_H
