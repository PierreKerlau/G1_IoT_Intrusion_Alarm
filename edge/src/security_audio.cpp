#include <Arduino.h>
#include <security_audio.h>

#define USE_SOUND

void playPressBeep(int buzzerPin) {
#ifdef USE_SOUND
  tone(buzzerPin, 2500, 30);
#endif
}

void playGoodCombinationSound(int buzzerPin) {
#ifdef USE_SOUND
  tone(buzzerPin, 1000, 100);
  delay(100);
  tone(buzzerPin, 2000, 150);
  delay(150);
#endif
}

void playWrongCombinationSound(int buzzerPin) {
#ifdef USE_SOUND
  tone(buzzerPin, 150, 400);
  delay(400);
#endif
}
