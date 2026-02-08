#include "security_code.h"
#include <Arduino.h>

#define PRINT_TIME_IN_LOOP                    // Should print the time
#define PRINT_TIME_INTERVAL          1000     // Interval for printing the time in milliseconds
#define HEARTBEAT_TIME_INTERVAL      6 * 1000 // Interval for the LoRaWAN heartbeat in milliseconds
#define SECURITY_LOGIC_TIME_INTERVAL 100      // Interval for running the security logic in milliseconds

unsigned long lastSecurityLogicTime = 0;
unsigned long lastTimePrint         = 0;
unsigned long lastTimeHeartbeat     = 0;

/* TODO
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
  while (!Serial) {
    delay(100); // Wait for serial port to connect.
  }

  delay(3000); // DEBUG: Wait a moment before starting the system

  setupSecurity();
  setupLora();

  Serial.println("System ready!\n");
}

void loop() {
  AlarmState currentAlarmState = getAlarmState();

  // Print time and alarm state at regular intervals for debugging purposes
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

  // Send heartbeat message at regular intervals to indicate that the system is alive, with the current alarm state included in the payload data
  if (millis() - lastTimeHeartbeat > HEARTBEAT_TIME_INTERVAL) {
    lastTimeHeartbeat = millis();
    loraSendHeartbeat(getAlarmState());
  }
}
