#ifndef TIME_RANGE_H
#define TIME_RANGE_H

#include <Arduino.h>

/**
 * Represents a time range rule, which can be used to check if a given time falls within this range.
 * The rule is defined using bitmasks for each time component (weekday, hour, day of month, month).
 * This structure is 16 bytes in size.
 */
struct TimeRangeRule {
  uint8_t  weekDayMask;  // 0-6 (0-Sunday, 1-Monday, ... , 6-Saturday)
  uint32_t hourMask;     // 0-23 hours as bits
  uint32_t monthDayMask; // 1-31 days as bits
  uint16_t monthMask;    // 1-12 months as bits
};
#define TIME_RANGE_RULE_BYTES 11

void setTimeRanges(const TimeRangeRule* rules, size_t ruleCount);

#endif // TIME_RANGE_H
