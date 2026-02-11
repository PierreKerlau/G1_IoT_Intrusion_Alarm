#include "main.h"
#include <Arduino.h>

/*
Debugging project to directly modify the EEPROM values for the alarm system
(secret combination and time range rules) for debugging purposes.

After running this code once to set the desired values in EEPROM,
the main alarm system code will be started in CONFIGURATION mode.
*/

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(100); // Wait for serial port to connect.
  delay(3000);  // DEBUG: Wait a moment before starting the system

  Serial.println(F("--- Initialising EEPROM writer ---"));

  // Set an incorrect secret combination for testing
  std::array<int, 4> secretCombination = {1, 2, 3, 4};
  Serial.println("Setting secret combination to {" + String(secretCombination[0]) + ", " + String(secretCombination[1]) + ", " + String(secretCombination[2]) + ", " + String(secretCombination[3]) + "} for testing purposes");
  storeSecretCombinationEEPROM(secretCombination);

  // Reset time range rule count to 0 (no active time ranges)
  size_t        ruleCount = 1;
  TimeRangeRule rules[1];
  rules[0] = {
      .weekDayMask  = 0b1111111,                         // Active all days of the week
      .hourMask     = 0b111111111111111111111111,        // Active all hours of the day
      .monthDayMask = 0b1111111111111111111111111111111, // Active all days of the month
      .monthMask    = 0b111111111111,                    // Active all months of the year
  };
  storeTimeRangeRulesEEPROM(rules, ruleCount);

  Serial.println(F("--- EEPROM writer setup complete ---"));
  Serial.println(F("--- Reading written EEPROM data ---"));

  EEPROMSetupData setupData = setupEEPROM();
  Serial.print(F("Retrieved secret combination from EEPROM: {"));
  for (size_t i = 0; i < setupData.secretCombination.size(); i++) {
    Serial.print(setupData.secretCombination[i]);
    if (i < setupData.secretCombination.size() - 1) {
      Serial.print(F(", "));
    }
  }
  Serial.println(F("}"));

  Serial.print(F("Number of time range rules stored in EEPROM: "));
  Serial.println(setupData.timeRangeRulesCount);

  Serial.println(F("--- End of EEPROM writer setup ---"));
}

void loop() {}
