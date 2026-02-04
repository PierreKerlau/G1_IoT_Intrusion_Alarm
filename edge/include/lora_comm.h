#ifndef LORA_COMM_H
#define LORA_COMM_H

#include <Arduino.h>

enum class PayloadType : uint8_t {
  MOTION_STATE = 0x01,
};

struct LoraPayload {
  uint8_t  id;
  uint32_t seq; // TODO: Remove
  uint32_t ts;
  PayloadType type;
  uint32_t  data;
  // TODO: Add cryptographic signature
} __attribute__((packed));

void setupLora();

void loraSendMotionState(bool state);

bool waitRespAny(const char *expectedResponse1, const char *expectedResponse2, uint32_t timeoutMs);

void sendPayload(const LoraPayload &pkt);

#endif
