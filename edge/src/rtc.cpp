#include "rtc.h"

iarduino_RTC     rtc(RTC_DS1307); // Module DS1307 I2C
TimeRangeChecker timeRangeChecker = TimeRangeChecker();

void setupRTC(const TimeRangeRule* rules, size_t ruleCount) {
  rtc.begin();
  rtc.settimezone(1); // Heure d'hiver UTC+1

  // The time only needs to be set once, or after a power loss
  rtc.settime(
      55, // seconds
      59, // minutes
      17, // hours
      6,  // day
      3,  // month
      26, // year
      5   // weekday (0-6, 0=Sunday)
  );

  timeRangeChecker.setTimeRanges(rules, ruleCount);
}

String getTimeString() {
  return String(rtc.gettime("d-m-Y, H:i:s, D"));
}

/**
 * Checks if the current time falls within any of the defined time ranges for monitoring.
 * @return true if the current time is within a monitoring range, false otherwise.
 */
bool isMonitoringTime() {
  return timeRangeChecker.isMonitoringTime(rtc);
}

uint32_t getCurrentUnixTime() {
  return rtc.gettimeUnix();
}

void setCurrentUnixTime(uint32_t unixTime) {
  rtc.settimeUnix(unixTime);
}

void setTimeRangeRules(const TimeRangeRule* rules, size_t ruleCount) {
  timeRangeChecker.setTimeRanges(rules, ruleCount);
}