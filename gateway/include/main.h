#ifndef MAIN_H
#define MAIN_H

#include <SoftwareSerial.h>

enum class PayloadType : uint8_t {
  UNKNOWN        = 0x00, // Bad data
  EDGE_HEARTBEAT = 0x01, // Heartbeat message sent periodically to indicate that the system is alive, with the current alarm state included in the payload data
  MOTION_STATE   = 0x02, // Message sent when motion is detected, with the motion state (e.g., detected or not detected) included in the payload data
  CONFIGURATION  = 0x03, // TODO: Add support for configuration messages (e.g., changing the secret combination, time range rules, etc.)
};

#define MAX_PAYLOAD_DATA_SIZE 200

struct LoraPayload {
  uint8_t     id;     // 1 byte : ID of the sender node, set by the sender
  uint32_t    ts;     // 4 bytes : Unix timestamp of when the payload was created, set by the sender
  PayloadType type;   // 1 byte : Type of the payload (e.g., heartbeat, motion state, configuration, etc.), set by the sender
  uint8_t     length; // 1 byte : Length of the payload data in bytes, set by the sender
  String      data;   // Represents MAX_PAYLOAD_DATA_SIZE bytes : Payload data as hexadecimal string, content depends on the payload type, set by the sender
  String      hmac;   // Represents 4 bytes : HMAC signature of the payload for integrity and authenticity verification
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