#include "security_code.h"

TM1637 tm1637(6, 7);

const int BUTTON_BLUE_PIN  = 2;
const int BUTTON_WHITE_PIN = 3;
const int BUTTON_RED_PIN   = 4;
const int BUTTON_GREEN_PIN = 5;

const int BUZZER_PIN = 11;

// Led color definitions and variables
#define NUM_LEDS                1
#define LED_SATURATION          1
#define LED_BRIGHTNESS          0.2
#define LED_BRIGHTNESS_INACTIVE 0    // LED off
#define LED_HUE_MONITORING      0.65 // Blue
#define LED_HUE_TRIGGERED       0.14 // Yellow
#define LED_HUE_DISARMED        0.3  // Green
#define LED_HUE_FAILED_DISARM   0    // Red
#define LED_HUE_CONFIGURATION   0.75 // Purple
const int    LED_CLK_PIN  = 8;
const int    LED_DATA_PIN = 9;
ChainableLED leds(LED_CLK_PIN, LED_DATA_PIN, NUM_LEDS);

// TODO: Add a way to change the secret combination from LoRaWAN
// Combination and cursor position variables
std::array<int, 4> currentCombination  = {0, 0, 0, 0};     // Current combination entered by the user (default to 0000)
std::array<int, 4> expectedCombination = {-1, -1, -1, -1}; // Correct combination to disarm the system. Initialize with -1 to indicate that it has not been set yet.
int                cursorPosition      = 0;                // Position of the cursor (0 to 3)

// Blinking effect variables
unsigned long lastTimeBlink    = 0;
bool          numberLightBlink = true; // Set to true to hide the current number of the cursor for a blinking effect
const int     BLINKING_SPEED   = 400;  // Blinking speed in milliseconds

const int MAX_TRIES = 3; // Maximum number of tries before triggering the alarm
int       tries     = 0; // Current number of tries

AlarmState alarmState = AlarmState::INACTIVE; // Current state of the alarm system

unsigned long alarmStartTime            = 0; // Time when the alarm was first triggered
unsigned long alarmSuccessfulDisarmTime = 0; // Time when the alarm was successfully disarmed

const unsigned long MAX_DISARM_TIME                 = 15 * 1000; // Maximum time to disarm the system in milliseconds
const unsigned long ALARM_SUCCESSFUL_DISARM_TIMEOUT = 10 * 1000; // Maximum time before resetting the system after a successful disarm in milliseconds
const unsigned long ALARM_TIMEOUT                   = 60 * 1000; // Maximum time for an alarm in milliseconds

int releasePin = -1; // Pin to check for release after button press, -1 if not waiting for any button release

void clearScreen();
void updateScreen();
void updateLedColor();
void handleButtons();
void resetBlinking();
void checkCombination();

void setupSecurity() {
  pinMode(BUTTON_BLUE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_WHITE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_GREEN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  updateLedColor();

  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.clearDisplay();

  setupMotion();

  EEPROMSetupData eepromData = setupEEPROM();
  expectedCombination        = eepromData.secretCombination;
  bool validEepromData       = true;
  for (int i = 0; i < 4; i++) {
    int value = eepromData.secretCombination[i];
    if (value < 0 || value > 9) {
      Serial.print("Error: Invalid secret combination digit retrieved from EEPROM at index ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(expectedCombination[i]);
      validEepromData = false;
      break;
    }
  }
  if (validEepromData && eepromData.timeRangeRulesCount == 0) {
    Serial.print("Error: Invalid time range rules count retrieved from EEPROM: ");
    Serial.println(eepromData.timeRangeRulesCount);
    validEepromData = false;
  }

  if (!validEepromData) {
    Serial.println("Error: Invalid data retrieved from EEPROM. Switching to CONFIGURATION state.");
    setupRTC(nullptr, 0); // Setup RTC with no time range rules, effectively disabling time-based monitoring until valid configuration is set
    setAlarmState(AlarmState::CONFIGURATION);
  } else { // Valid data retrieved from EEPROM, proceed with normal setup
    size_t         ruleCount = eepromData.timeRangeRulesCount;
    TimeRangeRule* rules     = new TimeRangeRule[ruleCount];

    Serial.println("Secret combination retrieved from EEPROM: " + String(eepromData.secretCombination[0]) + String(eepromData.secretCombination[1]) + String(eepromData.secretCombination[2]) + String(eepromData.secretCombination[3]));
    Serial.println("Number of time range rules retrieved from EEPROM: " + String(eepromData.timeRangeRulesCount));

    retrieveTimeRangeRulesEEPROM(rules, ruleCount);
    for (size_t i = 0; i < ruleCount; i++) {
      Serial.println("Time Range Rule " + String(i) + ": weekDayMask=" + String(rules[i].weekDayMask, BIN) + ", hourMask=" + String(rules[i].hourMask, BIN) + ", monthDayMask=" + String(rules[i].monthDayMask, BIN) + ", monthMask=" + String(rules[i].monthMask, BIN));
    }

    setupRTC(rules, ruleCount); // Setup RTC with the retrieved time range rules to enable time-based monitoring
    delete[] rules;             // Free the dynamically allocated memory for time range rules after setting up RTC
  }
}

// --- MAIN LOGIC ---

/**
 * Main logic function for the security system. Its behavior depends on the current state of the alarm system:
 * - If the system is INACTIVE,      it checks if the current time is within the monitoring time ranges and transitions to MONITORING if it is.
 * - If the system is MONITORING,    it checks if the current time is still within the monitoring time ranges and transitions back to INACTIVE if it isn't. It also checks for motion detection and transitions to TRIGGERED if motion is detected.
 * - If the system is TRIGGERED,     it checks if the user has exceeded the maximum number of tries or the maximum disarm time and transitions to FAILED_DISARM if either condition is met. It also handles user input for disarming the system.
 * - If the system is DISARMED,      it plays a non-blocking success animation and checks if the system should be reset after a successful disarm.
 * - If the system is FAILED_DISARM, it checks if the system should be reset after an alarm timeout.
 * - If the system is CONFIGURATION, it will eventually handle configuration tasks (e.g., setting time, changing combination, etc.) which are not implemented yet.
 * The function also listens for incoming LoRa payloads and processes them if received, allowing for remote commands and configuration updates via LoRaWAN.
 * @return The current state of the alarm system after running the logic
 */
AlarmState runSecurityLogic() {
  LoraPayload pkt = listenForPayload();
  if (pkt.id != 0) { // Valid packet received, handle it
    Serial.println("Valid LoRa packet received");
    // TODO: Process received LoRa commands/configuration updates
  }

  if (alarmState == AlarmState::INACTIVE) {
    if (isMonitoringTime()) {
      setAlarmState(AlarmState::MONITORING);
    }
  } else if (alarmState == AlarmState::MONITORING) {
    if (!isMonitoringTime()) {
      setAlarmState(AlarmState::INACTIVE);
    } else if (checkMotion()) { // Motion detected, trigger alarm
      playMotionSound(BUZZER_PIN);
      setAlarmState(AlarmState::TRIGGERED);
      alarmStartTime = millis(); // Start the disarm timer
    }
  } else if (alarmState == AlarmState::TRIGGERED) {
    // Check if the user has exceeded the maximum number of tries or disarm time
    if ((tries >= MAX_TRIES) || (millis() - alarmStartTime > MAX_DISARM_TIME)) {
      // Too late to disarm, alarm was triggered
      // TODO: play alarm sound and animation + send alert via LoRaWAN
      clearScreen();
      setAlarmState(AlarmState::FAILED_DISARM);
      return alarmState;
    }

    if (isWaitingForRelease()) {
      // If we're waiting for a button release, do not handle button presses or blinking effect
      return alarmState;
    }

    if (millis() - lastTimeBlink > BLINKING_SPEED) { // Handle blinking effect for the current digit
      numberLightBlink = !numberLightBlink;
      lastTimeBlink    = millis();
      updateScreen();
    }

    handleButtons();
  } else if (alarmState == AlarmState::DISARMED) {
    // Non-blocking animation (200ms delay), will stop automatically after a few blinks defined by SUCCESS_ANIMATION_STEPS
    playSuccessAnimation(tm1637, leds, currentCombination);

    // Check if we should reset the system after a successful disarm
    if (millis() - alarmSuccessfulDisarmTime > ALARM_SUCCESSFUL_DISARM_TIMEOUT) {
      // Reset the system
      Serial.println("Resetting system after successful disarm...");
      setAlarmState(AlarmState::INACTIVE);
    }
  } else if (alarmState == AlarmState::FAILED_DISARM) {
    // Check if we should reset the system after alarm timeout
    if (millis() - alarmStartTime > ALARM_TIMEOUT) {
      // Reset the system
      Serial.println("Resetting system after alarm timeout...");
      playAlarmTimeoutSound(BUZZER_PIN);
      setAlarmState(AlarmState::INACTIVE);
    }
  } else if (alarmState == AlarmState::CONFIGURATION) {
    // TODO: Configure the system (e.g., set time, change combination, etc.)
  }
  return alarmState;
}

// --- HANDLING BUTTONS ---
void handleButtons() {
  // Button + / Up (Blue)
  if (digitalRead(BUTTON_BLUE_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    currentCombination[cursorPosition]++;
    if (currentCombination[cursorPosition] > 9) currentCombination[cursorPosition] = 0;
    resetBlinking();
    releasePin = BUTTON_BLUE_PIN;
  }

  // Button - / Down (White)
  if (digitalRead(BUTTON_WHITE_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    currentCombination[cursorPosition]--;
    if (currentCombination[cursorPosition] < 0) currentCombination[cursorPosition] = 9;
    resetBlinking();
    releasePin = BUTTON_WHITE_PIN;
  }

  // Button OK (Green)
  if (digitalRead(BUTTON_GREEN_PIN) == LOW) {
    if (cursorPosition < 3) {
      playPressBeep(BUZZER_PIN);
      cursorPosition++;
      resetBlinking();
    } else {
      checkCombination();
    }
    releasePin = BUTTON_GREEN_PIN;
  }

  // Button Previous (Red)
  if (digitalRead(BUTTON_RED_PIN) == LOW) {
    playPressBeep(BUZZER_PIN);
    if (cursorPosition > 0) {
      cursorPosition--;
    }
    resetBlinking();
    releasePin = BUTTON_RED_PIN;
  }
}

// --- DISPLAYING ---
void clearScreen() {
  tm1637.clearDisplay();
}

/**
 * Display the current combination on the screen, with a blinking effect on the current digit to indicate the cursor position when the alarm is triggered.
 * If numberLightBlink is false, display nothing (blank) for the current digit to create the blinking effect.
 */
void updateScreen() {
  if (alarmState == AlarmState::TRIGGERED) {
    for (int i = 0; i < 4; i++) {
      if (i == cursorPosition && numberLightBlink) {
        tm1637.display(i, 0x7f); // 0x7f is the code for displaying nothing (blank) on TM1637
      } else {
        tm1637.display(i, currentCombination[i]);
      }
    }
  }
}

void resetBlinking() {
  numberLightBlink = false;
  lastTimeBlink    = millis();
  updateScreen();
}

// --- CHECKING COMBINATION ---

/**
 * Check if the current combination entered by the user is correct.
 * If it is correct, disarm the system.
 * If it is wrong, increment the tries count and trigger the alarm if the maximum number of tries is reached.
 */
void checkCombination() {
  bool isCorrectCombination = true;
  for (int i = 0; i < 4; i++) {
    if (currentCombination[i] != expectedCombination[i]) {
      isCorrectCombination = false;
      break;
    }
  }
  cursorPosition = 0;

  if (isCorrectCombination) {
    setAlarmState(AlarmState::DISARMED);
  } else {
    currentCombination = {0, 0, 0, 0};
    tries++;
    Serial.println("WRONG CODE - Attempt " + String(tries) + " of " + String(MAX_TRIES));
    if (tries >= MAX_TRIES) { // Final attempt failed, trigger alarm
      Serial.println("DISARMING FAILED - TOO MANY ATTEMPTS");
      setAlarmState(AlarmState::FAILED_DISARM);
    } else { // Not the final attempt
      playWrongCombinationSound(BUZZER_PIN);
      playErrorAnimation(tm1637, leds);
      resetBlinking();
    }
  }
}

/**
 * Check if we're currently waiting for a button release after a button press.
 * @return true if we're waiting for a button release, false otherwise
 */
bool isWaitingForRelease() {
  if (releasePin != -1 && digitalRead(releasePin) == HIGH) {
    releasePin = -1; // Reset release pin after successful release
  }
  return releasePin != -1;
}

/**
 * Update the LED color based on the current alarm state.
 * This function is called whenever the alarm state changes to reflect the new state with the appropriate LED color.
 */
void updateLedColor() {
  if (alarmState == AlarmState::INACTIVE) {
    leds.setColorHSB(0, 0, LED_SATURATION, LED_BRIGHTNESS_INACTIVE); // Inactive state, LED off
  } else {
    float hue;

    switch (alarmState) {
    case AlarmState::MONITORING:    hue = LED_HUE_MONITORING; break;
    case AlarmState::TRIGGERED:     hue = LED_HUE_TRIGGERED; break;
    case AlarmState::DISARMED:      hue = LED_HUE_DISARMED; break;
    case AlarmState::FAILED_DISARM: hue = LED_HUE_FAILED_DISARM; break;
    case AlarmState::CONFIGURATION: hue = LED_HUE_CONFIGURATION; break;
    }

    leds.setColorHSB(0, hue, LED_SATURATION, LED_BRIGHTNESS);
  }
}

AlarmState getAlarmState() {
  return alarmState;
}

// --- ALARM STATE MANAGEMENT ---

/**
 * Set the alarm state and perform actions based on the new state, such as updating the LED color, playing sounds, and resetting timers.
 * This function centralizes all side effects related to changing the alarm state to ensure consistent behavior across the system.
 * @param newState The new state to set for the alarm system
 */
void setAlarmState(AlarmState newState) {
  if (alarmState == newState) return; // No state change, do nothing

  AlarmState previousState = alarmState; // Temporarily store the previous state
  alarmState               = newState;   // Update state

  Serial.print("Alarm state changed: ");
  Serial.println(alarmStateToString(previousState) + " -> " + alarmStateToString(alarmState));

  // TODO: Add LoRaWAN messages for state changes, especially for TRIGGERED, DISARMED and FAILED_DISARM states

  // Handle actions on state change
  if (alarmState == AlarmState::INACTIVE) {
    updateLedColor();
    clearScreen();
  } else if (alarmState == AlarmState::MONITORING) {
    updateLedColor();
    clearScreen();
  } else if (alarmState == AlarmState::TRIGGERED) {
    tries              = 0; // Reset tries count
    cursorPosition     = 0;
    currentCombination = {0, 0, 0, 0};
    releasePin         = -1; // Reset release pin

    alarmStartTime = millis(); // Start the disarm timer

    updateLedColor();
    resetBlinking();             // Reset blinking effect and update the screen to show the initial state
    playMotionSound(BUZZER_PIN); // Play motion detected sound when alarm state starts
  } else if (alarmState == AlarmState::DISARMED) {
    updateLedColor();
    alarmSuccessfulDisarmTime = millis(); // Start the timer to reset the system after a successful disarm
    // TODO: Send LoRaWAN message to notify successful disarm
    playGoodCombinationSound(BUZZER_PIN);
    resetBlinking();
    startSuccessAnimation(); // Initialize success animation variables
  } else if (alarmState == AlarmState::FAILED_DISARM) {
    updateLedColor();
    playAlarmSound(BUZZER_PIN);
    playErrorAnimation(tm1637, leds);
    resetBlinking();
  } else if (alarmState == AlarmState::CONFIGURATION) {
    updateLedColor();
    // TODO: Handle configuration mode
  }
}

/**
 * Helper function to convert AlarmState enum to a human-readable string for logging purposes.
 */
String alarmStateToString(AlarmState state) {
  switch (state) {
  case AlarmState::INACTIVE:
    return "INACTIVE";
  case AlarmState::MONITORING:
    return "MONITORING";
  case AlarmState::TRIGGERED:
    return "TRIGGERED";
  case AlarmState::DISARMED:
    return "DISARMED";
  case AlarmState::FAILED_DISARM:
    return "FAILED_DISARM";
  case AlarmState::CONFIGURATION:
    return "CONFIGURATION";
  default:
    return "UNKNOWN";
  }
}
