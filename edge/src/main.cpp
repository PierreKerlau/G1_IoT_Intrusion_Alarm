#include "lora_comm.h"
#include "security_code.h"
#include <Arduino.h>

#define PRINT_TIME_IN_LOOP                // Should print the time
#define PRINT_TIME_INTERVAL          1000 // Interval for printing the time in milliseconds
#define HEARTBEAT_TIME_INTERVAL      1000 // Interval for the LoRaWAN heartbeat in milliseconds
#define SECURITY_LOGIC_TIME_INTERVAL 100  // Interval for running the security logic in milliseconds

unsigned long lastSecurityLogicTime = 0;
unsigned long lastTimePrint         = 0;

/* TODOS
- Implement EEPROM storage for the secret combination
- Implement EEPROM storage for the time range rules

- Add LoRaWAN messages for state changes, especially for TRIGGERED, DISARMED and FAILED_DISARM states
- Add functionality to change the secret combination via LoRaWAN

- Connect to the Xiao ESP32S3 Sense
- Add SD card logging for all events and state changes
- Add Camera to capture images when motion is detected and the alarm is triggered

- Add error handling
- Test system resilience for various edge cases (e.g., power loss, sensor failure, etc.)
- Add comments and documentation for all functions and variables
- Integrate with other system components as needed
*/

void setup() {
  Serial.begin(115200);

  setupSecurity();
  setupLora();

  Serial.println("System ready.");
  loraSendMotionState(false);
}

void loop() {
  AlarmState currentAlarmState = getAlarmState();

  // Print time every minute
#ifdef PRINT_TIME_IN_LOOP
  if (millis() - lastTimePrint > PRINT_TIME_INTERVAL) {
    // loraSendMotionState(isAlarmActive);
    lastTimePrint = millis();
    Serial.println(getTimeString() + " - Alarm state: " + alarmStateToString(currentAlarmState));
  }
#endif // PRINT_TIME_IN_LOOP

  if (millis() - lastSecurityLogicTime > SECURITY_LOGIC_TIME_INTERVAL) {
    lastSecurityLogicTime = millis();
    runSecurityLogic();
  }

  // Send heartbeat
  // if (millis() - lastTimePrint > HEARTBEAT_TIME_INTERVAL) {
  //   // loraSendMotionState(isAlarmActive);
  //   lastTimePrint = millis();
  //   Serial.println(getTimeString());
  // }

  // if (!isAlarmActive) { // No alarm triggered, check for time window and motion
  //   // bool monitoringTime = isMonitoringTime();
  //   // if (monitoringTime != lastIsMonitoringTime) { // Time range status just changed
  //   //   Serial.print("Time range status changed: ");
  //   //   Serial.println(monitoringTime ? "IN RANGE" : "OUT OF RANGE");
  //   //   lastIsMonitoringTime = monitoringTime;
  //   //   if (monitoringTime) {
  //   //     updateLedColor(0.04, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "orange" for now
  //   //   } else {
  //   //     updateLedColor(0.65, 1.0, 0.2); // TODO: Define colors as constants or enums, this is "blue" for now
  //   //   }
  //   // }

  //   if (monitoringTime) {
  //     // Serial.print(getTimeString());
  //     // Serial.println(" - Monitoring for motion");
  //     if (checkMotion() == true) { // Motion detected, trigger alarm
  //       loraSendMotionState(true);
  //       startAlarmState();

  //       isAlarmActive = true;
  //     }
  //   }
  //   delay(100);
  // } else { // Alarm is active, run security logic
  //   // Only run security logic if we're not waiting for button release
  //   if (!isWaitingForRelease()) {
  //     bool disarmed = runSecurityLogic();

  //     if (disarmed == true) {
  //       isAlarmActive = false;
  //       loraSendMotionState(false);
  //     }
  //   } else {
  //     Serial.println("Waiting for button release...");
  //   }
  // }
}
