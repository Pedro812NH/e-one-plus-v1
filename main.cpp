/**
 * ESP32 Power Monitoring System
 * 
 * Main application file for ESP32 NodeMCU with SCT-013-000 current sensor
 * Reads current values, calculates power metrics, and transmits to local backend
 * Includes Wi-Fi configuration via captive portal and connection recovery
 * Features OTA updates and local AI processing
 * 
 * Created for PlatformIO environment
 */

#include <Arduino.h>
#include "Config.h"
#include "PowerMonitor.h"
#include "DataManager.h"
#include "NetworkManager.h"
#include "AiProcessor.h"

// Global instances
PowerMonitor powerMonitor;
DataManager dataManager;
NetworkManager networkManager;
AiProcessor aiProcessor;

// Timing variables
unsigned long lastSendTime = 0;
unsigned long lastAiProcessTime = 0;
const unsigned long sendInterval = 5000; // 5 seconds
const unsigned long aiProcessInterval = 60000; // 1 minute

// Button handling for config portal
const int CONFIG_BUTTON_PIN = 0; // typically BOOT/FLASH button on ESP32
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void checkConfigButton() {
  // Read the button state
  int reading = digitalRead(CONFIG_BUTTON_PIN);
  
  // Check if the reading has changed due to noise or button press
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // If the button state has been stable for the debounce period
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button is pressed (LOW for most ESP32 boards)
    if (reading == LOW) {
      Serial.println("Config button pressed, starting configuration portal");
      networkManager.startConfigPortal();
    }
  }
  
  lastButtonState = reading;
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\n\nESP32 Power Monitoring System Starting...");
  Serial.print("Firmware Version: ");
  Serial.println(FIRMWARE_VERSION);
  
  // Set up config button
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize file system for config storage
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed! System will use default values");
  }
  
  // Initialize network manager (handles Wi-Fi connection and captive portal)
  networkManager.begin();
  
  // Initialize power monitor (handles sensor readings and calculations)
  powerMonitor.begin();
  
  // Initialize data manager (handles data storage and transmission)
  dataManager.begin();
  
  // Initialize AI processor (for local data analysis)
  aiProcessor.begin();
  
  // Enable OTA updates after initialization
  networkManager.enableOTA(true);
  
  Serial.println("System initialization complete");
}

void loop() {
  // Handle network tasks (maintain connection, check for portal requests, OTA)
  networkManager.update();
  
  // Check if config button is pressed
  checkConfigButton();
  
  // Time to send data?
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;
    
    // Read current sensor and calculate power metrics
    powerMonitor.update();
    
    // Prepare data for sending
    PowerData data;
    data.timestamp = networkManager.getTimestamp();
    data.current = powerMonitor.getCurrentAmps();
    data.voltage = powerMonitor.getVoltage();
    data.power = powerMonitor.getPowerWatts();
    data.energy = powerMonitor.getEnergyKwh();
    
    // Process data with AI module (anomaly detection)
    data.anomaly = aiProcessor.detectAnomaly(data);
    aiProcessor.update(data);
    
    // Log current readings to serial
    Serial.println("------------------------------");
    if (networkManager.isConnected()) {
      Serial.print("Time: "); Serial.println(networkManager.getFormattedTime());
    }
    Serial.print("Current (A): "); Serial.println(data.current, 3);
    Serial.print("Voltage (V): "); Serial.println(data.voltage, 1);
    Serial.print("Power (W): "); Serial.println(data.power, 1);
    Serial.print("Energy (kWh): "); Serial.println(data.energy, 4);
    if (data.anomaly) {
      Serial.println("* ANOMALY DETECTED *");
    }
    
    // Try to send data or buffer it if connection is lost
    if (networkManager.isConnected()) {
      // First try to send any buffered data
      if (dataManager.hasBufferedData()) {
        dataManager.sendBufferedData();
      }
      
      // Send current data
      bool success = dataManager.sendData(data);
      if (!success) {
        dataManager.bufferData(data);
        Serial.println("Failed to send data, buffered for later transmission");
      }
    } else {
      // Buffer data for later
      dataManager.bufferData(data);
      Serial.println("No connection, data buffered for later transmission");
    }
  }
  
  // Time to run AI processing tasks?
  if (currentMillis - lastAiProcessTime >= aiProcessInterval) {
    lastAiProcessTime = currentMillis;
    
    // Run AI trend analysis
    Serial.println("Running AI trend analysis...");
    aiProcessor.analyzeTrend();
    
    // Display prediction
    Serial.print("Predicted next power usage: ");
    Serial.print(aiProcessor.getPredictedPower(), 1);
    Serial.println(" W");
  }
  
  // Allow for background tasks and power saving
  delay(100);
}
