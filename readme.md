# IoT Intrusion Alarm System

A comprehensive IoT security system built with Arduino Uno R4 WiFi, featuring motion detection, time-based monitoring, LoRa wireless communication, and remote control via Node-RED.

## System Overview

This project implements a complete intrusion alarm system with three main components:

1. **Edge Device**: Motion-detecting alarm with user interface (keypad, display, LED)
2. **Gateway**: LoRa-to-Serial bridge for communication between edge device and server
3. **EEPROM Utility**: Configuration tool for initializing device settings

### Key Features

- **Time-Based Monitoring**: Configure specific time windows when the system actively monitors for intrusions
- **LoRa Wireless Communication**: Long-range, low-power wireless communication between edge device and gateway
- **HMAC Integrity**: Secure message integrity verification
- **Remote Configuration**: Update settings (combination, time rules, alarm state) via Node-RED
- **Persistent Storage**: Configuration stored in EEPROM survives power cycles
- **Visual & Audio Feedback**: LED colors and buzzer tones indicate system status
- **Real-Time Clock**: Accurate timekeeping for time-based monitoring rules

## Project Structure

```
G1_IoT_Intrusion_Alarm/
├── edge/                       # Edge device (alarm system)
│   ├── src/
│   │   ├── main.cpp           # Main program loop
│   │   ├── security_code.cpp  # Security logic and state management
│   │   ├── lora_comm.cpp      # LoRa communication
│   │   ├── eeprom_driver.cpp  # EEPROM operations
│   │   ├── rtc.cpp            # Real-time clock
│   │   ├── time_range.cpp     # Time window checking
│   │   ├── motion_detector.cpp # PIR sensor interface
│   │   ├── security_animation.cpp # Visual feedback
│   │   └── security_audio.cpp # Audio feedback
│   ├── include/               # Header files
│   └── readme.md             # Edge device documentation
│
├── gateway/                   # Gateway (LoRa-Serial bridge)
│   ├── src/
│   │   └── main.cpp          # Bridge logic and conversion
│   ├── include/
│   │   └── main.h            # Type definitions
│   └── readme.md             # Gateway documentation
│
├── utils/
│   └── eeprom/               # EEPROM configuration utility
│       ├── src/
│       │   ├── main.cpp      # EEPROM writer
│       │   └── eeprom_driver.cpp
│       ├── include/
│       │   ├── eeprom_driver.h
│       │   └── time_range.h
│       └── readme.md         # Utility documentation
│
├── platformio.ini            # PlatformIO configuration
└── readme.md                 # This file
```

## Hardware Requirements

### Edge Device
- Arduino Uno R4 WiFi
- LoRa Module (E5) on Serial1 (9600 baud)
- PIR Motion Sensor (pin 12)
- TM1637 4-Digit Display (pins 6 CLK, 7 DIO)
- ChainableLED RGB LED (pins 8 CLK, 9 DATA)
- Buzzer (pin 11)
- 4 Push Buttons (pins 2, 3, 4, 5)
- RTC Module DS1307 (I2C)

### Gateway
- Arduino Uno R4 WiFi
- LoRa Module (E5) via SoftwareSerial (pins 8 RX, 9 TX)

## Software & Tools

### Development Tools
- **PlatformIO**: Build system and dependency management
- **Visual Studio Code**: IDE with PlatformIO extension
- **Arduino Framework**: Core libraries for Arduino development

### Key Libraries
- **TM1637**: 4-digit 7-segment display driver
- **ChainableLED**: RGB LED control library
- **iarduino_RTC**: Real-time clock interface for DS1307
- **SoftwareSerial**: Software-based serial communication
- **EEPROM**: Non-volatile storage access

### Communication Stack
- **LoRa**: 868.1 MHz, SF7, BW125 for edge-gateway communication
- **Serial/USB**: 115200 baud for gateway-Node-RED communication
- **JSON**: Message format for Serial communication
- **Hex Encoding**: Binary payload representation for LoRa

## Getting Started

### 1. Hardware Setup

Connect all components according to the pin configurations documented in [Edge readme.md](./edge/readme.md) and [Gateway readme.md](./gateway/readme.md).

### 2. Configure EEPROM (First Time Only)

Before uploading the main edge device code, you may need to initialize the EEPROM with your desired configuration:

```sh
# Navigate to EEPROM utility
cd utils/eeprom

# Edit src/main.cpp to set your secret combination and time rules

# Build and upload
pio run -t upload

# Verify via serial monitor
pio device monitor
```

See readme.md for detailed configuration options.

### 3. Upload Edge Device Firmware

```sh
# Navigate to edge directory
cd edge

# Build and upload
pio run -t upload

# Monitor operation
pio device monitor
```

### 4. Upload Gateway Firmware

```sh
# Navigate to gateway directory
cd gateway

# Build and upload
pio run -t upload
```

### 5. Connect to Node-RED

Connect the gateway Arduino to your Node-RED server via USB. Configure a serial node with:
- **Port**: Auto-detect or specify USB port
- **Baud Rate**: 115200
- **Format**: JSON strings

## System Architecture

### Communication Flow

#### Edge to Node-RED
```
[Edge Device] --LoRa HEX (868.1MHz)--> [Gateway] --USB Serial JSON--> [Node-RED Server]
```

#### Node-RED to Edge
```
[Node-RED Server] --USB Serial JSON--> [Gateway] --LoRa HEX (868.1MHz)--> [Edge Device]
```

### Alarm States

The edge device operates in the following states:

1. **INACTIVE**: System off, no monitoring
2. **MONITORING**: Active monitoring within configured time windows
3. **TRIGGERED**: Motion detected, awaiting user disarm
4. **DISARMED**: Successfully disarmed after trigger
5. **FAILED_DISARM**: Failed to disarm within time/attempt limits
6. **CONFIGURATION**: Setup mode (entered on invalid EEPROM data)

### Message Types

#### Edge → Gateway (LoRa)
- **EDGE_HEARTBEAT** (0x01): Periodic status updates with alarm state
- **MOTION_STATE** (0x02): Motion detection events

#### Gateway → Edge (LoRa)
- **SET_COMBINATION** (0x11): Update secret combination
- **SET_TIME_RANGE** (0x12): Update monitoring time windows
- **SET_ALARM_STATE** (0x13): Force alarm state change
- **SET_RTC_TIME** (0x14): Synchronize RTC time

### Payload Format

**LoRa (Hex String)**:
```
[ID:1][TS:4][TYPE:1][LENGTH:1][DATA:0-200][HMAC:4]
```

**Serial (JSON)**:
```json
{
  "id": 1,
  "ts": 1234567890,
  "type": 1,
  "length": 1,
  "data": "01",
  "hmac": "ABCD1234"
}
```

## Configuration

### Secret Combination
- 4 digits (0-9)
- Stored in EEPROM addresses 0-3
- Can be updated via LoRa command `SET_COMBINATION`

### Time Range Rules
- Bitmask-based time windows
- Stored in EEPROM starting at address 101
- Supports up to 255 rules
- Each rule: 11 bytes (weekday, hour, monthday, month masks)
- Can be updated via LoRa command `SET_TIME_RANGE`

### RTC Synchronization
- DS1307 RTC module
- Timezone: UTC+1
- Can be updated via LoRa command `SET_RTC_TIME`

## Security Features

### HMAC Authentication
All LoRa messages include HMAC signature for integrity verification:
- Algorithm: DJB2-based hash
- Key: Shared secret (configured in code)
- Prevents message tampering and replay attacks

### Access Control
- Multi-attempt limit (3 tries)
- Time-limited disarm window (15 seconds)
- Failed attempts trigger alarm lockout

## LED Status Indicators

| Color  | Alarm State       |
|--------|-------------------|
| Off    | INACTIVE          |
| Blue   | MONITORING        |
| Yellow | TRIGGERED         |
| Green  | DISARMED          |
| Red    | FAILED_DISARM     |
| Purple | CONFIGURATION     |

## Troubleshooting

### Edge Device Enters CONFIGURATION Mode
- Invalid EEPROM data detected
- Run EEPROM utility to write valid configuration
- Check that secret combination digits are 0-9

### No LoRa Communication
- Verify both devices use same frequency (868.1 MHz)
- Check LoRa module connections
- Ensure both devices initialized successfully (check serial output)

### Incorrect Time-Based Monitoring
- Verify RTC time is set correctly
- Check time range rule bitmasks
- Use EEPROM utility to update rules if needed

### Gateway Not Forwarding Messages
- Check Serial baud rate (115200)
- Verify Node ID matches between devices
- Enable DEBUG_SERIAL_PRINT for detailed logs

## Documentation

Detailed documentation for each component:

- **Edge Device**: readme.md
- **Gateway**: readme.md  
- **EEPROM Utility**: readme.md

## Development

### Building All Projects

```sh
# Build everything
pio run

# Build specific project
pio run
```

### Code Style

The project uses clang-format with configuration in .clang-format:
- 2-space indentation
- Pointer alignment left
- Consecutive alignment for assignments and declarations

### Testing

- Monitor serial output for debug information
- Test each alarm state transition
- Verify LoRa communication with gateway
- Check EEPROM persistence across power cycles

## Future Enhancements

- [ ] SD card logging for event history
- [ ] Camera integration (Xiao ESP32S3 Sense)
- [ ] Multiple node support in Node-RED using DEVICE ID
- [ ] Battery backup support

## Contributors

Group 1 - IoT Intrusion Alarm Project