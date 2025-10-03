/**
 * ESP32 Power Monitoring System - Minimal Version
 * 
 * Reads SCT-013-000 current sensor and transmits data via HTTP
 * Simplified version with core functionality only
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Configuration
#define DEVICE_NAME "ESP32_Power_Monitor"
#define FIRMWARE_VERSION "1.0.0"

// Hardware pins
#define CURRENT_SENSOR_PIN 34       // ADC pin connected to SCT-013-000
#define LED_PIN 2                   // Built-in LED pin

// Power monitoring constants
#define MAINS_VOLTAGE 230.0         // Default mains voltage (V)
#define CT_BURDEN_RESISTOR 33.0     // Burden resistor value (ohms)
#define CT_TURNS_RATIO 2000.0       // SCT-013-000 turns ratio
#define ADC_BITS 12                 // ESP32 ADC resolution (bits)
#define ADC_COUNTS (1<<ADC_BITS)    // 4096 for 12-bit ADC
#define VREF 3.3                    // ADC reference voltage
#define SAMPLES_PER_CYCLE 100       // Number of samples to take per measurement cycle

// Network settings
#define WIFI_SSID "Your_SSID"       // Replace with your WiFi SSID
#define WIFI_PASSWORD "Your_Password" // Replace with your WiFi password
#define DEFAULT_BACKEND_URL "http://192.168.1.100:8000/api/power-data"

// Timing variables
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // 5 seconds

// Global variables
float currentRMS = 0.0;
float mainVoltage = MAINS_VOLTAGE;
float powerWatts = 0.0;
float energyKwh = 0.0;
unsigned long lastEnergyCalcTime = 0;

// Function declarations
void setupWiFi();
float readCurrentSensor();
float calculateRMSCurrent(float rawADC);
void calculatePower();
void updateEnergy();
bool sendData();
String createJsonPayload();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\n\nESP32 Power Monitoring System Starting...");
  Serial.print("Firmware Version: ");
  Serial.println(FIRMWARE_VERSION);
  
  // Initialize file system
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed! System will use default values");
  }
  
  // Set up LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED on during initialization
  
  // Configure ADC for current sensor
  analogReadResolution(ADC_BITS);
  lastEnergyCalcTime = millis();
  
  // Connect to WiFi
  setupWiFi();
  
  digitalWrite(LED_PIN, LOW); // Turn off LED after initialization
  Serial.println("System initialization complete");
}

void loop() {
  // Read current sensor and calculate power metrics
  unsigned long currentMillis = millis();
  
  // Read and send data every 5 seconds
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;
    
    // Read current sensor
    float rawCurrent = readCurrentSensor();
    currentRMS = calculateRMSCurrent(rawCurrent);
    
    // Calculate power and energy
    calculatePower();
    updateEnergy();
    
    // Log current readings to serial
    Serial.println("------------------------------");
    Serial.print("Current (A): "); Serial.println(currentRMS, 3);
    Serial.print("Voltage (V): "); Serial.println(mainVoltage, 1);
    Serial.print("Power (W): "); Serial.println(powerWatts, 1);
    Serial.print("Energy (kWh): "); Serial.println(energyKwh, 4);
    
    // Send data to backend
    if (WiFi.status() == WL_CONNECTED) {
      bool success = sendData();
      if (success) {
        Serial.println("Data sent successfully");
      } else {
        Serial.println("Failed to send data");
      }
    } else {
      Serial.println("WiFi not connected, data not sent");
      // Attempt to reconnect
      setupWiFi();
    }
  }
  
  // Allow for background tasks and power saving
  delay(100);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi network: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for connection with timeout
  int timeout = 20; // 10 second timeout
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    Serial.print(".");
    timeout--;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
  }
}

float readCurrentSensor() {
  int sampleCount = SAMPLES_PER_CYCLE;
  float sumSquared = 0.0;
  
  // Take multiple samples to improve accuracy
  for (int i = 0; i < sampleCount; i++) {
    // Read analog value from CT sensor
    int adcValue = analogRead(CURRENT_SENSOR_PIN);
    
    // Convert to voltage
    float voltage = (adcValue / float(ADC_COUNTS)) * VREF;
    
    // Find the offset from the midpoint (1.65V for 3.3V reference)
    float voltageOffset = voltage - (VREF / 2.0);
    
    // Square for RMS calculation
    sumSquared += voltageOffset * voltageOffset;
    
    // Small delay between samples
    delayMicroseconds(200);
  }
  
  // Return the mean squared value
  return sumSquared / sampleCount;
}

float calculateRMSCurrent(float meanSquared) {
  // Calculate RMS current from mean squared voltage
  float rmsVoltage = sqrt(meanSquared);
  
  // Convert voltage to current using the CT specifications
  // I = V / R / (Turns Ratio)
  float rmsCurrent = (rmsVoltage / CT_BURDEN_RESISTOR) * CT_TURNS_RATIO;
  
  // Filter out noise floor - ignore very small readings
  if (rmsCurrent < 0.05) {
    rmsCurrent = 0.0;
  }
  
  return rmsCurrent;
}

void calculatePower() {
  // P = I * V for simple calculation (assuming resistive load)
  powerWatts = currentRMS * mainVoltage;
}

void updateEnergy() {
  unsigned long currentTime = millis();
  unsigned long timeDelta = currentTime - lastEnergyCalcTime;
  
  // Only update if some time has passed
  if (timeDelta > 0) {
    // Convert power (W) to energy (kWh): kWh = W * hours / 1000
    // hours = milliseconds / (1000 * 60 * 60)
    float hoursElapsed = timeDelta / 3600000.0;
    float energyIncrement = powerWatts * hoursElapsed / 1000.0;
    
    // Add to cumulative energy
    energyKwh += energyIncrement;
    
    // Update last calculation time
    lastEnergyCalcTime = currentTime;
  }
}

String createJsonPayload() {
  // Create JSON document
  StaticJsonDocument<256> doc;
  
  doc["timestamp"] = millis();
  doc["current_amps"] = currentRMS;
  doc["voltage_volts"] = mainVoltage;
  doc["power_watts"] = powerWatts;
  doc["energy_kwh"] = energyKwh;
  doc["device_id"] = DEVICE_NAME;
  
  // Serialize to string
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}

bool sendData() {
  HTTPClient http;
  bool success = false;
  
  // Create JSON payload
  String jsonPayload = createJsonPayload();
  
  // Connect to server
  http.begin(DEFAULT_BACKEND_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Send POST request
  int httpResponseCode = http.POST(jsonPayload);
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode == HTTP_CODE_OK) {
      success = true;
    }
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
  return success;
}