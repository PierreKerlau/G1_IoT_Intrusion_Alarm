#include "lora_comm.h"

static const char* LORA_RFCFG_CMD = "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF";

static const uint8_t LORA_NODE_ID = 1; // TODO: Configurable ID

const String HMAC_KEY = "b5df4g1ds14b1ds4fdsv5dsfvdsbds"; // TODO: Use a proper secret key management strategy

// Store the state of the LoRa module initialization
bool lora_working = false;

String payloadToHex(const LoraPayload& pkt);
bool   waitRespAny(const char* expectedResponse1, const char* expectedResponse2, uint32_t timeoutMs);
void   sendPayload(LoraPayload& pkt);

uint32_t parseU32BE(const String& s, size_t offset);
bool     hexToPayloadBE(const String& hex, LoraPayload& pkt);
void     appendHexByte(String& out, uint8_t b);

uint32_t computeHMAC(const LoraPayload& pkt);
bool     verifyHMAC(const LoraPayload& pkt);

void setupLora() {
  Serial1.begin(9600);
  delay(500);

  Serial1.println("AT+MODE=TEST");
  delay(500);
  Serial1.println(LORA_RFCFG_CMD);
  delay(500);

  // Verify the module response
  if (!waitRespAny("RFCFG", "OK", 4000)) {
    Serial.println(F("[LoRa] Error: the module is not responding correctly."));
    lora_working = false;
    return;
  }
  lora_working = true;

  Serial.println(F("[LoRa] Module initialized in TEST mode."));
}

// TODO: Retry lora begin with an interval

/**
 * Listen for incoming LoRa payloads.
 * @return The received payload, or an empty payload if no valid payload was received.
 */
LoraPayload listenForPayload() {
  if (!lora_working) {
    Serial.println(F("[LoRa] Module not operational, listening cancelled."));
    return LoraPayload{};
  }

  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();

    if (line.startsWith("+TEST: RX \"")) {
      Serial.println(F("[LoRa] Raw Payload received:"));
      Serial.println(line);

      String hex;

      // Parse format: +TEST: RX,"<hex_data>"
      int q1 = line.indexOf('"');
      int q2 = line.indexOf('"', q1 + 1);
      if (q1 >= 0 && q2 > q1) {
        hex = line.substring(q1 + 1, q2);
      } else {
        return LoraPayload{}; // Invalid data
      }

      if (hex.length() >= (1 + 4 + 1 + 1 + 4) * 2) { // 2 hex characters = 1 byte, minimum length for a valid payload (id + ts + type + length + hmac)
        LoraPayload pkt;
        if (!hexToPayloadBE(hex, pkt)) {
          return LoraPayload{}; // Invalid data
        }
        printPayload(pkt);
        if (pkt.id != LORA_NODE_ID) {
          Serial.println("[LoRa] Invalid node ID: " + String(pkt.id));
          return LoraPayload{};
        }
        if (!verifyHMAC(pkt)) {
          Serial.println("[LoRa] HMAC verification failed.");
          return LoraPayload{};
        }
        Serial.println("[LoRa] HMAC verification succeeded.");
        return pkt;
      } else {
        Serial.println("[LoRa] Invalid payload length: " + String(hex.length()) + " (expected " + String(sizeof(LoraPayload) * 2) + ")");
      }
    }
  }
  return LoraPayload{};
}

/**
 * Blocks until a response containing either of the two expected responses is received, or until timeoutMs is reached.
 * @param expectedResponse1 The first expected response string to wait for.
 * @param expectedResponse2 The second expected response string to wait for.
 * @param timeoutMs The timeout in milliseconds to wait for a response.
 * @return true if a response containing expectedResponse1 or expectedResponse2 is received, false if timeout is reached or if an error response is received.
 */
bool waitRespAny(const char* expectedResponse1, const char* expectedResponse2, uint32_t timeoutMs) {
  uint32_t start = millis();
  String   resp;

  while (millis() - start < timeoutMs) {
    while (Serial1.available()) {
      char c = (char)Serial1.read();
      resp += c;

      if (resp.indexOf("ERROR") >= 0 || resp.indexOf("FAIL") >= 0) {
        Serial.print(F("<< "));
        Serial.println(resp);
        return false;
      }

      if ((expectedResponse1 && resp.indexOf(expectedResponse1) >= 0) || (expectedResponse2 && resp.indexOf(expectedResponse2) >= 0)) {
        Serial.print(F("<< "));
        Serial.println(resp);
        return true;
      }
    }
  }

  if (resp.length() > 0) {
    Serial.print(F("<< (timeout) "));
    Serial.println(resp);
  }
  return false;
}

/**
 * Send a motion sensor detection packet.
 * @param state The current state of the motion sensor.
 */
void loraSendMotionState(bool state) {
  if (!lora_working) {
    Serial.println(F("[LoRa] Module not operational, send cancelled."));
    return;
  }

  uint32_t unixTime = getCurrentUnixTime();

  LoraPayload pkt;
  pkt.id      = LORA_NODE_ID;
  pkt.ts      = unixTime;
  pkt.type    = PayloadType::MOTION_STATE;
  pkt.length  = 1;
  pkt.data[0] = state ? 1 : 0;

  sendPayload(pkt);
}

/**
 * Send a heartbeat payload through LoRa.
 * @param state The current state of the alarm.
 */
void loraSendHeartbeat(AlarmState state) {
  if (!lora_working) {
    Serial.println(F("[LoRa] Module not operational, send cancelled."));
    return;
  }

  uint32_t unixTime = getCurrentUnixTime();

  LoraPayload pkt;
  pkt.id      = LORA_NODE_ID;
  pkt.ts      = unixTime;
  pkt.type    = PayloadType::EDGE_HEARTBEAT;
  pkt.length  = 1;
  pkt.data[0] = static_cast<uint8_t>(state);

  sendPayload(pkt);
}

/**
 * Send the given payload through LoRa as hex data.
 * Add hmac signing in the future for integrity and authenticity verification.
 * @param pkt The payload to send.
 */
void sendPayload(LoraPayload& pkt) {
  pkt.hmac = computeHMAC(pkt);

  String hex = payloadToHex(pkt);

  String cmd = "AT+TEST=TXLRPKT,\"" + hex + "\"";

  Serial.print(F("[LoRa] Sending payload: "));
  Serial.println(cmd);

  Serial1.println(cmd);

  // After sending a message, we have to manually switch back to listening
  delay(300);
  Serial1.println("AT+TEST=RXLRPKT");
}

String payloadToHex(const LoraPayload& pkt) {
  String hex;
  hex.reserve(sizeof(LoraPayload) * 2);

  appendHexByte(hex, pkt.id);

  appendHexByte(hex, (uint8_t)((pkt.ts >> 24) & 0xFF));
  appendHexByte(hex, (uint8_t)((pkt.ts >> 16) & 0xFF));
  appendHexByte(hex, (uint8_t)((pkt.ts >> 8) & 0xFF));
  appendHexByte(hex, (uint8_t)(pkt.ts & 0xFF));

  appendHexByte(hex, static_cast<uint8_t>(pkt.type));

  appendHexByte(hex, (uint8_t)(pkt.length & 0xFF));

  for (size_t i = 0; i < pkt.length; i++) {
    appendHexByte(hex, pkt.data[i]);
  }

  appendHexByte(hex, (uint8_t)((pkt.hmac >> 24) & 0xFF));
  appendHexByte(hex, (uint8_t)((pkt.hmac >> 16) & 0xFF));
  appendHexByte(hex, (uint8_t)((pkt.hmac >> 8) & 0xFF));
  appendHexByte(hex, (uint8_t)(pkt.hmac & 0xFF));

  return hex;
}

/**
 * Append a single byte as two uppercase hex characters.
 * @param out The output string to append to.
 * @param b The byte to append as hex.
 */
void appendHexByte(String& out, uint8_t b) {
  const char* hexChars = "0123456789ABCDEF";
  out += hexChars[b >> 4];
  out += hexChars[b & 0x0F];
}

/**
 * Parse a 32-bit unsigned integer from big-endian hex (8 chars = 4 bytes).
 * @param s The hex string to parse.
 * @param offset The starting position in the string to parse from.
 * @return The parsed 32-bit unsigned integer.
 */
uint32_t parseU32BE(const String& s, size_t offset) {
  uint32_t value = 0;
  for (size_t i = 0; i < 4; i++) {
    String byteStr = s.substring(offset + i * 2, offset + i * 2 + 2);
    value          = (value << 8) | (uint32_t)strtoul(byteStr.c_str(), nullptr, 16);
  }
  return value;
}

/**
 * Convert a hex payload string into a LoraPayload using big-endian field order.
 * @param hex The hex string representing the payload.
 * @param pkt The LoraPayload object to populate.
 * @return true if the payload was successfully parsed, false otherwise.
 */
bool hexToPayloadBE(const String& hex, LoraPayload& pkt) {
  size_t offset = 0;
  pkt.id        = (uint8_t)strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16);
  offset += 2;
  pkt.ts = parseU32BE(hex, offset);
  offset += 8;
  uint32_t rawType = strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16);
  if (auto type = parsePayloadType(rawType)) {
    pkt.type = type.value();
  } else {
    Serial.println("[LoRa] Invalid payload type: " + String(rawType));
    return false;
  }
  offset += 2;
  pkt.length = (uint8_t)strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16);
  offset += 2;

  if (hex.length() < (1 + 4 + 1 + 1 + 4 + pkt.length) * 2) {
    Serial.println("[LoRa] Invalid hex length: " + String(hex.length()) + " (expected at least " + String((1 + 4 + 1 + 1 + 4 + pkt.length) * 2) + ")");
    return false; // Check expected length based on the length field
  }

  // Serial.print("[LoRa] Data: ");
  for (size_t i = 0; i < pkt.length; i++) {
    // Serial.print(hex.substring(offset, offset + 2));
    pkt.data[i] = (uint8_t)strtoul(hex.substring(offset, offset + 2).c_str(), nullptr, 16);
    offset += 2;
  }
  // Serial.println();

  pkt.hmac = parseU32BE(hex, offset);
  // Serial.println(pkt.hmac, HEX);
  offset += 8;

  return true;
}

uint32_t computeHMAC(const LoraPayload& pkt) {
  String data = String(pkt.id) + String(pkt.ts) + String(static_cast<uint8_t>(pkt.type)) + String(pkt.length);
  for (size_t i = 0; i < pkt.length; i++) {
    String byteStr = String(pkt.data[i], 16);
    byteStr.toUpperCase();
    for (size_t j = 0; j < 2 - byteStr.length(); j++) {
      data += "0";
    }
    data += byteStr;
  }

  // Serial.println("[HMAC] Computing HMAC for payload data (raw): " + String(pkt.id) + "," + String(pkt.ts) + "," + String(static_cast<uint8_t>(pkt.type)) + "," + String(pkt.length));
  // Serial.println("[HMAC] Computing HMAC for payload data (data): " + data);
  uint32_t hmac = 5381; // Initialize with a cryptographic magic number

  // DJB2 hash-like algorithm
  String mix = data + HMAC_KEY;
  // Serial.println("[HMAC] Computing HMAC for payload data (mix): " + mix);
  for (int i = 0; i < mix.length(); i++) {
    // Serial.println("[HMAC] Computing HMAC for payload data (hmac): i=" + String(i) + ", " + mix);
    hmac = ((hmac << 5) + hmac) + mix.charAt(i); // (hmac * 32 + hmac) + c = hmac * 33 + c
  }

  // Serial.println("[HMAC] HMAC result: " + String(hmac));
  return hmac;
}

bool verifyHMAC(const LoraPayload& pkt) {
  uint32_t computedHMAC = computeHMAC(pkt);
  // Serial.println("[HMAC] Verifying HMAC: computed=" + String(computedHMAC) + ", received=" + String(pkt.hmac));
  return computedHMAC == pkt.hmac;
}

void printPayload(const LoraPayload& pkt) {
  Serial.println(F("[LoRa] Payload:"));

  Serial.print("  ID: ");
  Serial.println(pkt.id);
  Serial.print("  TS: ");
  Serial.println(pkt.ts);
  Serial.print("  Type: ");
  Serial.println(static_cast<uint8_t>(pkt.type), HEX);
  Serial.print("  Length: ");
  Serial.println(pkt.length, HEX);
  String dataStr;
  for (size_t i = 0; i < pkt.length; i++) {
    if (pkt.data[i] < 0x10) dataStr += "0";
    dataStr += String(pkt.data[i], HEX);
  }
  dataStr.toUpperCase();
  Serial.print("  Data: ");
  Serial.println(dataStr);
  Serial.print("  HMAC: ");
  String hmacStr = String(pkt.hmac, HEX);
  hmacStr.toUpperCase();
  for (size_t i = hmacStr.length(); i < 8; i++) {
    hmacStr = "0" + hmacStr; // Pad with leading zeros if necessary
  }
  Serial.println(hmacStr);
}
