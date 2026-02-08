#ifndef SECURITY_CODE_H
#define SECURITY_CODE_H

#include "rtc.h"

/**
 * Stores the state of the alarm system.
 */
enum class AlarmState {
  INACTIVE,      // No alarm and time window is not monitored
  MONITORING,    // No alarm but time window is monitored, waiting for potential motion detection
  TRIGGERED,     // Alarm triggered due to motion detection, waiting for user to disarm
  DISARMED,      // Alarm was triggered but user successfully disarmed it
  FAILED_DISARM, // Alarm was triggered and user failed to disarm it within the allowed time or number of tries
};

extern bool isTimeInRanges();

void   setupSecurity();
void   startAlarmState();
bool   isWaitingForRelease();
String alarmStateToString(AlarmState state);

AlarmState runSecurityLogic();
AlarmState getAlarmState();
void       setAlarmState(AlarmState newState);

#endif // SECURITY_CODE_H