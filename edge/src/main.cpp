#include "lora_comm.h"
#include "motion_detector.h"
#include "rtc.h"
#include "security_code.h"
#include <Arduino.h>

#define PRINT_TIME_IN_LOOP

bool          isAlarmActive   = false;
bool          lastTimeInRange = false;
unsigned long lastTimePrint;

void setup() {
  Serial.begin(115200);

  setupMotion();
  setupSecurity();
  setupRTC();
  setupLora();

  Serial.println("System ready.");
  loraSendMotionState(false);
  lastTimePrint = millis();
}

void loop() {
#ifdef PRINT_TIME_IN_LOOP
  if (millis() - lastTimePrint > 1000) { // Send heartbeat every minute
    // loraSendMotionState(isAlarmActive);
    lastTimePrint = millis();
    Serial.println(getTimeString());
  }
#endif // PRINT_TIME_IN_LOOP

  if (!isAlarmActive) { // No alarm triggered, just check for motion
    if (isTimeInRanges() != lastTimeInRange) {
      Serial.print("Time range status changed: ");
      Serial.println(isTimeInRanges() ? "IN RANGE" : "OUT OF RANGE");
      lastTimeInRange = isTimeInRanges();
      if (isTimeInRanges()) {
        setLedColorHSB(0.04, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "orange" for now
      } else {
        setLedColorHSB(0.65, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "blue" for now
      }
    }

    if (isTimeInRanges()) {
      // Serial.print(getTimeString());
      // Serial.println(" - Monitoring for motion");
      if (checkMotion() == true) { // Motion detected, trigger alarm
        loraSendMotionState(true);
        resetAlarmState();

        isAlarmActive = true;
      }
    }
    delay(100);
  } else {
    bool disarmed = runSecurityLogic();

    if (disarmed == true) {
      isAlarmActive = false;
      loraSendMotionState(false);
    }
  }
}
