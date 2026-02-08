#include "lora_comm.h"

static const char* LORA_RFCFG_CMD = "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF";

static const uint8_t LORA_NODE_ID = 1; // TODO: Configurable ID

static uint32_t g_seq = 0; // TODO: Remove

// Store the state of the LoRa module initialization
bool lora_working = false;

String payloadToHex(const LoraPayload& pkt);
bool   waitRespAny(const char* expectedResponse1, const char* expectedResponse2, uint32_t timeoutMs);
void   sendPayload(const LoraPayload& pkt);

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
    Serial.println(F("[LoRa] Raw Payload received:"));
    Serial.println(line);

    if (line.startsWith("+TEST:RXLRPKT,")) {
      String hex = line.substring(strlen("+TEST:RXLRPKT,"));
      if (hex.length() == sizeof(LoraPayload) * 2) {
        LoraPayload pkt;
        for (size_t i = 0; i < sizeof(LoraPayload); i++) {
          String byteStr = hex.substring(i * 2, i * 2 + 2);
          pkt.id         = strtoul(byteStr.c_str(), nullptr, 16);
          pkt.seq        = strtoul(byteStr.c_str(), nullptr, 16);
          pkt.ts         = strtoul(byteStr.c_str(), nullptr, 16);
          pkt.type       = static_cast<PayloadType>(strtoul(byteStr.c_str(), nullptr, 16));
          pkt.data       = strtoul(byteStr.c_str(), nullptr, 16);
        }
        Serial.println(F("[LoRa] Payload received:"));
        Serial.print("  ID: ");
        Serial.println(pkt.id);
        Serial.print("  Seq: ");
        Serial.println(pkt.seq);
        Serial.print("  TS: ");
        Serial.println(pkt.ts);
        Serial.print("  Type: ");
        Serial.println(static_cast<uint8_t>(pkt.type), HEX);
        Serial.print("  Data: ");
        Serial.println(pkt.data, HEX);
        return pkt;
      }
    }
  }
  return LoraPayload{};
}

/**
 * Blocks until a response containing either expectedResponse1 or expectedResponse2 is received, or until timeoutMs is reached.
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

void loraSendMotionState(bool state) {
  if (!lora_working) {
    Serial.println(F("[LoRa] Module not operational, send cancelled."));
    return;
  }

  uint32_t unixTime = getCurrentUnixTime();

  LoraPayload pkt;
  pkt.id   = LORA_NODE_ID;
  pkt.seq  = g_seq++;
  pkt.ts   = unixTime;
  pkt.data = state;
  pkt.type = PayloadType::MOTION_STATE;

  sendPayload(pkt);
}

void loraSendHeartbeat(AlarmState state) {
  if (!lora_working) {
    Serial.println(F("[LoRa] Module not operational, send cancelled."));
    return;
  }

  uint32_t unixTime = getCurrentUnixTime();

  LoraPayload pkt;
  pkt.id   = LORA_NODE_ID;
  pkt.seq  = g_seq++;
  pkt.ts   = unixTime;
  pkt.data = static_cast<uint32_t>(state);
  pkt.type = PayloadType::EDGE_HEARTBEAT;

  sendPayload(pkt);
}

void sendPayload(const LoraPayload& pkt) {
  String hex = payloadToHex(pkt);

  String cmd = "AT+TEST=TXLRPKT,\"" + hex + "\"";

  Serial.print(F("[LoRa] Sending payload: "));
  Serial.println(cmd);

  Serial1.println(cmd);
}

String payloadToHex(const LoraPayload& pkt) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&pkt);
  String         hex;
  hex.reserve(sizeof(LoraPayload) * 2);

  for (size_t i = 0; i < sizeof(LoraPayload); i++) {
    uint8_t     b        = ptr[i];
    const char* hexChars = "0123456789ABCDEF";
    hex += hexChars[b >> 4];
    hex += hexChars[b & 0x0F];
  }
  return hex;
}
