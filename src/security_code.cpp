#include <Arduino.h>
#include <TM1637.h>
#include "security_code.h"

const int CLK = 6;
const int DIO = 7;
TM1637 tm1637(CLK, DIO);

const int BLUE_PIN = 2;   
const int WHITE_PIN = 3;  
const int RED_PIN = 4;    
const int GREEN_PIN = 5;  
const int BUZZER_PIN = 11;

int goodCombination[4] = {1, 2, 3, 4}; 
int secretCode[4] = {0, 0, 0, 0}; 
int CursorPosition = 0;

unsigned long LastTimeBlink = 0;
bool numberLight = true;
const int BLINKING_SPEED = 400;

bool systemDisarmed = false;

void updateScreen();
void handleButtons();
void pressBeepSound();
void goodCombinationSound();
void wrongCombinationSound();
void resetBlinking();
void codeVerification();
void waitingRelease(int pin);
void sucessAnimation();
void errorAnimation();

void setupSecurity() {
  pinMode(BLUE_PIN, INPUT_PULLUP);
  pinMode(WHITE_PIN, INPUT_PULLUP);
  pinMode(GREEN_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.clearDisplay(); 
}

void resetAlarmState() {
  systemDisarmed = false;
  CursorPosition = 0;
  secretCode[0]=0; secretCode[1]=0; secretCode[2]=0; secretCode[3]=0;
  resetBlinking(); 
}

// --- LOGIQUE ---
bool runSecurityLogic() {
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
    pressBeepSound();
    secretCode[CursorPosition]++;
    if (secretCode[CursorPosition] > 9) secretCode[CursorPosition] = 0;
    resetBlinking();
    waitingRelease(BLUE_PIN);
  }
  
  if (digitalRead(WHITE_PIN) == LOW) {
    pressBeepSound();
    secretCode[CursorPosition]--;
    if (secretCode[CursorPosition] < 0) secretCode[CursorPosition] = 9;
    resetBlinking();
    waitingRelease(WHITE_PIN);
  }
  
  if (digitalRead(GREEN_PIN) == LOW) {
    if (CursorPosition < 3) {
      pressBeepSound();
      CursorPosition++;
      resetBlinking();
    } else {
      codeVerification();
    }
    waitingRelease(GREEN_PIN);
  }
  
  if (digitalRead(RED_PIN) == LOW) {
    pressBeepSound();
    if (CursorPosition > 0) {
      CursorPosition--;
    }
    resetBlinking();
    waitingRelease(RED_PIN);
  }
}

// --- AUDIO ---
void pressBeepSound() {
  tone(BUZZER_PIN, 2500, 30);
}

void goodCombinationSound() {
  tone(BUZZER_PIN, 1000, 100); 
  delay(100);
  tone(BUZZER_PIN, 2000, 150); 
  delay(150);
}

void wrongCombinationSound() {
  tone(BUZZER_PIN, 150, 400); 
  delay(400);
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
    goodCombinationSound();
    sucessAnimation();
    
    systemDisarmed = true;
    tm1637.clearDisplay();
  } else {
    Serial.println("CODE FAUX");
    wrongCombinationSound();
    errorAnimation();
    resetBlinking();
  }
}

void waitingRelease(int pin) {
  delay(50);
  while (digitalRead(pin) == LOW) {} 
  delay(50);
}

void sucessAnimation() {
  for(int i=0; i<3; i++) {
    tm1637.clearDisplay(); delay(200);
    tm1637.display(0, secretCode[0]); tm1637.display(1, secretCode[1]);
    tm1637.display(2, secretCode[2]); tm1637.display(3, secretCode[3]);
    delay(200);
  }
}

void errorAnimation() {
  for(int i=0; i<4; i++) {
    tm1637.display(0,0); tm1637.display(1,0); tm1637.display(2,0); tm1637.display(3,0);
    delay(100);
    tm1637.clearDisplay();
    delay(100);
  }
}