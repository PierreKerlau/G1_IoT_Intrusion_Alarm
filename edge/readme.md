# Edge Device - Intrusion Alarm System

This is the edge device component of the IoT intrusion alarm system. It runs on an Arduino Uno R4 WiFi and handles motion detection, user interaction, and LoRa communication with the gateway.

## Hardware Components

- **Arduino Uno R4 WiFi**: Main microcontroller
- **PIR Motion Sensor**: Connected to pin 12 for motion detection
- **TM1637 4-Digit Display**: Connected to pins 6 (CLK) and 7 (DIO) for displaying the security code
- **ChainableLED**: Connected to pins 8 (CLK) and 9 (DATA) for status indication
- **Buzzer**: Connected to pin 11 for audio feedback
- **4 Buttons**: Connected to pins 2 (Blue), 3 (White), 4 (Red), 5 (Green) for code input
- **RTC Module (DS1307)**: I2C connection for timekeeping
- **LoRa Module (E5)**: Serial connection (Serial1, 9600 baud) for wireless communication

## Features

### Alarm States

The system operates in several states:

- **INACTIVE**: System is off, no monitoring
- **MONITORING**: Active monitoring for motion within configured time windows
- **TRIGGERED**: Alarm triggered by motion detection, awaiting disarm
- **DISARMED**: User successfully disarmed after trigger
- **FAILED_DISARM**: User failed to disarm within allowed attempts/time
- **CONFIGURATION**: Special mode for system setup

### Time-Based Monitoring

The system uses time range rules stored in EEPROM to determine when to actively monitor for intrusions. Rules can specify:
- Days of the week (bitmask)
- Hours of the day (bitmask)
- Days of the month (bitmask)
- Months of the year (bitmask)

### LoRa Communication

The device communicates with the gateway using LoRa at 868.1MHz (SF7, BW125). Two types of messages are sent:

1. **Heartbeat** (`PayloadType::EDGE_HEARTBEAT`): Periodic status updates including current alarm state
2. **Motion State** (`PayloadType::MOTION_STATE`): Sent when motion is detected

### Visual & Audio Feedback

- **LED Colors**:
  - Blue: Monitoring mode
  - Green: Disarmed
  - Yellow: Triggered
  - Red: Failed disarm
  - Purple: Configuration mode

- **Audio**:
  - Button press beeps
  - Success/failure tones for code entry
  - Alarm sounds when triggered
  - Timeout warnings

### EEPROM Storage

Persistent storage at specific addresses (see eeprom_driver.h):
- Address 0-3: Secret combination (4 digits)
- Address 100: Number of time range rules
- Address 101+: Time range rules (11 bytes each)

## Key Files

### Source Files

- main.cpp: Main program loop and initialization
- security_code.cpp: Core security logic and state management
- lora_comm.cpp: LoRa communication and payload handling
- eeprom_driver.cpp: EEPROM read/write operations
- rtc.cpp: Real-time clock management
- time_range.cpp: Time window checking logic
- motion_detector.cpp: PIR sensor interface
- security_animation.cpp: Visual feedback animations
- security_audio.cpp: Audio feedback generation

### Header Files

- security_code.h: Security system interface and types
- lora_comm.h: LoRa communication interface
- eeprom_driver.h: EEPROM storage interface
- rtc.h: RTC interface
- time_range.h: Time range rule structures
- motion_detector.h: Motion sensor interface
- security_animation.h: Animation interface
- security_audio.h: Audio interface

## Building and Uploading

This project uses PlatformIO for building and uploading:

```sh
# Build the project
pio run -t build

# Upload to device
pio run -t upload

# Open serial monitor
pio device monitor
```

Configuration is in platformio.ini.

## Configuration

### Setting the Secret Combination

The 4-digit secret combination can be configured via LoRa commands from the gateway (see `parsePayloadType` for supported payload types).

### Time Range Rules

Time monitoring windows can be configured remotely via LoRa commands. Rules use bitmasks to define when the system should be in monitoring mode.

### RTC Time

The system time can be set remotely via LoRa to ensure accurate time-based monitoring.

## Operation

1. **Startup**: System reads configuration from EEPROM and initializes all components
2. **Monitoring**: When in configured time windows, the system actively monitors for motion
3. **Motion Detection**: Triggers alarm state, LED turns yellow, alarm sounds
4. **Disarm Attempt**: User has limited attempts and time to enter correct code
5. **Success/Failure**: System transitions to DISARMED or FAILED_DISARM state
6. **Heartbeats**: Periodic status updates sent to gateway via LoRa

## Dependencies

Key libraries used:
- **TM1637**: 4-digit display control
- **ChainableLED**: RGB LED control
- **iarduino_RTC**: Real-time clock interface
- **EEPROM**: Non-volatile storage
- Arduino core libraries

See platformio.ini for complete dependency list.