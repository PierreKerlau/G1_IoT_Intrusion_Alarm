#include "security_code.h"
#include <Arduino.h>

#define PRINT_TIME_IN_LOOP                // Should print the time
#define PRINT_TIME_INTERVAL          1000 // Interval for printing the time in milliseconds (use 3 * 1000 for production)
#define SECURITY_LOGIC_TIME_INTERVAL 100  // Interval for running the security logic in milliseconds

unsigned long lastSecurityLogicTime = 0;
unsigned long lastTimePrint         = 0;

/**
 * Setup every sensor, actuator and device (LED, buttons, RTC, motion sensor, etc.).
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

/**
 * Main loop of the program.
 * The bulk of the logic is managed by runSecurityLogic() and its AlarmState.
 */
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
}
