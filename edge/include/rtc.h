#ifndef RTC_H
#define RTC_H

#include "time_range.h"
#include <Arduino.h>
#include <Wire.h>
#include <iarduino_RTC.h>

void     setupRTC(const TimeRangeRule* rules, size_t ruleCount);
String   getTimeString();
bool     isMonitoringTime();
uint32_t getCurrentUnixTime();

#endif // RTC_H