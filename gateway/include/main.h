#ifndef MAIN_H
#define MAIN_H

#include <SoftwareSerial.h>

enum class PayloadType : uint8_t {
  EDGE_HEARTBEAT = 0x01, // Heartbeat message sent periodically to indicate that the system is alive, with the current alarm state included in the payload data
  MOTION_STATE   = 0x02, // Message sent when motion is detected, with the motion state (e.g., detected or not detected) included in the payload data
  CONFIGURATION  = 0x03, // TODO: Add support for configuration messages (e.g., changing the secret combination, time range rules, etc.)
};

struct LoraPayload {
  uint8_t     id;
  uint32_t    seq;
  uint32_t    ts;
  PayloadType type;
  uint32_t    data;
  // TODO: Add cryptographic signature
} __attribute__((packed));

void listenLora();
void listenSerial();

// Lora (hex) -> LoraPayload -> Json -> Serial (Json)
String loraToSerial(const String& loraLine);
bool   hexToPayload(const String& hex, LoraPayload& pkt);
String payloadToJson(const LoraPayload& pkt);

// Serial (Json) -> Json -> LoraPayload ->Lora (hex)
String      serialToLora(const String& serialLine);
LoraPayload jsonToPayload(const String& json);
String      payloadToHex(const LoraPayload& pkt);

#endif // MAIN_H