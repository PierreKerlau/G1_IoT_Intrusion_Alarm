#ifndef TIME_RANGE_H // Import once
#define TIME_RANGE_H

#include <Arduino.h>
#include "iarduino_RTC.h"

struct TimeRangeRule {
  uint8_t  weekDayMask;  // 0-6 (0-Sunday, 1-Monday, ... , 6-Saturday)
  uint32_t hourMask;     // 0-23 hours as bits
  uint32_t monthDayMask; // 1-31 days as bits
  uint16_t monthMask;    // 1-12 months as bits
};

class TimeRangeChecker {
private:
  TimeRangeRule* timeRanges;
  size_t rulesCount;

  bool isTimeInRange(const TimeRangeRule &timeRule, const TimeRangeRule &currentTimeAsRange);

public:
  bool isTimeInRanges(iarduino_RTC &rtc);
  void setTimeRanges(const TimeRangeRule *rules, size_t ruleCount);
};

#endif // TIME_RANGE_H
