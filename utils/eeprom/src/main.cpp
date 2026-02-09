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
  Serial.println(F("Setting secret combination to {255, 255, 255, 255}"));
  std::array<int, 4> secretCombination = {255, 255, 255, 255};
  storeSecretCombinationEEPROM(secretCombination);

  // Reset time range rule count to 0 (no active time ranges)
  size_t ruleCount = 0;
  storeTimeRangeRulesEEPROM(nullptr, ruleCount);

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
