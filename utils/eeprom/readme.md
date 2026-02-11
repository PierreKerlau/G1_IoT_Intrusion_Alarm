# EEPROM Writer Utility

This is a utility project for debugging and initializing the EEPROM memory of the edge device. It allows you to directly set the secret combination and time range rules that the main alarm system will read at startup.

## Purpose

This utility is used to:
- Set an initial secret combination for testing
- Configure time range rules for monitoring windows
- Verify EEPROM read/write operations
- Reset or update stored configuration without modifying the main alarm code

## EEPROM Memory Layout

The edge device uses the following EEPROM storage layout:

| Address | Size | Content |
|---------|------|---------|
| 0-3 | 4 bytes | Secret combination (4 digits, 0-9 each) |
| 100 | 1 byte | Number of time range rules (max 255) |
| 101+ | 11 bytes each | Time range rules (see `TimeRangeRule`) |

See eeprom_driver.h for address definitions.

## Time Range Rule Structure

Each `TimeRangeRule` occupies 11 bytes:

```cpp
struct TimeRangeRule {
  uint8_t  weekDayMask;  // 1 byte : Days of week (bit 0=Sun, bit 6=Sat)
  uint32_t hourMask;     // 4 bytes: Hours of day (bit 0=00:00, bit 23=23:00)
  uint32_t monthDayMask; // 4 bytes: Days of month (bit 0=1st, bit 30=31st)
  uint16_t monthMask;    // 2 bytes: Months (bit 0=Jan, bit 11=Dec)
};
```

## Usage

### 1. Configure Values

Edit the values in main.cpp to set your desired configuration:

```cpp
// Set secret combination (4 digits, 0-9)
std::array<int, 4> secretCombination = {1, 2, 3, 4};

// Set time range rules
TimeRangeRule rules[1];
rules[0] = {
    .weekDayMask  = 0b1111111,                         // All days
    .hourMask     = 0b111111111111111111111111,        // All hours
    .monthDayMask = 0b1111111111111111111111111111111, // All days
    .monthMask    = 0b111111111111,                    // All months
};
```

### 2. Build and Upload

Using PlatformIO:

```sh
# Build the utility
pio run -e uno_r4_wifi

# Upload to device
pio run -e uno_r4_wifi -t upload

# Monitor serial output to verify
pio device monitor
```

### 3. Verify Output

The utility will:
1. Write the secret combination to EEPROM
2. Write the time range rules to EEPROM
3. Read back all values to verify
4. Print everything to Serial for confirmation

Example output:
```
--- Initialising EEPROM writer ---
Setting secret combination to {1, 2, 3, 4} for testing purposes
Stored secret combination digit in EEPROM: 1
Stored secret combination digit in EEPROM: 2
Stored secret combination digit in EEPROM: 3
Stored secret combination digit in EEPROM: 4
...
--- EEPROM writer setup complete ---
--- Reading written EEPROM data ---
Retrieved secret combination from EEPROM: {1, 2, 3, 4}
Number of time range rules stored in EEPROM: 1
```

### 4. Upload Main Alarm Code

After writing the EEPROM values, upload the main edge device code. The alarm system will read these values at startup and use them for operation.

## Key Files

### Source Files

- main.cpp: Main utility program that writes test values
- eeprom_driver.cpp: EEPROM read/write functions

### Header Files

- eeprom_driver.h: EEPROM interface and memory layout definitions
- time_range.h: Time range rule structure (shared with edge project)

## Key Functions

### EEPROM Operations

- `setupEEPROM()`: Reads secret combination and rule count from EEPROM
- `storeSecretCombinationEEPROM()`: Writes 4-digit secret combination
- `storeTimeRangeRulesEEPROM()`: Writes time range rules and count
- `retrieveTimeRangeRulesEEPROM()`: Reads time range rules from EEPROM

## Example Configurations

### Always Active (24/7)

```cpp
TimeRangeRule rules[1];
rules[0] = {
    .weekDayMask  = 0b1111111,                         // All days of week
    .hourMask     = 0b111111111111111111111111,        // All 24 hours
    .monthDayMask = 0b1111111111111111111111111111111, // All days of month
    .monthMask    = 0b111111111111,                    // All 12 months
};
```

### Weekdays 9-5

```cpp
TimeRangeRule rules[1];
rules[0] = {
    .weekDayMask  = 0b0111110,                         // Mon-Fri (bits 1-5)
    .hourMask     = 0b00000000111111111000000000,      // Hours 9-17 (bits 9-17)
    .monthDayMask = 0b1111111111111111111111111111111, // All days
    .monthMask    = 0b111111111111,                    // All months
};
```

### Multiple Time Windows

```cpp
size_t ruleCount = 2;
TimeRangeRule rules[2];

// Weekday monitoring
rules[0] = {
    .weekDayMask  = 0b0111110,                         // Mon-Fri
    .hourMask     = 0b00000000111111111000000000,      // 9-17h
    .monthDayMask = 0b1111111111111111111111111111111, // All days of the month
    .monthMask    = 0b111111111111,                    // All months of the year
};

// Weekend monitoring
rules[1] = {
    .weekDayMask  = 0b1000001,                         // Sat-Sun
    .hourMask     = 0b111111111111111111111111,        // All hours
    .monthDayMask = 0b1111111111111111111111111111111, // All days of the month
    .monthMask    = 0b111111111111,                    // All months of the year
};
```

## Building

Configuration is in platformio.ini:

```ini
[env:uno_r4_wifi]
platform = renesas-ra
board = uno_r4_wifi
framework = arduino
```

The utility shares the same hardware target as the main edge device.

## Important Notes

### One-Time Setup

This utility should only be run when you need to:
- Initialize a new device
- Change the secret combination
- Update monitoring time windows
- Test EEPROM functionality

### After Running

After successfully writing to EEPROM, upload the main edge device firmware from /edge.

### EEPROM Persistence

Values written to EEPROM persist across power cycles and firmware updates. To clear EEPROM, run this utility with your desired default values.

### Validation

The utility does not validate:
- Secret combination digits (should be 0-9)
- Time range rule logic
- Maximum rule count (255)

Validation is performed by the main edge device at startup.

## Dependencies

- **EEPROM**: Arduino EEPROM library for non-volatile storage
- Arduino core libraries

Shares the `eeprom_driver` and `time_range` modules with the main edge device project.

## Integration with Edge Device

The edge device reads these EEPROM values at startup in `setupSecurity()`:

1. Calls `setupEEPROM()` to read secret combination and rule count
2. Validates the secret combination (each digit 0-9)
3. Retrieves time range rules with `retrieveTimeRangeRulesEEPROM()`
4. Initializes the RTC with the time range rules
5. Enters CONFIGURATION mode if any validation fails

See edge/readme.md for more information about the main alarm system.