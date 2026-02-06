#include <Arduino.h>
#include <TM1637.h>
#include <ChainableLED.h>
#include <security_code.h>
#include <security_audio.h>
#include <security_animation.h>

#define NUM_LEDS 1

TM1637 tm1637(6, 7);

const int BLUE_PIN = 2;
const int WHITE_PIN = 3;
const int RED_PIN = 4;
const int GREEN_PIN = 5;
const int BUZZER_PIN = 11;
ChainableLED leds(8, 9, NUM_LEDS);

int goodCombination[4] = {1, 2, 3, 4};
int secretCode[4] = {0, 0, 0, 0};
int CursorPosition = 0;

unsigned long LastTimeBlink = 0;
bool numberLight = true;
const int BLINKING_SPEED = 400;

bool systemDisarmed = false;

void updateScreen();
void handleButtons();
void resetBlinking();
void codeVerification();
void waitingRelease(int pin);

void setupSecurity() {
  pinMode(BLUE_PIN, INPUT_PULLUP);
  pinMode(WHITE_PIN, INPUT_PULLUP);
  pinMode(GREEN_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.clearDisplay();
  leds.setColorHSB(0, 0.0, 0.0, 0.0);
}

void resetAlarmState() {
  systemDisarmed = false;
  CursorPosition = 0;
  // TODO: Make secret code random or configurable
  secretCode[0]=0; secretCode[1]=0; secretCode[2]=0; secretCode[3]=0;
  resetBlinking();
}

// --- LOGIQUE ---
bool runSecurityLogic() {
  leds.setColorHSB(0, 0.04, 1.0, 0.2);
  if (millis() - LastTimeBlink > BLINKING_SPEED) {
    numberLight = !numberLight; 
    LastTimeBlink = millis();
    updateScreen();
  }
  
  handleButtons();

  return systemDisarmed;
}

// --- GESTION DES BOUTONS ---
void handleButtons() {
  if (digitalRead(BLUE_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    secretCode[CursorPosition]++;
    if (secretCode[CursorPosition] > 9) secretCode[CursorPosition] = 0;
    resetBlinking();
    waitingRelease(BLUE_PIN);
  }
  
  if (digitalRead(WHITE_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    secretCode[CursorPosition]--;
    if (secretCode[CursorPosition] < 0) secretCode[CursorPosition] = 9;
    resetBlinking();
    waitingRelease(WHITE_PIN);
  }
  
  if (digitalRead(GREEN_PIN) == LOW) {
    if (CursorPosition < 3) {
      playPressBeep(BUZZER_PIN);
      CursorPosition++;
      resetBlinking();
    } else {
      codeVerification();
    }
    waitingRelease(GREEN_PIN);
  }
  
  if (digitalRead(RED_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    if (CursorPosition > 0) {
      CursorPosition--;
    }
    resetBlinking();
    waitingRelease(RED_PIN);
  }
}

// --- AFFICHAGE ---
void updateScreen() {
  for (int i = 0; i < 4; i++) {
    if (i == CursorPosition && numberLight == false) {
      tm1637.display(i, 0x7f);
    } 
    else {
      tm1637.display(i, secretCode[i]);
    }
  }
}

void resetBlinking() {
  numberLight = true;
  LastTimeBlink = millis();
  updateScreen();
}

// --- VERIFICATION ---
void codeVerification() {
  bool isTrue = true;
  for(int i=0; i<4; i++) {
    if (secretCode[i] != goodCombination[i]) isTrue = false;
  }
  
  CursorPosition = 0;
  secretCode[0]=0; secretCode[1]=0; secretCode[2]=0; secretCode[3]=0;

  if (isTrue) {
    Serial.println("CODE CORRECT");
    playGoodCombinationSound(BUZZER_PIN);
    playSuccessAnimation(tm1637, leds, secretCode);

    systemDisarmed = true;
    tm1637.clearDisplay();
  } else {
    Serial.println("CODE FAUX");
    playWrongCombinationSound(BUZZER_PIN);
    playErrorAnimation(tm1637, leds);
    resetBlinking();
  }
}

void waitingRelease(int pin) {
  delay(50);
  while (digitalRead(pin) == LOW) {} 
  delay(50);
}