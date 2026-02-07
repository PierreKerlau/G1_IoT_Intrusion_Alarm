#include <Arduino.h>
#include <security_audio.h>

// Comment out the #define below to disable sound effects
#define USE_SOUND
#define ALARM_SOUND
#define ALARM_SOUND_DURATION 2 * 1000 // Duration of the alarm sound in milliseconds

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

void playMotionSound(int buzzerPin) {
#if defined(USE_SOUND)
  tone(buzzerPin, 100, 600);
#endif
}

void playAlarmSound(int buzzerPin) {
#if defined(USE_SOUND) && defined(ALARM_SOUND)
  tone(buzzerPin, 600, ALARM_SOUND_DURATION);
#endif
}
