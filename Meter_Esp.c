#include <LoRa.h>
#include <HardwareSerial.h>

// RS485 Configuration
#define RS485_RX_PIN 16
#define RS485_TX_PIN 17
#define RS485_DE_RE_PIN 4
HardwareSerial rs485(2); // Use UART2

// LoRa Configuration
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5
#define LORA_RST 14
#define LORA_DIO0 2

#define MODBUS_BALANCE_ADDR 0x504D
#define MODBUS_VOLTAGE_R_ADDR 0x5037
#define MODBUS_VOLTAGE_Y_ADDR 0x5038
#define MODBUS_VOLTAGE_B_ADDR 0x5039

#pragma pack(push, 1)
struct MeterData {
  float voltageR, voltageY, voltageB;
  float currentR, currentY, currentB;
  float pfR, pfY, pfB;
  float frequency;
  float kwR, kwY, kwB;
  float kvaR, kvaY, kvaB;
  float balance;
};
#pragma pack(pop)

void setup() {
  Serial.begin(115200);
  rs485.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);

  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  int retryCount = 0;
  while (!LoRa.begin(433E6) && retryCount < 5) {
    Serial.println("LoRa init failed. Retrying...");
    delay(1000);
    retryCount++;
  }
  if (retryCount == 5) {
    Serial.println("LoRa init failed. Entering fallback mode.");
    while (1);
  }

  Serial.printf("MeterData size: %d bytes\n", sizeof(MeterData));
}

uint16_t modbusCRC(uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool readModbus(uint8_t slaveID, uint16_t addr, uint16_t regs, uint16_t *result) {
  uint8_t frame[8];
  frame[0] = slaveID;
  frame[1] = 0x03; // Read holding registers
  frame[2] = highByte(addr);
  frame[3] = lowByte(addr);
  frame[4] = highByte(regs);
  frame[5] = lowByte(regs);
  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = lowByte(crc);
  frame[7] = highByte(crc);

  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(10); // Ensure direction switch
  rs485.write(frame, 8);
  rs485.flush();
  digitalWrite(RS485_DE_RE_PIN, LOW);

  delay(100);
  
  uint8_t response[256];
  int len = rs485.readBytes(response, 256);
  if (len < 5 + 2*regs) {
    Serial.println("Modbus response too short.");
    return false;
  }
  
  crc = modbusCRC(response, len - 2);
  if (lowByte(crc) != response[len-2] || highByte(crc) != response[len-1]) 
    return false;

  for (uint16_t i=0; i<regs; i++) {
    result[i] = (response[3 + 2*i] << 8) | response[4 + 2*i];
  }
  return true;
}

void sendRelayCommand(bool state) {
  uint8_t cmd[10] = {0x13, 0x00, 0x02, 0x00, 0x01, 0x02, 
                    state ? 0x50 : 0x00, state ? 0x50 : 0x00, 0x00, 0x00};
  uint16_t crc = modbusCRC(cmd, 8);
  cmd[8] = lowByte(crc);
  cmd[9] = highByte(crc);

  digitalWrite(RS485_DE_RE_PIN, HIGH);
  rs485.write(cmd, 10);
  rs485.flush();
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

unsigned long previousMillis = 0;
const unsigned long interval = 10000;

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Perform Modbus read and LoRa transmission
    MeterData data;
    uint16_t regValue;

    if (readModbus(0x13, MODBUS_BALANCE_ADDR, 1, &regValue)) {
      data.balance = regValue;
      if (data.balance == 0) sendRelayCommand(false);
    }

    // Read other parameters and send via LoRa
    if (readModbus(0x13, MODBUS_VOLTAGE_R_ADDR, 1, &regValue)) data.voltageR = regValue / 10.0;
    if (readModbus(0x13, MODBUS_VOLTAGE_Y_ADDR, 1, &regValue)) data.voltageY = regValue / 10.0;
    if (readModbus(0x13, MODBUS_VOLTAGE_B_ADDR, 1, &regValue)) data.voltageB = regValue / 10.0;

    LoRa.beginPacket();
    LoRa.write((uint8_t*)&data, sizeof(data));
    LoRa.endPacket();
  }
}
