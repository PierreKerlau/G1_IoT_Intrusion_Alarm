#ifndef SECURITY_AUDIO_H
#define SECURITY_AUDIO_H

#include <Arduino.h>

void playPressBeep(int buzzerPin);
void playGoodCombinationSound(int buzzerPin);
void playWrongCombinationSound(int buzzerPin);
void playMotionSound(int buzzerPin);
void playAlarmSound(int buzzerPin);
void playAlarmTimeoutSound(int buzzerPin);

#endif // SECURITY_AUDIO_H
