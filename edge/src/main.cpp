#include "lora_comm.h"
#include "motion_detector.h"
#include "rtc.h"
#include "security_code.h"
#include <Arduino.h>

#define PRINT_TIME_IN_LOOP           // Should print the time
#define PRINT_TIME_INTERVAL     1000 // Interval for printing the time in milliseconds
#define HEARTBEAT_TIME_INTERVAL 1000 // Interval for the LoRaWAN heartbeat in milliseconds

bool          isAlarmActive        = false;
bool          lastIsMonitoringTime = false;
unsigned long lastTimePrint        = 0;

void setup() {
  Serial.begin(115200);

  setupMotion();
  setupSecurity();
  setupRTC();
  setupLora();

  Serial.println("System ready.");
  loraSendMotionState(false);
}

void loop() {
  // Print time every minute
#ifdef PRINT_TIME_IN_LOOP
  if (millis() - lastTimePrint > PRINT_TIME_INTERVAL) {
    // loraSendMotionState(isAlarmActive);
    lastTimePrint = millis();
    Serial.println(getTimeString());
  }
#endif // PRINT_TIME_IN_LOOP

  // Send heartbeat
  // if (millis() - lastTimePrint > HEARTBEAT_TIME_INTERVAL) {
  //   // loraSendMotionState(isAlarmActive);
  //   lastTimePrint = millis();
  //   Serial.println(getTimeString());
  // }

  if (!isAlarmActive) { // No alarm triggered, check for time window and motion
    bool isMonitoringTime = isTimeInRanges();
    if (isMonitoringTime != lastIsMonitoringTime) { // Time range status just changed
      Serial.print("Time range status changed: ");
      Serial.println(isMonitoringTime ? "IN RANGE" : "OUT OF RANGE");
      lastIsMonitoringTime = isMonitoringTime;
      if (isMonitoringTime) {
        setLedColorHSB(0.04, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "orange" for now
      } else {
        setLedColorHSB(0.65, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "blue" for now
      }
    }

    if (isMonitoringTime) {
      // Serial.print(getTimeString());
      // Serial.println(" - Monitoring for motion");
      if (checkMotion() == true) { // Motion detected, trigger alarm
        loraSendMotionState(true);
        startAlarmState();

        isAlarmActive = true;
      }
    }
    delay(100);
  } else { // Alarm is active, run security logic
    // Only run security logic if we're not waiting for button release
    if (!isWaitingForRelease()) {
      bool disarmed = runSecurityLogic();

      if (disarmed == true) {
        isAlarmActive = false;
        loraSendMotionState(false);
      }
    } else {
      Serial.println("Waiting for button release...");
    }
  }
}
