# Gateway - LoRa to Serial Bridge

This is the gateway component of the IoT intrusion alarm system. It runs on an Arduino Uno R4 WiFi and acts as a bridge between the edge device (via LoRa) and the Node-RED server (via Serial/USB).

## Hardware Components

- **Arduino Uno R4 WiFi**: Main microcontroller
- **LoRa Module (E5)**: Connected via SoftwareSerial on pins 8 (RX) and 9 (TX) for wireless communication with edge device

## Overview

The gateway performs bidirectional communication:
- **LoRa → Serial**: Receives LoRa packets from edge device, converts to JSON, sends to Node-RED
- **Serial → LoRa**: Receives JSON commands from Node-RED, converts to LoRa packets, sends to edge device

This enables remote monitoring and control of the alarm system through the Node-RED dashboard.

## Communication Protocol

### LoRa Configuration

- **Frequency**: 868.1 MHz
- **Spreading Factor**: SF7
- **Bandwidth**: 125 kHz
- **Configuration Command**: `AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF`

### Payload Structure

The [`LoraPayload`](include/main.h) struct contains:

```cpp
struct LoraPayload {
  uint8_t     id;     // Node ID (1 byte)
  uint32_t    ts;     // Unix timestamp (4 bytes)
  PayloadType type;   // Payload type (1 byte)
  uint8_t     length; // Data length (1 byte)
  String      data;   // Hex string representing payload data
  String      hmac;   // Hex string representing HMAC (4 bytes)
}
```

### Payload Types

Defined in [`PayloadType`](include/main.h) enum:

- `EDGE_HEARTBEAT` (0x01): Periodic status updates from edge device
- `MOTION_STATE` (0x02): Motion detection events from edge device
- `CONFIGURATION` (0x03): Configuration messages (for future use)

### Data Formats

**LoRa Format** (Hex string with byte syze):
```
[ID:1][TS:4][TYPE:1][LENGTH:1][DATA:1-200][HMAC:4]
```
Example: `0100000000010105ABCD1234` represents ID=1, TS=0, TYPE=1, LENGTH=1, DATA=5, HMAC=ABCD1234(hex)

**JSON Format**:
```json
{"id":1,"ts":1234567890,"type":1,"length":1,"data":"05","hmac":"ABCD1234"}
```

## Key Files

### Source Files

- [`main.cpp`](src/main.cpp): Main program with communication loops and conversion logic

### Header Files

- [`main.h`](include/main.h): Type definitions and function declarations

## Key Functions

### Communication Functions

- [`listenLora()`](src/main.cpp): Non-blocking listener for LoRa module data
- [`listenSerial()`](src/main.cpp): Non-blocking listener for Serial/USB data

### Conversion Functions

**LoRa to Serial Pipeline:**
1. [`loraToSerial()`](src/main.cpp): Orchestrates LoRa → JSON conversion
2. [`hexToPayload()`](src/main.cpp): Converts hex string to `LoraPayload` struct
3. [`payloadToJson()`](src/main.cpp): Converts `LoraPayload` to JSON string

**Serial to LoRa Pipeline:**
1. [`serialToLora()`](src/main.cpp): Orchestrates JSON → LoRa conversion
2. [`jsonToPayload()`](src/main.cpp): Converts JSON string to `LoraPayload` struct
3. [`payloadToHex()`](src/main.cpp): Converts `LoraPayload` to hex string

### Utility Functions

- [`printPayload()`](src/main.cpp): Debug output for payload contents

## Building and Uploading

This project uses PlatformIO for building and uploading:

```sh
# Build the project
pio run -e uno_r4_wifi

# Upload to device
pio run -e uno_r4_wifi -t upload

# Open serial monitor
pio device monitor
```

Configuration is in [platformio.ini](../platformio.ini).

## Operation Flow

### Startup Sequence

1. Initialize Serial communication at 115200 baud
2. Initialize LoRa SoftwareSerial at 9600 baud
3. Configure LoRa module with `AT+MODE=TEST`
4. Set LoRa RF parameters with `LORA_RFCFG_CMD`
5. Enter receive mode with `AT+TEST=RXLRPKT`

### Runtime Operation

The main loop continuously:
1. Checks for incoming LoRa data and forwards to Serial as JSON
2. Checks for incoming Serial data and forwards to LoRa as hex

**Example LoRa Reception:**
```
LoRa: +TEST: RX,"0100000000010101ABCD1234"
  ↓ Parse & Convert
Serial: {"id":1,"ts":0,"type":1,"length":1,"data":"01","hmac":"ABCD1234"}
```

**Example Serial Transmission:**
```
Serial: {"id":1,"ts":1234567890,"type":3,"length":4,"data":"01020304","hmac":"DEADBEEF"}
  ↓ Parse & Convert
LoRa: AT+TEST=TXLRPKT,"01499602D2030401020304DEADBEEF"
```

## Integration with Node-RED

The gateway expects to communicate with a Node-RED server over USB Serial:

- **Baud Rate**: 115200
- **Format**: JSON strings, one per line
- **Direction**: Bidirectional

Node-RED can:
- Monitor heartbeat messages and motion detection events
- Send configuration commands to the edge device
- Display alarm system status on a dashboard

## Debugging

Enable debug output by uncommenting `#define DEBUG_SERIAL_PRINT` in [main.cpp](src/main.cpp) (DO NOT USE IN PRODUCTION). This will print:
- Raw LoRa payloads received
- Hex data extracted from LoRa messages
- Conversion results
- Serial commands received
- Conversion errors

## Configuration

### Node ID Filtering

The gateway only processes messages from the expected edge device node:

```cpp
#define EXPECTED_ID 1
```

Modify this value to match your edge device's node ID.

## Dependencies

- **SoftwareSerial**: For LoRa module communication
- Arduino core libraries

See [platformio.ini](../platformio.ini) for complete dependency list.

## Protocol Notes

### Big-Endian Encoding

Multi-byte values are transmitted in big-endian (network) byte order:
- Timestamp (4 bytes): Most significant byte first
- HMAC (4 bytes): Most significant byte first

### Hex String Format

All binary data is represented as uppercase hex strings:
- Each byte = 2 hex characters
- Example: byte 0x1F → "1F"

### Error Handling

The gateway performs validation:
- Checks payload structure and length
- Verifies node ID matches expected value
- Validates JSON format from Serial
- Handles timeout and parsing errors gracefully

## Limitations

- Single node support (only processes messages from `EXPECTED_ID`)
- No HMAC verification (performed on edge device)
- Will filter bad payloads without attempting to reconstruct them