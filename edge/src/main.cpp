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
  loraSendSensorState(0);
}

void loop() {
  if (!isAlarmActive) {
    if (checkMotion() == true) {
      resetAlarmState(); 
      
      isAlarmActive = true;
      loraSendSensorState(1);
    }
    delay(100); 
  } 
  
  else {
    bool disarmed = runSecurityLogic();

    if (disarmed == true) {
      isAlarmActive = false;
      loraSendSensorState(0);
    }
  }
}