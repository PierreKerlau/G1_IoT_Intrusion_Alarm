#ifndef SECURITY_CODE_H
#define SECURITY_CODE_H

#include "eeprom_driver.h"
#include "lora_comm.h"
#include "motion_detector.h"
#include "rtc.h"
#include "security_animation.h"
#include "security_audio.h"
#include "time_range.h"
#include <Arduino.h>
#include <ChainableLED.h>
#include <TM1637.h>
#include <array>

/**
 * Stores the state of the alarm system.
 */
enum class AlarmState {
  INACTIVE      = 0, // No alarm and time window is not monitored
  MONITORING    = 1, // No alarm but time window is monitored, waiting for potential motion detection
  TRIGGERED     = 2, // Alarm triggered due to motion detection, waiting for user to disarm
  DISARMED      = 3, // Alarm was triggered but user successfully disarmed it
  FAILED_DISARM = 4, // Alarm was triggered and user failed to disarm it within the allowed time or number of tries
  CONFIGURATION = 5  // State used for configuration mode during the first setup (e.g., setting time, changing combination, etc.)
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