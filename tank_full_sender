#include <LoRa.h>

// Pin Definitions
#define TRIG_PIN 4       // Ultrasonic sensor trigger pin
#define ECHO_PIN 16      // Ultrasonic sensor echo pin
#define LORA_SS 5        // LoRa module SS pin
#define LORA_RST 14      // LoRa module reset pin
#define LORA_DIO0 2      // LoRa module DIO0 pin

// LoRa Frequency
#define LORA_FREQUENCY 433E6 // 433 MHz

// Tank Full Threshold (in cm)
const float TANK_FULL_THRESHOLD = 30.0; // Adjust based on your tank height

// Function Prototypes
float measureDistance();
void sendTankFullMessage();

void setup() {
  Serial.begin(115200);

  // Initialize Ultrasonic Sensor Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize LoRa
  Serial.println("Initializing LoRa...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa initialization failed. Check connections.");
    while (1);
  }
  Serial.println("LoRa initialized.");
}

void loop() {
  // Measure the distance using the ultrasonic sensor
  float distance = measureDistance();
  Serial.print("Measured Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Check if the tank is full
  if (distance <= TANK_FULL_THRESHOLD) {
    Serial.println("Tank is full. Sending TANK_FULL message...");
    sendTankFullMessage();
    delay(10000); // Avoid sending the message repeatedly
  }

  delay(1000); // Measure every second
}

// Function to measure distance using the RCWL-1655 ultrasonic sensor
float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2; // Convert to cm
  return distance;
}

// Function to send the "TANK_FULL" message via LoRa
void sendTankFullMessage() {
  LoRa.beginPacket();
  LoRa.print("TANK_FULL");
  LoRa.endPacket();
  Serial.println("TANK_FULL message sent.");
}
