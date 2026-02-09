#ifndef RTC_H
#define RTC_H

#include "time_range.h"
#include <Arduino.h>
#include <Wire.h>
#include <iarduino_RTC.h>

#define MINIMUM_UNIX_TIME 1770665090 // 09/02/2026
#define MAXIMUM_UNIX_TIME 0xFFFFFFFF // 07/02/2106 maximum uint32_t unix time
#define MAX_TIME_DELAY    1000       // Minimum delay between RTC and Broker timestamps before updating RTC

void   setupRTC(const TimeRangeRule* rules, size_t ruleCount);
bool   isMonitoringTime();
String getTimeString();

uint32_t getCurrentUnixTime();
void     setCurrentUnixTime(uint32_t unixTime);

void setTimeRangeRules(const TimeRangeRule* rules, size_t ruleCount);

#endif // RTC_H