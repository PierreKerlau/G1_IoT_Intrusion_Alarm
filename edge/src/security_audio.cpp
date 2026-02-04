#include <Arduino.h>
#include <security_audio.h>

void playPressBeep(int buzzerPin) {
  tone(buzzerPin, 2500, 30);
}

void playGoodCombinationSound(int buzzerPin) {
  tone(buzzerPin, 1000, 100); 
  delay(100);
  tone(buzzerPin, 2000, 150); 
  delay(150);
}

void playWrongCombinationSound(int buzzerPin) {
  tone(buzzerPin, 150, 400); 
  delay(400);
}

