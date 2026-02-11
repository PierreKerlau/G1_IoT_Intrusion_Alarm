#include "security_code.h"
#include <optional>

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
void processLoraPayload(const LoraPayload& pkt);
void setExpectedCombinationFromPacket(const LoraPayload& pkt);
void setTimeRulesFromPacket(const LoraPayload& pkt);
void setAlarmStateFromPacket(const LoraPayload& pkt);
void setRTCTimeFromPacket(const LoraPayload& pkt, bool forceUpdate = false);

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
  if (pkt.type != PayloadType::UNKNOWN) { // Valid packet received
    processLoraPayload(pkt);              // Process configuration updates
  }

  if (alarmState == AlarmState::INACTIVE) {
    if (isMonitoringTime()) {
      setAlarmState(AlarmState::MONITORING);
    }
  } else if (alarmState == AlarmState::MONITORING) {
    if (!isMonitoringTime()) {
      setAlarmState(AlarmState::INACTIVE);
    } else if (checkMotion()) { // Motion detected, trigger alarm
      Serial.println("[MOTION] Motion detected, triggering alarm!");
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

// --- LoraWAN PAYLOAD PROCESSING ---
void processLoraPayload(const LoraPayload& pkt) {
  // TODO: Test

  // Update RTC (only force update if payload type is SET_RTC_TIME)
  bool forceTimeUpdate = pkt.type == PayloadType::SET_RTC_TIME;
  setRTCTimeFromPacket(pkt, forceTimeUpdate);

  if (pkt.type == PayloadType::SET_COMBINATION) {
    Serial.println("[LoRa] Received SET_COMBINATION payload");
    setExpectedCombinationFromPacket(pkt);
  } else if (pkt.type == PayloadType::SET_TIME_RANGE) {
    Serial.println("[LoRa] Received SET_TIME_RANGE payload");
    setTimeRulesFromPacket(pkt);
  } else if (pkt.type == PayloadType::SET_ALARM_STATE) {
    Serial.println("[LoRa] Received SET_ALARM_STATE payload");
    setAlarmStateFromPacket(pkt);
  } else if (pkt.type != PayloadType::SET_RTC_TIME) { // Unknown payload
    Serial.println("[LoRa] Received unknown payload type from broker");
  }
}

void setExpectedCombinationFromPacket(const LoraPayload& pkt) {
  // TODO: Test
  if (pkt.length >= 4) { // Combination config: digit1, digit2, digit3, digit4
    std::array<int, 4> newCombination;
    bool               validCombination = true;
    for (int i = 0; i < 4; i++) {
      newCombination[i] = pkt.data[i];
      if (newCombination[i] < 0 || newCombination[i] > 9) {
        Serial.print("[PSWD] Error: Invalid combination digit received in LoRa configuration payload at index ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(newCombination[i]);
        validCombination = false;
        break;
      }
    }
    if (validCombination) {
      storeSecretCombinationEEPROM(newCombination);
      expectedCombination = newCombination;
      // TODO: Remove for security
      Serial.println("[PSWD] Secret combination updated via LoRaWAN to: " + String(expectedCombination[0]) + String(expectedCombination[1]) + String(expectedCombination[2]) + String(expectedCombination[3]));
    }
  }
}

void setTimeRulesFromPacket(const LoraPayload& pkt) {
  // TODO: Test
  size_t ruleCount = (pkt.length - (pkt.length % sizeof(TimeRangeRule))) / sizeof(TimeRangeRule);
  if (ruleCount * sizeof(TimeRangeRule) > MAX_PAYLOAD_DATA_SIZE) {
    Serial.println("[SET_RULES] Error: Rule count is higher than the maximum possible data size. Length (bytes)=" + String(ruleCount * sizeof(TimeRangeRule)) + ", MAX_PAYLOAD_DATA_SIZE=" + String(MAX_PAYLOAD_DATA_SIZE));
    return; // Do not set any rules
  }
  if (ruleCount == 0 || pkt.length % sizeof(TimeRangeRule) != 0) {
    Serial.println("[SET_RULES] Warning: Payload length is not a multiple of TimeRangeRule size. Length=" + String(pkt.length) + ", sizeof(TimeRangeRule)=" + String(sizeof(TimeRangeRule)));
  }

  TimeRangeRule rules[ruleCount];
  for (size_t i = 0; i < ruleCount; i++) {
    TimeRangeRule rule = {
        .weekDayMask  = pkt.data[i * sizeof(TimeRangeRule)],
        .hourMask     = (pkt.data[i * sizeof(TimeRangeRule) + 1] << 24) | (pkt.data[i * sizeof(TimeRangeRule) + 2] << 16) | (pkt.data[i * sizeof(TimeRangeRule) + 3] << 8) | pkt.data[i * sizeof(TimeRangeRule) + 4],
        .monthDayMask = (pkt.data[i * sizeof(TimeRangeRule) + 5] << 24) | (pkt.data[i * sizeof(TimeRangeRule) + 6] << 16) | (pkt.data[i * sizeof(TimeRangeRule) + 7] << 8) | pkt.data[i * sizeof(TimeRangeRule) + 8],
        .monthMask    = (pkt.data[i * sizeof(TimeRangeRule) + 9] << 8) | pkt.data[i * sizeof(TimeRangeRule) + 10],
    };

    rules[i] = rule;
  }

  storeTimeRangeRulesEEPROM(rules, ruleCount);
  setTimeRangeRules(rules, ruleCount);
}

void setAlarmStateFromPacket(const LoraPayload& pkt) {
  // TODO: Test
  if (pkt.length >= 1) {
    if (auto newState = parseAlarmState(pkt.data[0])) {
      setAlarmState(*newState);
      Serial.println("[SET_STATE] Alarm state updated via LoRaWAN");
    } else {
      Serial.print("[SET_STATE] Error: Invalid alarm state received: ");
      Serial.println(pkt.data[0]);
    }
  }
}

/**
 * Sets the RTC time from a LoRa payload if the timestamp is valid and optionally checks for a time delay.
 * @param pkt The LoRa payload containing the timestamp.
 * @param forceUpdate Whether to check for a time delay before setting the RTC time. Set to true to force update without delay check.
 */
void setRTCTimeFromPacket(const LoraPayload& pkt, bool forceUpdate) {
  // TODO: Test
  if (pkt.ts > MINIMUM_UNIX_TIME && pkt.ts < MAXIMUM_UNIX_TIME) {
    uint32_t rtcUnixTime = getCurrentUnixTime();
    if (forceUpdate || pkt.ts < rtcUnixTime - MAX_TIME_DELAY || pkt.ts > rtcUnixTime + MAX_TIME_DELAY) {
      Serial.println("[SET_RTC] Updating RTC time from payload");
      String   oldTimeString = getTimeString();
      uint32_t oldTimeUnix   = getCurrentUnixTime();

      setCurrentUnixTime(pkt.ts);
      delay(50);

      uint32_t timeDifference = abs((int64_t)(getCurrentUnixTime()) - oldTimeUnix);
      Serial.println("Time difference: " + String(timeDifference) + " seconds (" + oldTimeString + " -> " + getTimeString() + ")");
    }
  }
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
    float hue = 0;

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

/**
 * Convert a uint8_t potentially representing an AlarmState into its enum value or return nullopt
 * @param raw The raw uint8_t value to convert
 * @return AlarmState correspondig to the raw value or nullopt
 */
std::optional<AlarmState> parseAlarmState(uint8_t raw) {
  switch (raw) {
  case 0:  return AlarmState::INACTIVE;
  case 1:  return AlarmState::MONITORING;
  case 2:  return AlarmState::TRIGGERED;
  case 3:  return AlarmState::DISARMED;
  case 4:  return AlarmState::FAILED_DISARM;
  case 5:  return AlarmState::CONFIGURATION;
  default: return std::nullopt; // Invalid value
  }
}

/**
 * Convert a uint8_t potentially representing an PayloadType into its enum value or return nullopt
 * @param raw The raw uint8_t value to convert
 * @return PayloadType correspondig to the raw value or nullopt
 */
std::optional<PayloadType> parsePayloadType(uint8_t raw) {
  switch (raw) {
  case 0x00: return PayloadType::UNKNOWN;
  case 0x01: return PayloadType::EDGE_HEARTBEAT;
  case 0x02: return PayloadType::MOTION_STATE;
  case 0x11: return PayloadType::SET_COMBINATION;
  case 0x12: return PayloadType::SET_TIME_RANGE;
  case 0x13: return PayloadType::SET_ALARM_STATE;
  case 0x14: return PayloadType::SET_RTC_TIME;
  default:   return std::nullopt; // Invalid value
  }
}
