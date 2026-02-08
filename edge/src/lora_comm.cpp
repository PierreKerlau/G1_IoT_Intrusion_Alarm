#include "lora_comm.h"
#include <Arduino.h>


static const char* LORA_RFCFG_CMD = "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF";

static const uint8_t LORA_NODE_ID = 1; // TODO: Configurable ID

static uint32_t g_seq = 0; // TODO: Remove

// Indique si le module LoRa a été initialisé correctement
bool LORA_WORKING = false;

static String payloadToHex(const LoraPayload& pkt) {
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

void setupLora() {
  Serial1.begin(9600);
  delay(500);

  Serial1.println("AT+MODE=TEST");
  delay(500);
  Serial1.println(LORA_RFCFG_CMD);
  delay(500);

  // Vérification de la réponse du module
  if (!waitRespAny("RFCFG", "OK", 4000)) {
    Serial.println(F("[LoRa] Erreur : le module ne répond pas correctement."));
    LORA_WORKING = false;
    return;
  }
  LORA_WORKING = true;

  Serial.println(F("[LoRa] Module initialisé en mode TEST."));
}

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
  if (!LORA_WORKING) {
    Serial.println(F("[LoRa] Module non opérationnel, envoi annulé."));
    return;
  }

  LoraPayload pkt;
  pkt.id   = LORA_NODE_ID;
  pkt.seq  = g_seq++;
  pkt.ts   = millis();
  pkt.data = state;
  pkt.type = PayloadType::MOTION_STATE;

  sendPayload(pkt);
}

void sendPayload(const LoraPayload& pkt) {
  String hex = payloadToHex(pkt);

  String cmd = "AT+TEST=TXLRPKT,\"" + hex + "\"";

  Serial.print(F("[LoRa] Envoi payload: "));
  Serial.println(cmd);

  Serial1.println(cmd);
}