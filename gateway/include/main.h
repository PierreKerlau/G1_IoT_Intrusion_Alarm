enum class PayloadType : uint8_t {
    SENSOR_STATE = 0x01,
};

struct LoraPayload {
    uint8_t id;
    uint32_t seq;
    uint32_t ts;
    PayloadType type;
    uint32_t data;
    // TODO: Add cryptographic signature
} __attribute__((packed));

bool hexToPayload(const String &hex, LoraPayload &pkt);
String payloadToJSON(const LoraPayload &pkt);