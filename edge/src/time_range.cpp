#include "time_range.h"

bool TimeRangeChecker::isTimeInRange(const TimeRangeRule& timeRule, const TimeRangeRule& currentTimeAsRange) {
  // Check day of week
  if ((timeRule.weekDayMask & currentTimeAsRange.weekDayMask) == 0) {
    // Serial.print(" -> Weekday does not match"); Serial.print(" (Rule mask: "); Serial.print(timeRule.weekDayMask, BIN); Serial.print(", Current mask: "); Serial.print(currentTimeAsRange.weekDayMask, BIN); Serial.println(")");
    return false;
  }
  // Check hour
  if ((timeRule.hourMask & currentTimeAsRange.hourMask) == 0) {
    // Serial.print(" -> Hour does not match"); Serial.print(" (Rule mask: "); Serial.print(timeRule.weekDayMask, BIN); Serial.print(", Current mask: "); Serial.print(currentTimeAsRange.weekDayMask, BIN); Serial.println(")");
    return false;
  }
  // Check day of month
  if ((timeRule.monthDayMask & currentTimeAsRange.monthDayMask) == 0) {
    // Serial.print(" -> Day of month does not match"); Serial.print(" (Rule mask: "); Serial.print(timeRule.weekDayMask, BIN); Serial.print(", Current mask: "); Serial.print(currentTimeAsRange.weekDayMask, BIN); Serial.println(")");
    return false;
  }
  // Check month
  if ((timeRule.monthMask & currentTimeAsRange.monthMask) == 0) {
    // Serial.print(" -> Month does not match"); Serial.print(" (Rule mask: "); Serial.print(timeRule.weekDayMask, BIN); Serial.print(", Current mask: "); Serial.print(currentTimeAsRange.weekDayMask, BIN); Serial.println(")");
    return false;
  }
  // Serial.print(" -> Time matches this rule"); Serial.print(" (Rule weekDay: "); Serial.print(timeRule.weekDayMask, BIN); Serial.print(", hour: "); Serial.print(timeRule.hourMask, BIN); Serial.print(", day: "); Serial.print(timeRule.monthDayMask, BIN); Serial.print(", month: "); Serial.print(timeRule.monthMask, BIN); Serial.println(")");
  return true; // All checks passed, time is in range
}

bool TimeRangeChecker::isMonitoringTime(iarduino_RTC& rtc) {
  TimeRangeRule currentTimeAsRange;
  uint16_t      weekDay  = rtc.weekday;
  uint16_t      hour     = rtc.Hours;
  uint16_t      monthDay = rtc.day;
  uint16_t      month    = rtc.month;

#ifdef DEBUG
  Serial.print("Weekday=");
  Serial.print(weekDay);
  Serial.print(", Hour=");
  Serial.print(hour);
  Serial.print(", Day=");
  Serial.print(monthDay);
  Serial.print(", Month=");
  Serial.print(month);
  Serial.print(", Hour=");
  Serial.print(hour);
#endif // DEBUG

  if (weekDay > 6 || hour > 23 || monthDay == 0 || monthDay > 31 || month == 0 || month > 12) {
    Serial.print("Invalid time values from RTC: ");
    Serial.println();

    return false;
  }

  currentTimeAsRange.weekDayMask  = 1 << weekDay;  // Get weekday (0-6) and convert to bitmask
  currentTimeAsRange.hourMask     = 1 << hour;     // Get hour (0-23) and convert to bitmask
  currentTimeAsRange.monthDayMask = 1 << monthDay; // Get day of month (1-31) and convert to bitmask
  currentTimeAsRange.monthMask    = 1 << month;    // Get month (1-12) and convert to bitmask

#ifdef DEBUG
  Serial.print("Current time: ");
  Serial.print(rtc.gettime("d-m-Y, H:i:s, D"));

  Serial.print(" Current time as range: ");
  Serial.print("D=");
  Serial.print(weekDay);
  Serial.print(", H=");
  Serial.print(hour);
  Serial.print(", d=");
  Serial.print(monthDay);
  Serial.print(", m=");
  Serial.print(month);

  Serial.print(", MASKS Weekday: ");
  Serial.print(currentTimeAsRange.weekDayMask, BIN);
  Serial.print(", Hour: ");
  Serial.print(currentTimeAsRange.hourMask, BIN);
  Serial.print(", Day: ");
  Serial.print(currentTimeAsRange.monthDayMask, BIN);
  Serial.print(", Month: ");
  Serial.print(currentTimeAsRange.monthMask, BIN);
  Serial.println();
#endif // DEBUG

  for (size_t i = 0; i < rulesCount; ++i) {
    if (isTimeInRange(timeRanges[i], currentTimeAsRange)) {
      return true; // Time matches at least one rule
    }
  }
  return false; // No rules matched
}

/**
 * Sets the time range rules to be used for monitoring.
 * The provided rules are copied into the TimeRangeChecker instance.
 * @param rules An array of TimeRangeRule structures defining the time ranges for monitoring.
 * @param ruleCount The number of rules in the provided array.
 */
void TimeRangeChecker::setTimeRanges(const TimeRangeRule* rules, size_t ruleCount) {
  if (rules == nullptr || ruleCount == 0) {
    Serial.println("No time range rules provided. Monitoring will be disabled.");
    rulesCount = 0;
    delete[] timeRanges; // Free any existing rules
    timeRanges = nullptr;
    return;
  }

  if (ruleCount > 255) {
    Serial.println("Error: Cannot set more than 255 time range rules. Only the first 255 rules will be used. Provided ruleCount: " + String(ruleCount));
    ruleCount = 255; // Adjust to maximum allowed
  }

  timeRanges = new TimeRangeRule[ruleCount];
  for (size_t i = 0; i < ruleCount; ++i) {
    timeRanges[i] = rules[i];
  }
  rulesCount = ruleCount;

  // // Test with hardcoded values for now
  // rulesCount = 1;
  // timeRanges = new TimeRangeRule[rulesCount];
  // // Every day, every hour, every day of month, every month
  // // timeRanges[0] = TimeRangeRule{
  // //   .weekDayMask  = 0b01111111, // All days of the week
  // //   .hourMask     = 0b00000000111111111111111111111111, // All hours of the day (0-23)
  // //   .monthDayMask = 0b11111111111111111111111111111110, // All days of the month (1-31)
  // //   .monthMask    = 0b0001111111111110,
  // // };
  // // Every day, every hour, every day of month, every month
  // timeRanges[0] = TimeRangeRule{
  //     .weekDayMask  = 0b00111110,                         // Monday to Friday (1-5)
  //     .hourMask     = 0b00000000111111000000000011111100, // 2h-7h, 18h-23h (0-1, 8-17)
  //     .monthDayMask = 0b11111111111111111111111111111110, // All days of the month (1-31)
  //     .monthMask    = 0b0000000001111000,                 // March to June (3-6)
  // };
  // Serial.println("Added time range rule:");
  // Serial.print(" - Weekday mask: ");
  // Serial.println(timeRanges[0].weekDayMask, BIN);
  // Serial.print(" - Hour mask: ");
  // Serial.println(timeRanges[0].hourMask, BIN);
  // Serial.print(" - Month day mask: ");
  // Serial.println(timeRanges[0].monthDayMask, BIN);
  // Serial.print(" - Month mask: ");
  // Serial.println(timeRanges[0].monthMask, BIN);
  // Serial.println();
}
