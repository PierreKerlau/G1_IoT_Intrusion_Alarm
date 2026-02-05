#include <Arduino.h>
#include "security_code.h"
#include "motion_detector.h"
#include "lora_comm.h"

bool isAlarmActive = false;

void setup() {
  Serial.begin(115200);

  setupMotion();
  setupSecurity();
  setupLora();

  Serial.println("System ready.");
  loraSendMotionState(false);
}

void loop() {
  if (!isAlarmActive) { // No alarm triggered, just check for motion
    if (checkMotion() == true) { // Motion detected, trigger alarm
      loraSendMotionState(true);
      resetAlarmState();

      isAlarmActive = true;
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
