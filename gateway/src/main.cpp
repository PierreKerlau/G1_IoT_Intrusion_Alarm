#include "main.h"

// #define DEBUG_SERIAL_PRINT

// --- CONFIGURATION ---
#define LORA_RX     8
#define LORA_TX     9
#define EXPECTED_ID 1 // TODO: Configurable ID

#define LORA_RFCFG_CMD "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF"

SoftwareSerial loraSerial(LORA_RX, LORA_TX);

// Test
const unsigned long TEST_CONFIG_PAYLOAD_INTERVAL = 10000; // Interval to send test configuration payloads in milliseconds
unsigned long       lastTestConfigPayloadTime    = 0;

void setup() {
  Serial.begin(115200);
  loraSerial.begin(9600);

  while (!Serial) {
    delay(100); // Wait for serial port to connect
  }
  delay(3000); // Wait a moment before starting the system

#ifdef DEBUG_SERIAL_PRINT
  Serial.println(F("--- Initialising the gateway ---"));
#endif // DEBUG_SERIAL_PRINT

  // Simple AT initialization sequence for the LoRa module
  loraSerial.println("AT+MODE=TEST");
  delay(500);
  loraSerial.println(LORA_RFCFG_CMD);
  delay(500);
  loraSerial.println("AT+TEST=RXLRPKT");

#ifdef DEBUG_SERIAL_PRINT
  Serial.println(F("Listening (Freq: 868.1MHz)..."));
#endif // DEBUG_SERIAL_PRINT

  lastTestConfigPayloadTime = millis();
}

void loop() {
  listenLora();
  listenSerial();
  delay(50); // Small delay to avoid busy looping

  //   if (millis() - lastTestConfigPayloadTime > TEST_CONFIG_PAYLOAD_INTERVAL) {
  //     lastTestConfigPayloadTime = millis();
  // #ifdef DEBUG_SERIAL_PRINT
  //     Serial.println(F("Sending a test configuration payload..."));
  // #endif // DEBUG_SERIAL_PRINT

  //     // Send a test configuration payload every 10 seconds for testing purposes
  //     LoraPayload pkt{
  //         .id     = EXPECTED_ID,
  //         .ts     = 123456, // Use current time in seconds as timestamp
  //         .type   = PayloadType::CONFIGURATION,
  //         .length = 4,
  //         .data   = "DEADBEEF", // Example configuration data
  //         .hmac   = "ABCD1234"  // Example HMAC, TODO: Compute real HMAC
  //     };
  //     String hex      = payloadToHex(pkt);
  //     String loraLine = "AT+TEST=TXLRPKT,\"" + hex + "\"";
  //     loraSerial.println(loraLine);
  //     delay(300);
  //     loraSerial.println("AT+TEST=RXLRPKT");
  //   }
}

/**
 * Non-blocking wait for LoRa module data and transmit the response to Serial for the Node RED server.
 */
void listenLora() {
  if (loraSerial.available()) {
    String loraLine = loraSerial.readStringUntil('\n');
    loraLine.trim();

#ifdef DEBUG_SERIAL_PRINT
    if (loraLine.startsWith("+TEST: RX \"")) {
      Serial.println(String("[LoRa] Payload received (raw): ") + loraLine);
    }
#endif // DEBUG_SERIAL_PRINT

    String json = loraToSerial(loraLine);
    if (json.length() > 0) {
      Serial.println(json);
    }
  }
}

/**
 * Non-blocking wait for Serial data and transmit the response to LoRa module.
 */
void listenSerial() {
  if (Serial.available()) {
    String serialLine = Serial.readStringUntil('\n');
    serialLine.trim();

#ifdef DEBUG_SERIAL_PRINT
    Serial.println(String("[Serial] Command received: ") + serialLine);
#endif // DEBUG_SERIAL_PRINT

    String loraLine = serialToLora(serialLine);
    if (loraLine.length() > 0) {
      loraSerial.println(loraLine);
      delay(300);
      loraSerial.println("AT+TEST=RXLRPKT");
    }
#ifdef DEBUG_SERIAL_PRINT
    else {
      Serial.println("[Serial] Invalid command, could not convert to LoRa.");
    }
#endif // DEBUG_SERIAL_PRINT
  }
}

/**
 * Converts a raw LoRa line (e.g. +TEST: RX,"<hex_data>") into a Json string representing the payload, or an empty string if the line is not recognized or the payload is invalid.
 * @param loraLine The raw line received from the LoRa module.
 * @return A Json string representing the payload, e.g. {"id":1,"seq":0,"ts":0,"type":2,"data":1}, or an empty string if the line is not recognized or the payload is invalid.
 */
String loraToSerial(const String& loraLine) {
  String json = "";

  // Expected format: +TEST: RX,"<hex_data>"
  if (loraLine.startsWith("+TEST: RX \"")) {
    int q1 = loraLine.indexOf('"');
    int q2 = loraLine.indexOf('"', q1 + 1);

    if (q1 >= 0 && q2 > q1) {
      String hexData = loraLine.substring(q1 + 1, q2);
#ifdef DEBUG_SERIAL_PRINT
      Serial.println(String("[LoRa] Hex payload extracted: ") + hexData);
#endif // DEBUG_SERIAL_PRINT
      LoraPayload pkt;

      if (hexToPayload(hexData, pkt)) {
        if (pkt.id == EXPECTED_ID) {
          json = payloadToJson(pkt);

#ifdef DEBUG_SERIAL_PRINT
          Serial.println(String("Converted LoRa hex to json Payload: ") + json);
#endif // DEBUG_SERIAL_PRINT
        }
      }
#ifdef DEBUG_SERIAL_PRINT
      else {
        Serial.println("Failed to convert hex to payload, invalid hex data.");
      }
#endif // DEBUG_SERIAL_PRINT
    }
  }

  return json;
}

/**
 * Converts a Json string representing a LoraPayload into a raw LoRa command line to send to the module, or an empty string if the Json is invalid.
 * @param serialLine The Json string received from Serial, e.g. {"id":1,"seq":0,"ts":0,"type":2,"data":1}
 * @return A raw LoRa command line to send to the module, e.g. AT+TEST=TXLRPKT,"<hex_data>", or an empty string if the Json is invalid.
 */
String serialToLora(const String& serialLine) {
  LoraPayload pkt = jsonToPayload(serialLine);
  printPayload(pkt);
  if (pkt.id == 0) {
    // Invalid payload, return empty string
    return "";
  }
  String hex      = payloadToHex(pkt);
  String loraLine = "AT+TEST=TXLRPKT,\"" + hex + "\"";
  return loraLine;
}

/**
 * Converts a hex string into a LoraPayload struct.
 * The hex string is expected to represent the binary data of the LoraPayload struct, with each byte represented as two hex characters (e.g. "0100000000003039" for a payload with id=1, seq=0, ts=0, type=CONFIGURATION, data=12345).
 * @param hex The hex string to convert.
 * @param pkt The output LoraPayload struct to fill with the converted data.
 * @return true if the conversion was successful, false if the hex string is invalid (e.g. wrong length, non-hex characters, etc.).
 */
bool hexToPayload(const String& hex, LoraPayload& pkt) {
  // Parse a 32-bit unsigned integer from big-endian hex (8 chars = 4 bytes).
  auto parseU32BE = [](const String& s, size_t offset) -> uint32_t {
    uint32_t value = 0;
    for (size_t i = 0; i < 4; i++) {
      String byteStr = s.substring(offset + i * 2, offset + i * 2 + 2);
      value          = (value << 8) | (uint32_t)strtoul(byteStr.c_str(), nullptr, 16);
    }
    return value;
  };

  size_t offset = 0;

  pkt.id = (uint8_t)strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16);
  offset += 2;
  pkt.ts = parseU32BE(hex, offset);
  offset += 8;
  pkt.type = static_cast<PayloadType>(strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16));
  offset += 2;
  pkt.length = (uint8_t)strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16);
  offset += 2;

  if (hex.length() - 8 < offset + pkt.length * 2) { // Invalid hex string, not enough data for the declared length
#ifdef DEBUG_SERIAL_PRINT
    Serial.println("Invalid hex string: not enough data for the declared length. Expected at least " + String(offset + pkt.length * 2) + String(" characters, but got ") + String(hex.length()) + " characters. offset: " + String(offset) + " length: " + String(pkt.length));
#endif // DEBUG_SERIAL_PRINT
    return false;
  }

  pkt.data = hex.substring(offset, offset + pkt.length * 2); // Each byte is represented by 2 hex characters
  offset += pkt.length * 2;
  pkt.hmac = hex.substring(offset, offset + 8); // Each byte is represented by 2 hex characters
  offset += 8;

  return true;
}

/**
 * Converts a LoraPayload struct into a Json string representation.
 * The Json string will have the format {"id":1,"seq":0,"ts":0,"type":2,"data":1}.
 * @param pkt The LoraPayload struct to convert.
 * @return A Json string representation of the payload.
 */
String payloadToJson(const LoraPayload& pkt) {
  String json = "{";
  json += "\"id\":" + String(pkt.id) + ",";
  json += "\"ts\":" + String(pkt.ts) + ",";
  json += "\"type\":" + String(static_cast<uint8_t>(pkt.type)) + ",";
  json += "\"length\":" + String(pkt.length) + ",";
  json += "\"data\":\"" + pkt.data + "\",";
  json += "\"hmac\":\"" + String(pkt.hmac) + "\"";
  json += "}";
  return json;
}

/**
 * Converts a Json string representing a LoraPayload back into a LoraPayload struct.
 * @param json The Json string to convert, e.g. {"id":1,"seq":0,"ts":0,"type":2,"data":1}
 * @return The corresponding LoraPayload struct. If the Json is invalid or missing fields, the returned struct will have default values (0 or equivalent).
 */
LoraPayload jsonToPayload(const String& json) {
  // Expected LoraPayload in Json, e.g. {"id":1,"seq":0,"ts":0,"type":2,"data":1}
  LoraPayload pkt = {
      .id     = 0,
      .ts     = 0,
      .type   = PayloadType::UNKNOWN, // Default to UNKNOWN
      .length = 1,                    // Default to 1
  };
  pkt.data = "00"; // Default to 0

  // Extract id
  int pos = json.indexOf("\"id\":");
  if (pos >= 0) {
    int    end = json.indexOf(",", pos);
    String val = json.substring(pos + 5, end);
    pkt.id     = (uint8_t)strtol(val.c_str(), nullptr, 10);
  }

  // Extract ts
  pos = json.indexOf("\"ts\":");
  if (pos >= 0) {
    int    end = json.indexOf(",", pos);
    String val = json.substring(pos + 5, end);
    pkt.ts     = (uint32_t)strtol(val.c_str(), nullptr, 10);
  }

  // Extract type
  pos = json.indexOf("\"type\":");
  if (pos >= 0) {
    int    end = json.indexOf(",", pos);
    String val = json.substring(pos + 7, end);
    pkt.type   = static_cast<PayloadType>(strtol(val.c_str(), nullptr, 10));
  }

  // Extract length
  pos = json.indexOf("\"length\":");
  if (pos >= 0) {
    int    end = json.indexOf(",", pos + 9);
    String val = json.substring(pos + 9, end);
    pkt.length = (uint8_t)strtol(val.c_str(), nullptr, 10);
  }

  // Extract data
  pos = json.indexOf("\"data\":\"");
  if (pos >= 0) {
    int    end = json.indexOf("\"", pos + 8);
    String val = json.substring(pos + 8, end);
    pkt.data   = val;
  }

  // Extract hmac
  pos = json.indexOf("\"hmac\":\"");
  if (pos >= 0) {
    int end  = json.indexOf("\"", pos + 8);
    pkt.hmac = json.substring(pos + 8, end);
  }

  if (pkt.length > MAX_PAYLOAD_DATA_SIZE) {
    pkt.length = MAX_PAYLOAD_DATA_SIZE; // Truncate if length exceeds max
  } else if (pkt.length == 0) {
    pkt.length = 1;    // Default to 1 if length is 0
    pkt.data   = "00"; // Default to "00"
  }

  return pkt;
}

/**
 * Converts a LoraPayload struct into a hex string representation of its binary data, where each byte is represented as two hex characters (e.g. "0100000000003039" for a payload with id=1, seq=0, ts=0, type=CONFIGURATION, data=12345).
 * @param pkt The LoraPayload struct to convert.
 * @return A hex string representation of the payload's binary data.
 */
String payloadToHex(const LoraPayload& pkt) {
  // Append a single byte as two uppercase hex characters.
  auto appendHexByte = [](String& out, uint8_t b) {
    const char* hexChars = "0123456789ABCDEF";
    out += hexChars[b >> 4];
    out += hexChars[b & 0x0F];
  };
  auto appendHexBytesFromHexString = [](String& out, String hexStr, size_t maxBytes) {
    const char* hexChars = "0123456789ABCDEF";
    for (size_t i = 0; i < hexStr.length() && i < maxBytes * 2; i += 2) {
      String  byteStr = hexStr.substring(i, i + 2);
      uint8_t byteVal = (uint8_t)strtoul(byteStr.c_str(), nullptr, 16);
      out += hexChars[byteVal >> 4];
      out += hexChars[byteVal & 0x0F];
    }
  };

  String hex;
  hex.reserve(sizeof(LoraPayload) * 2);

  appendHexByte(hex, pkt.id);

  appendHexByte(hex, (uint8_t)((pkt.ts >> 24) & 0xFF));
  appendHexByte(hex, (uint8_t)((pkt.ts >> 16) & 0xFF));
  appendHexByte(hex, (uint8_t)((pkt.ts >> 8) & 0xFF));
  appendHexByte(hex, (uint8_t)(pkt.ts & 0xFF));

  appendHexByte(hex, static_cast<uint8_t>(pkt.type));

  appendHexByte(hex, (uint8_t)(pkt.length & 0xFF));

  appendHexBytesFromHexString(hex, pkt.data, pkt.length);

  appendHexBytesFromHexString(hex, pkt.hmac, 4);

  return hex;
}

void printPayload(const LoraPayload& pkt) {
  Serial.println(F("[LoRa] Payload received:"));

  Serial.print("  ID: ");
  Serial.println(pkt.id);
  Serial.print("  TS: ");
  Serial.println(pkt.ts);
  Serial.print("  Type: ");
  Serial.println(static_cast<uint8_t>(pkt.type), HEX);
  Serial.print("  Length: ");
  Serial.println(pkt.length, HEX);
  Serial.print("  Data: ");
  Serial.println(pkt.data);
  Serial.print("  HMAC: ");
  Serial.println(pkt.hmac);
}
