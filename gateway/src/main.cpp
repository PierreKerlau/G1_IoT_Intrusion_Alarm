#include <SoftwareSerial.h>
#include "main.h"

// --- CONFIGURATION ---
#define LORA_RX 8
#define LORA_TX 9
#define EXPECTED_ID 1 // TODO: Configurable ID

#define LORA_RFCFG_CMD "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF"

SoftwareSerial loraSerial(LORA_RX, LORA_TX);

void setup() {
  Serial.begin(115200);
  loraSerial.begin(9600);

  Serial.println(F("--- Initialisation du Récepteur ---"));

  // Initialisation AT simple
  loraSerial.println("AT+MODE=TEST");
  delay(500);
  loraSerial.println(LORA_RFCFG_CMD);
  delay(500);
  loraSerial.println("AT+TEST=RXLRPKT");

  Serial.println(F("Écoute en cours (Fréq: 868.1MHz)..."));
}

void loop() {
  if (loraSerial.available()) {
    String line = loraSerial.readStringUntil('\n');
    line.trim();

    if (line.indexOf("+TEST: RX") >= 0) {
      int q1 = line.indexOf('"');
      int q2 = line.indexOf('"', q1 + 1);

      if (q1 >= 0 && q2 > q1) {
        String hexData = line.substring(q1 + 1, q2);
        LoraPayload pkt;

        if (hexToPayload(hexData, pkt)) {
          if (pkt.id == EXPECTED_ID) {
            String json = payloadToJSON(pkt);
            Serial.println(json);
          }
        }
      }
    }
  }
}

bool hexToPayload(const String &hex, LoraPayload &pkt) {
  if (hex.length() < (sizeof(LoraPayload) * 2)) return false;

  uint8_t *ptr = (uint8_t *)&pkt;

  for (size_t i = 0; i < sizeof(LoraPayload); i++){
    String byteStr = hex.substring(i * 2, i * 2 + 2);
    ptr[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
  }
  return true;
}

String payloadToJSON(const LoraPayload &pkt) {
  String json = "{";
  json += "\"id\":" + String(pkt.id) + ",";
  json += "\"seq\":" + String(pkt.seq) + ",";
  json += "\"ts\":" + String(pkt.ts) + ",";
  json += "\"type\":" + String(static_cast<uint8_t>(pkt.type)) + ",";
  json += "\"data\":" + String(pkt.data);
  json += "}";
  return json;
}
