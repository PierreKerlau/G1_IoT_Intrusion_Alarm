#include "Wire.h"
#include "iarduino_RTC.h"
#include "time_range.h"

iarduino_RTC rtc(RTC_DS1307); // Module DS1307 I2C
TimeRangeChecker timeRangeChecker = TimeRangeChecker();

void setupRTC() {
  rtc.begin();

  // The time only needs to be set once, or after a power loss
  rtc.settime(
      40, // seconds
      59,  // minutes
      17, // hours
      6,  // day
      3,  // month
      26, // year
      5   // weekday (0-6, 0=Sunday)
  );

  // TODO: Load time range rules from EEPROM

  timeRangeChecker.setTimeRanges(nullptr, 0); // TODO: Pass actual rules here
}

String getTimeString() {
  return String(rtc.gettime("d-m-Y, H:i:s, D"));
}

bool isTimeInRanges() {
  return timeRangeChecker.isTimeInRanges(rtc);
}
