#ifndef SECURITY_ANIMATION_H
#define SECURITY_ANIMATION_H

#include <ChainableLED.h>
#include <TM1637.h>

void playSuccessAnimation(TM1637& display, ChainableLED& leds, int secretCode[4]);
void playErrorAnimation(TM1637& display, ChainableLED& leds);

#endif
