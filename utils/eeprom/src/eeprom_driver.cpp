#include "eeprom_driver.h"

/**
 * Retrieve the secret combination and number of time range rules from EEPROM at startup.
 * @return An EEPROMSetupData structure containing the secret combination and the number of time range rules stored in EEPROM.
 * @note The secret combination is not validated here, so it may contain invalid values (e.g., digits outside the range 0-9). The caller should validate the retrieved combination before using it.
 */
EEPROMSetupData setupEEPROM() {
  EEPROMSetupData data;

  // Read secret combination
  for (int i = 0; i < 4; i++) {
    uint8_t value             = EEPROM.read(EEPROM_SECRET_COMBINATION_ADDRESS + i);
    data.secretCombination[i] = value;

    Serial.print("Read secret combination digit from EEPROM: ");
    Serial.println(data.secretCombination[i]);
  }

  // Read number of time range rules
  data.timeRangeRulesCount = EEPROM.read(EEPROM_TIME_RANGE_RULES_COUNT_ADDRESS);
  Serial.print("Read time range rules count from EEPROM: ");
  Serial.println(data.timeRangeRulesCount);

  return data;
}

/**
 * Retrieve time range rules from EEPROM and store them in the provided rules array.
 * @param rules Pointer to an array of TimeRangeRule where the retrieved rules will be stored.
 * @param ruleCount The maximum number of rules that can be stored in the provided rules array.
 * @return true if successful, false otherwise.
 */
void retrieveTimeRangeRulesEEPROM(TimeRangeRule* rules, size_t& ruleCount) {
  // Check if the provided ruleCount is sufficient to hold the number of rules in the pointer
  if (ruleCount > 255) {
    Serial.print("Error: Provided ruleCount is larger than the maximum number of rules (255).");
    ruleCount = 255; // Adjust ruleCount to the maximum allowed
  }

  // Read number of time range rules stored in EEPROM
  uint8_t storedRuleCount = EEPROM.read(EEPROM_TIME_RANGE_RULES_COUNT_ADDRESS);

  if (storedRuleCount > ruleCount) {
    Serial.println("Warning: Provided ruleCount (" + String(ruleCount) + ") does not match the number of rules stored in EEPROM (" + String(storedRuleCount) + "). Only the first " + String(ruleCount) + " rules will be retrieved.");
  } else if (storedRuleCount < ruleCount) {
    ruleCount = storedRuleCount; // Adjust ruleCount to the actual number of rules stored in EEPROM
    Serial.println("Warning: Provided ruleCount (" + String(ruleCount) + ") is larger than the number of rules stored in EEPROM (" + String(storedRuleCount) + "). Only the first " + String(storedRuleCount) + " rules will be retrieved, the rest of the rules array will not be modified.");
  }

  for (size_t i = 0; i < ruleCount; i++) {
    // Calculate the base address for the current rule
    int baseAddress = EEPROM_TIME_RANGE_RULES_START_ADDRESS + i * TIME_RANGE_RULE_BYTES;

    // Read each byte of the TimeRangeRule structure from EEPROM and reconstruct the structure
    rules[i].weekDayMask  = EEPROM.read(baseAddress);
    rules[i].hourMask     = (EEPROM.read(baseAddress + 1) << 24) | (EEPROM.read(baseAddress + 2) << 16) | (EEPROM.read(baseAddress + 3) << 8) | EEPROM.read(baseAddress + 4);
    rules[i].monthDayMask = (EEPROM.read(baseAddress + 5) << 24) | (EEPROM.read(baseAddress + 6) << 16) | (EEPROM.read(baseAddress + 7) << 8) | EEPROM.read(baseAddress + 8);
    rules[i].monthMask    = (EEPROM.read(baseAddress + 9) << 8) | EEPROM.read(baseAddress + 10);

    Serial.print("Retrieved time range rule from EEPROM: weekDayMask=");
    Serial.print(rules[i].weekDayMask, BIN);
    Serial.print(", hourMask=");
    Serial.print(rules[i].hourMask, BIN);
    Serial.print(", monthDayMask=");
    Serial.print(rules[i].monthDayMask, BIN);
    Serial.print(", monthMask=");
    Serial.println(rules[i].monthMask, BIN);
  }
}

/**
 * Store the time range rules and rule count in EEPROM memory.
 * @param rules Pointer to an array of TimeRangeRule to be stored.
 * @param ruleCount The number of rules to store.
 */
void storeTimeRangeRulesEEPROM(TimeRangeRule* rules, size_t& ruleCount) {
  if (ruleCount > 255) {
    Serial.print("Error: Cannot store more than 255 time range rules in EEPROM.");
    ruleCount = 255; // Adjust ruleCount to the maximum allowed
  }

  // Store the number of time range rules in EEPROM
  EEPROM.write(EEPROM_TIME_RANGE_RULES_COUNT_ADDRESS, ruleCount);

  for (size_t i = 0; i < ruleCount; i++) {
    int baseAddress = EEPROM_TIME_RANGE_RULES_START_ADDRESS + i * TIME_RANGE_RULE_BYTES;

    // Store each byte of the TimeRangeRule structure in EEPROM
    EEPROM.write(baseAddress, rules[i].weekDayMask);
    EEPROM.write(baseAddress + 1, (rules[i].hourMask >> 24) & 0xFF);
    EEPROM.write(baseAddress + 2, (rules[i].hourMask >> 16) & 0xFF);
    EEPROM.write(baseAddress + 3, (rules[i].hourMask >> 8) & 0xFF);
    EEPROM.write(baseAddress + 4, rules[i].hourMask & 0xFF);
    EEPROM.write(baseAddress + 5, (rules[i].monthDayMask >> 24) & 0xFF);
    EEPROM.write(baseAddress + 6, (rules[i].monthDayMask >> 16) & 0xFF);
    EEPROM.write(baseAddress + 7, (rules[i].monthDayMask >> 8) & 0xFF);
    EEPROM.write(baseAddress + 8, rules[i].monthDayMask & 0xFF);
    EEPROM.write(baseAddress + 9, (rules[i].monthMask >> 8) & 0xFF);
    EEPROM.write(baseAddress + 10, rules[i].monthMask & 0xFF);

    Serial.print("Stored time range rule in EEPROM: weekDayMask=");
    Serial.print(rules[i].weekDayMask, BIN);
    Serial.print(", hourMask=");
    Serial.print(rules[i].hourMask, BIN);
    Serial.print(", monthDayMask=");
    Serial.print(rules[i].monthDayMask, BIN);
    Serial.print(", monthMask=");
    Serial.println(rules[i].monthMask, BIN);
  }
}

/**
 * Store the secret combination in EEPROM.
 * @param combination An array of 4 integers representing the secret combination digits
 * @note Does not check whether the combination is valid. Setting an invalid combination allows starting the alarm in configuration mode.
 */
void storeSecretCombinationEEPROM(const std::array<int, 4>& combination) {
  for (int i = 0; i < 4; i++) {
    uint8_t value = combination[i];
    EEPROM.write(EEPROM_SECRET_COMBINATION_ADDRESS + i, value);

    Serial.print("Stored secret combination digit in EEPROM: ");
    Serial.println(value);
  }
}
