#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "rtc.h"
#include <Arduino.h>

// Forward declaration to avoid circular dependency
enum class AlarmState;

enum class PayloadType : uint8_t {
  UNKNOWN        = 0x00, // Bad data
  EDGE_HEARTBEAT = 0x01, // Heartbeat message sent periodically to indicate that the system is alive, with the current alarm state included in the payload data
  MOTION_STATE   = 0x02, // Message sent when motion is detected, with the motion state (e.g., detected or not detected) included in the payload data
  CONFIGURATION  = 0x03, // TODO: Add support for configuration messages (e.g., changing the secret combination, time range rules, etc.)
};

struct LoraPayload {
  uint8_t     id;
  uint32_t    seq; // TODO: Remove
  uint32_t    ts;  // Unix timestamp of when the payload was created, set by the sender
  PayloadType type;
  uint32_t    data;
  // TODO: Add cryptographic signature
} __attribute__((packed));

void setupLora();
void loraSendMotionState(bool state);
void loraSendHeartbeat(AlarmState state);

LoraPayload listenForPayload();

#endif // LORA_COMM_H
