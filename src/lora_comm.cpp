#include <Arduino.h>
#include <lora_comm.h>

static const char* LORA_RFCFG_CMD =
  "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF";

static const uint8_t LORA_NODE_ID = 1;

static uint32_t g_seq = 0;

static String payloadToHex(const LoraPayload& pkt) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&pkt);
  String hex;
  hex.reserve(sizeof(LoraPayload) * 2);

  for (size_t i = 0; i < sizeof(LoraPayload); i++) {
    uint8_t b = ptr[i];
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

  Serial.println(F("[LoRa] Module initialisé en mode TEST."));
}

void loraSendSensorState(int state) {
  LoraPayload pkt;
  pkt.id = LORA_NODE_ID;
  pkt.seq = g_seq++;
  pkt.ts = millis();
  pkt.motionStatus = state ? 1 : 0;

  String hex = payloadToHex(pkt);

  String cmd = "AT+TEST=TXLRPKT,\"" + hex + "\"";

  Serial.print(F("[LoRa] Envoi payload: "));
  Serial.println(cmd);

  Serial1.println(cmd);
}
