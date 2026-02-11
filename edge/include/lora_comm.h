#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "rtc.h"
#include <Arduino.h>
#include <optional>

// Forward declarations to avoid circular dependency
enum class AlarmState;
enum class PayloadType : uint8_t;
std::optional<PayloadType> parsePayloadType(uint8_t raw);

enum class PayloadType : uint8_t {
  UNKNOWN         = 0x00, // 0: Bad data
  EDGE_HEARTBEAT  = 0x01, // 1: Edge    -> Heartbeat message sent periodically to indicate that the system is alive, with the current alarm state included in the payload data
  MOTION_STATE    = 0x02, // 2: Edge    -> Message sent when motion is detected, with the motion state (e.g., detected or not detected) included in the payload data
  SET_COMBINATION = 0x11, // 17: Broker -> Set the expected combination
  SET_TIME_RANGE  = 0x12, // 18: Broker -> Set the monitored time range
  SET_ALARM_STATE = 0x13, // 19: Broker -> Set the alarm state
  SET_RTC_TIME    = 0x14, // 20: Broker -> Set the RTC time
};

#define MAX_PAYLOAD_DATA_SIZE 200

struct LoraPayload {
  uint8_t     id;                          // 1 byte : ID of the sender node, set by the sender
  uint32_t    ts;                          // 4 bytes : Unix timestamp of when the payload was created, set by the sender
  PayloadType type;                        // 1 byte : Type of the payload (e.g., heartbeat, motion state, configuration, etc.), set by the sender
  uint8_t     length;                      // 1 byte : Length of the payload data in bytes, set by the sender
  uint8_t     data[MAX_PAYLOAD_DATA_SIZE]; // MAX_PAYLOAD_DATA_SIZE bytes : Payload data, content depends on the payload type, set by the sender
  uint32_t    hmac;                        // 4 bytes : HMAC signature of the payload for integrity and authenticity verification
} __attribute__((packed));

void setupLora();
void loraSendMotionState(bool state);
void loraSendHeartbeat(AlarmState state);
void printPayload(const LoraPayload& pkt);

LoraPayload listenForPayload();

#endif // LORA_COMM_H
