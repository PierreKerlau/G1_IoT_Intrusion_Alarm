#ifndef LORA_COMM_H
#define LORA_COMM_H

#include <Arduino.h>

struct LoraPayload {
  uint8_t  id;
  uint32_t seq;
  uint32_t ts;
  uint8_t  motionStatus;
} __attribute__((packed));

void setupLora();

void loraSendSensorState(int state);

#endif
