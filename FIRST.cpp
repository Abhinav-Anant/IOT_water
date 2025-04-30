#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// WiFi credentials
String ssid;
String password;

// Pin definitions (adjust these to match your hardware)
const int relayMotorPin = 26;  // Motor relay pin
const int relayValvePin = 27;  // Solenoid valve relay pin
const int flowSensorPin = 34;  // Flow sensor pin
const int trigPin = 25;        // Ultrasonic sensor trigger pin
const int echoPin = 33;        // Ultrasonic sensor echo pin

// Flow sensor variables
volatile unsigned long flowPulseCount = 0;
volatile unsigned long lastPulseTime = 0;
float flowRate = 0.0;
unsigned long oldTime = 0;
const float calibrationFactor = 4.5; // Adjust based on your flow sensor

// Time zone settings (adjust these for your local time)
const long gmtOffset_sec = 19800;      // For UTC; e.g., use 19800 for UTC+5:30 (India)
const int daylightOffset_sec = 0;  // Set to 3600 if your region uses daylight saving

unsigned long previousMillis = 0;
const long interval = 1000; // 1 second

const float tankFullThreshold = 15.24; // 6 inches

// Function prototypes
void performFilling();
void calculateFlowRate();
float measureDistance();
void IRAM_ATTR flowPulseCounter();
void connectToWiFi();
void syncTime();

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(relayMotorPin, OUTPUT);
  pinMode(relayValvePin, OUTPUT);
  pinMode(flowSensorPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set relays to OFF initially
  digitalWrite(relayMotorPin, LOW);
  digitalWrite(relayValvePin, LOW);

  // Attach interrupt for flow sensor
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowPulseCounter, RISING);

  // Connect to WiFi
  connectToWiFi();

  // Synchronize time
  syncTime();
}

void loop() {
  // Get current local time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    delay(1000);
    return;
  }

  // Check if it's time to start filling (once per day at or after 5 AM)
  static int lastActionDay = -1;
  if (timeinfo.tm_hour >= 5 && timeinfo.tm_mday != lastActionDay) {
    Serial.println("Starting filling process at 5 AM");
    lastActionDay = timeinfo.tm_mday;
    performFilling();
  }

  // Small delay to prevent excessive looping
  delay(1000);

  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    // Perform periodic task
  }
}

void performFilling() {
  bool waterDetected = false;

  // Retry until water is detected
  while (!waterDetected) {
    // Turn on motor and open valve
    digitalWrite(relayMotorPin, HIGH);
    digitalWrite(relayValvePin, HIGH);
    Serial.println("Motor ON, Valve OPEN");

    // Check for water flow for 30 seconds
    unsigned long startTime = millis();
    while (millis() - startTime < 30000) {
      calculateFlowRate();
      Serial.print("Flow Rate: ");
      Serial.print(flowRate);
      Serial.println(" L/min");
      if (flowRate > 0) {
        waterDetected = true;
        break;
      }
      delay(1000);
    }

    if (!waterDetected) {
      // No water detected: turn off and wait 5 minutes
      digitalWrite(relayMotorPin, LOW);
      digitalWrite(relayValvePin, LOW);
      Serial.println("No water detected. Motor OFF, Valve CLOSED. Retrying in 5 minutes...");
      delay(300000); // 5 minutes (300,000 ms)
    }
  }

  // Water is flowing: monitor tank level and flow
  Serial.println("Water flowing. Monitoring level and flow...");
  unsigned long lastFlowTime = millis();
  while (true) {
    float distance = measureDistance();
    calculateFlowRate();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Flow Rate: ");
    Serial.print(flowRate);
    Serial.println(" L/min");

    // Check if tank is full (water 6 inches or 15.24 cm from sensor)
    if (distance < tankFullThreshold) {
      digitalWrite(relayMotorPin, LOW);
      digitalWrite(relayValvePin, LOW);
      Serial.println("Tank full. Motor OFF, Valve CLOSED");
      break;
    }

    // Check if no flow for 30 seconds
    if (flowRate > 0) {
      lastFlowTime = millis();
    } else if (millis() - lastFlowTime >= 30000) {
      digitalWrite(relayMotorPin, LOW);
      digitalWrite(relayValvePin, LOW);
      Serial.println("No flow for 30 seconds. Motor OFF, Valve CLOSED");
      break;
    }
    delay(1000);
  }
}

// Calculate flow rate in liters per minute
void calculateFlowRate() {
  if (millis() - oldTime > 1000) {
    // Only calculate flow rate if the time difference is reasonable
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    flowRate = ((1000.0 / (millis() - oldTime)) * flowPulseCount) / calibrationFactor;
    oldTime = millis();
    flowPulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowPulseCounter, RISING);
  }
}

// Measure distance using ultrasonic sensor in centimeters
float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2; // Speed of sound = 340 m/s
  return distance;
}

// Interrupt service routine for flow sensor pulses
void IRAM_ATTR flowPulseCounter() {
  unsigned long currentTime = millis();
  if (currentTime - lastPulseTime > 10) { // 10 ms debounce
    flowPulseCount++;
    lastPulseTime = currentTime;
  }
}

// Connect to WiFi with user input
void connectToWiFi() {
  Serial.println("Enter WiFi SSID:");
  while (Serial.available() == 0) {}
  ssid = Serial.readStringUntil('\n');
  ssid.trim();

  Serial.println("Enter WiFi Password:");
  while (Serial.available() == 0) {}
  password = Serial.readStringUntil('\n');
  password.trim();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi. Retrying in 5 minutes...");
    delay(300000); // Retry after 5 minutes
    ESP.restart();
  }
  Serial.println("Connected to WiFi");
}

// Synchronize time with NTP server
void syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  // Wait for time synchronization
  struct tm timeinfo;
  unsigned long startSyncTime = millis();
  while (!getLocalTime(&timeinfo) && millis() - startSyncTime < 30000) {
    delay(1000);
    Serial.println("Waiting for time sync...");
  }
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to synchronize time. Proceeding without time sync...");
  } else {
    Serial.println("Time synchronized");
  }
}