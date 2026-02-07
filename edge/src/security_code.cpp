#include <Arduino.h>
#include <ChainableLED.h>
#include <TM1637.h>
#include <array>
#include <security_animation.h>
#include <security_audio.h>
#include <security_code.h>

TM1637 tm1637(6, 7);

const int BUTTON_BLUE_PIN  = 2;
const int BUTTON_WHITE_PIN = 3;
const int BUTTON_RED_PIN   = 4;
const int BUTTON_GREEN_PIN = 5;

const int BUZZER_PIN = 11;

#define NUM_LEDS 1
const int    LED_CLK_PIN  = 8;
const int    LED_DATA_PIN = 9;
ChainableLED leds(LED_CLK_PIN, LED_DATA_PIN, NUM_LEDS);

// TODO: Retrieve the secret combination in EEPROM
// TODO: If no combination is set in EEPROM, generate a random one at startup and display it for a few seconds before clearing the screen and starting the system
// TODO: Add a way to change the secret combination from LoRaWAN
// Combination and cursor position variables
std::array<int, 4> currentCombination  = {0, 0, 0, 0}; // Current combination entered by the user (default to 0000)
std::array<int, 4> expectedCombination = {1, 2, 3, 4}; // Correct combination to disarm the system (default to 1234)
int                cursorPosition      = 0;            // Position of the cursor (0 to 3)

// Blinking effect variables
unsigned long lastTimeBlink  = 0;
bool          numberLight    = true; // Set to true to display the number, false to hide it (display nothing) for blinking effect
const int     BLINKING_SPEED = 400;  // Blinking speed in milliseconds

const int maxTries = 3; // Maximum number of tries before triggering the alarm
int       tries    = 0; // Current number of tries

const unsigned long maxDisarmTime   = 30000; // Maximum time to disarm the system in milliseconds (30 seconds)
unsigned long       disarmStartTime = 0;     // Time when the disarm attempt started

bool systemDisarmed = false;

void updateScreen();
void handleButtons();
void resetBlinking();
void codeVerification();
void waitForRelease(int pin);

void setupSecurity() {
  pinMode(BUTTON_BLUE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_WHITE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_GREEN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.clearDisplay();
  setLedColorHSB(0.65, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "blue" for now
}

void resetAlarmState() {
  systemDisarmed = false;
  cursorPosition = 0;
  // TODO: Make secret code random or configurable
  currentCombination = {0, 0, 0, 0};
  resetBlinking();
  disarmStartTime = millis(); // Start the disarm timer
  tries           = 0;        // Reset tries count
}

// --- LOGIQUE ---
bool runSecurityLogic() {
  if (systemDisarmed) {
    return true;
  } else if ((tries >= maxTries || (millis() - disarmStartTime) > maxDisarmTime) && !systemDisarmed) {
    // Too late to disarm, alarm was triggered
    return false;
  } else {                          // Allow user to keep trying to disarm the system
    setLedColorHSB(0.04, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "orange" for now

    if (millis() - lastTimeBlink > BLINKING_SPEED) { // Handle blinking effect for the current digit
      numberLight   = !numberLight;
      lastTimeBlink = millis();
      updateScreen();
    }

    handleButtons();

    return systemDisarmed;
  }
}

// --- GESTION DES BOUTONS ---
void handleButtons() {
  // Button + / Up (Blue)
  if (digitalRead(BUTTON_BLUE_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    currentCombination[cursorPosition]++;
    if (currentCombination[cursorPosition] > 9) currentCombination[cursorPosition] = 0;
    resetBlinking();
    waitForRelease(BUTTON_BLUE_PIN);
  }

  // Button - / Down (White)
  if (digitalRead(BUTTON_WHITE_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    currentCombination[cursorPosition]--;
    if (currentCombination[cursorPosition] < 0) currentCombination[cursorPosition] = 9;
    resetBlinking();
    waitForRelease(BUTTON_WHITE_PIN);
  }

  // Button OK (Green)
  if (digitalRead(BUTTON_GREEN_PIN) == LOW) {
    if (cursorPosition < 3) {
      playPressBeep(BUZZER_PIN);
      cursorPosition++;
      resetBlinking();
    } else {
      codeVerification();
    }
    waitForRelease(BUTTON_GREEN_PIN);
  }

  // Button Previous (Red)
  if (digitalRead(BUTTON_RED_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    if (cursorPosition > 0) {
      cursorPosition--;
    }
    resetBlinking();
    waitForRelease(BUTTON_RED_PIN);
  }
}

// --- AFFICHAGE ---
void updateScreen() {
  for (int i = 0; i < 4; i++) {
    if (i == cursorPosition && numberLight == false) {
      tm1637.display(i, 0x7f);
    } else {
      tm1637.display(i, currentCombination[i]);
    }
  }
}

void resetBlinking() {
  numberLight   = true;
  lastTimeBlink = millis();
  updateScreen();
}

// --- VERIFICATION ---
void codeVerification() {
  bool isTrue = true;
  for (int i = 0; i < 4; i++) {
    if (currentCombination[i] != expectedCombination[i])
      isTrue = false;
  }

  cursorPosition        = 0;
  currentCombination[0] = 0;
  currentCombination[1] = 0;
  currentCombination[2] = 0;
  currentCombination[3] = 0;

  if (isTrue) {
    Serial.println("CORRECT CODE");
    playGoodCombinationSound(BUZZER_PIN);
    playSuccessAnimation(tm1637, leds, currentCombination);

    systemDisarmed = true;
    tm1637.clearDisplay();
  } else {
    tries++;
    Serial.println("WRONG CODE - ATTEMPT " + String(tries) + " of " + String(maxTries));
    if (tries >= maxTries) {
      Serial.println("DISARMING FAILED - TOO MANY ATTEMPTS");
      playWrongCombinationSound(BUZZER_PIN);
      playErrorAnimation(tm1637, leds);
      resetBlinking();
    } else {
      playAlarmSound(BUZZER_PIN);
      playErrorAnimation(tm1637, leds);
      resetBlinking();
    }
  }
}

void waitForRelease(int pin) {
  // TODO: Do not block the entire system while waiting for button release !
  delay(50);
  while (digitalRead(pin) == LOW) {}
  delay(50);
}

void setLedColorHSB(float hue, float saturation, float brightness) {
  leds.setColorHSB(0, hue, saturation, brightness);
}