#include <SoftwareSerial.h>

// --- CONFIGURATION ---
#define LORA_RX 8
#define LORA_TX 9
#define EXPECTED_ID 1

#define LORA_RFCFG_CMD "AT+TEST=RFCFG,868.1,SF7,125,8,15,14,ON,OFF,OFF"

SoftwareSerial loraSerial(LORA_RX, LORA_TX);

struct Payload
{
  uint8_t id;
  uint32_t seq;
  uint32_t ts;
  uint8_t motionStatus;
} __attribute__((packed));

void setup()
{
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

void loop()
{
  if (loraSerial.available())
  {
    String line = loraSerial.readStringUntil('\n');
    line.trim();

    if (line.indexOf("+TEST: RX") >= 0)
    {
      int q1 = line.indexOf('"');
      int q2 = line.indexOf('"', q1 + 1);

      if (q1 >= 0 && q2 > q1)
      {
        String hexData = line.substring(q1 + 1, q2);
        Payload pkt;

        if (hexToPayload(hexData, pkt))
        {
          if (pkt.id == EXPECTED_ID)
          {
            // Serial.print(F("[CAPTEUR] Statut mouvement : "));
            // Serial.println(pkt.motionStatus);
            Serial.println(
                String("{\"id\":") + pkt.id + String(",\"seq\":") + pkt.seq + String(",\"ts\":") + pkt.ts + String(",\"motionStatus\":") + pkt.motionStatus + String("}"));
          }
        }
      }
    }
  }
}

bool hexToPayload(const String &hex, Payload &pkt)
{
  if (hex.length() < (sizeof(Payload) * 2))
    return false;
  uint8_t *ptr = (uint8_t *)&pkt;
  for (size_t i = 0; i < sizeof(Payload); i++)
  {
    String byteStr = hex.substring(i * 2, i * 2 + 2);
    ptr[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
  }
  return true;
}