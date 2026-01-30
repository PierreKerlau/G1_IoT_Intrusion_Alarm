#include <Arduino.h>
#include "security_code.h"
#include "motion_detector.h"

bool isAlarmActive = false;

void setup() {
  Serial.begin(9600);
  setupMotion();
  setupSecurity();
}

void loop() {
  if (!isAlarmActive) {
    if (checkMotion() == true) {
      resetAlarmState(); 
      
      isAlarmActive = true;
      Serial.println("Motion detected.");
    }
    delay(100); 
  } 
  
  else {
    bool disarmed = runSecurityLogic();

    if (disarmed == true) {
      isAlarmActive = false;
      Serial.println("System disarmed.");
      delay(3000); 
      Serial.println("Waiting for motion.");
    }
  }
}