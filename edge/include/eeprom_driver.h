#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include "time_range.h"
#include <EEPROM.h>
#include <array>

/*
EEPROM storage layout (starting at address 0):
- 0-3: Secret combination (4 bytes, each byte represents a digit from 0 to 9)
- 100: Number of time range rules (1 byte, max 255 rules)
- 101-...: Time range rules (16 bytes each, defined by the TimeRangeRule structure)
*/
#define EEPROM_SECRET_COMBINATION_ADDRESS     0
#define EEPROM_TIME_RANGE_RULES_COUNT_ADDRESS 100
#define EEPROM_TIME_RANGE_RULES_START_ADDRESS 101

/**
 * Structure to hold the data retrieved from EEPROM at startup, including the secret combination and the number of time range rules stored.
 */
struct EEPROMSetupData {
  std::array<int, 4> secretCombination;
  uint8_t            timeRangeRulesCount; // Number of time range rules stored in EEPROM
};

EEPROMSetupData setupEEPROM(); // Retrieve the secret combination and number of time range rules from EEPROM at startup

void retrieveTimeRangeRulesEEPROM(TimeRangeRule* rules, size_t& ruleCount);
void storeTimeRangeRulesEEPROM(TimeRangeRule* rules, size_t& ruleCount);
void storeSecretCombinationEEPROM(const std::array<int, 4>& combination);

#endif // EEPROM_DRIVER_H