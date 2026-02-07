#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

void   setupRTC();
String getTimeString();
bool   isTimeInRanges();

#endif // RTC_H