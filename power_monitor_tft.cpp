/**
 * ESP32 Power Monitoring System with TFT Display
 * 
 * Reads SCT-013-000 current sensor, displays data on a TFT screen,
 * and transmits to local backend via WiFi
 * Includes WiFi configuration via captive portal
 * 
 * Created for ESP32 with ILI9341 display
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>

// Initialize TFT display
TFT_eSPI tft = TFT_eSPI();

// Configuration
#define DEVICE_NAME "ESP32_Power_Monitor"
#define FIRMWARE_VERSION "1.0.0"

// Hardware pins
#define CURRENT_SENSOR_PIN 34       // ADC pin connected to SCT-013-000
#define LED_PIN 15                  // External LED pin (changed from built-in to avoid conflict with TFT)
#define BUTTON_PIN 0                // Boot button for config mode

// Power monitoring constants
#define MAINS_VOLTAGE 230.0         // Default mains voltage (V)
#define CT_BURDEN_RESISTOR 33.0     // Burden resistor value (ohms)
#define CT_TURNS_RATIO 2000.0       // SCT-013-000 turns ratio
#define ADC_BITS 12                 // ESP32 ADC resolution (bits)
#define ADC_COUNTS (1<<ADC_BITS)    // 4096 for 12-bit ADC
#define VREF 3.3                    // ADC reference voltage
#define SAMPLES_PER_CYCLE 100       // Number of samples to take per measurement cycle

// Network settings
#define DEFAULT_BACKEND_URL "http://192.168.1.100:8000/api/power-data"
#define WIFI_CONFIG_TIMEOUT 180     // Seconds to wait in config portal before continuing
#define CONNECTION_TIMEOUT 10000    // Milliseconds to wait for connection before retry
#define MAX_RETRY_ATTEMPTS 3        // Number of times to retry connection

// Timing variables
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // 5 seconds
unsigned long lastDisplayUpdateTime = 0;
const unsigned long displayUpdateInterval = 1000; // 1 second
unsigned long lastButtonCheckTime = 0;
const unsigned long buttonDebounceTime = 50; // 50 ms debounce

// Global variables
float currentRMS = 0.0;
float mainVoltage = MAINS_VOLTAGE;
float powerWatts = 0.0;
float energyKwh = 0.0;
unsigned long lastEnergyCalcTime = 0;
String backendUrl = DEFAULT_BACKEND_URL;
bool wifiConnected = false;
int lastButtonState = HIGH;

// Function declarations
void setupWiFi();
void setupDisplay();
void displayData();
void checkButton();
float readCurrentSensor();
float calculateRMSCurrent(float rawADC);
void calculatePower();
void updateEnergy();
bool sendData();
String createJsonPayload();
void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percent, uint16_t frameColor, uint16_t barColor);

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
  
  // Set up button with pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Configure ADC for current sensor
  analogReadResolution(ADC_BITS);
  lastEnergyCalcTime = millis();
  
  // Initialize display
  setupDisplay();
  
  // Display welcome screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("ESP32 Power Monitor");
  tft.setCursor(20, 60);
  tft.setTextSize(1);
  tft.println("Firmware v" + String(FIRMWARE_VERSION));
  tft.setCursor(20, 80);
  tft.println("Initializing...");
  
  // Connect to WiFi using WiFiManager
  WiFiManager wifiManager;
  
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT);
  
  // Add custom parameter for backend URL
  WiFiManagerParameter custom_backend_url("backend_url", "Backend URL", DEFAULT_BACKEND_URL, 100);
  wifiManager.addParameter(&custom_backend_url);
  
  // Add parameter for configurable mains voltage
  char voltage_str[10];
  sprintf(voltage_str, "%.1f", MAINS_VOLTAGE);
  WiFiManagerParameter custom_mains_voltage("mains_voltage", "Mains Voltage (V)", voltage_str, 10);
  wifiManager.addParameter(&custom_mains_voltage);
  
  // Display connecting message
  tft.setCursor(20, 100);
  tft.println("Connecting to WiFi...");
  
  // Try to connect to saved WiFi or start captive portal
  if (!wifiManager.autoConnect(DEVICE_NAME)) {
    Serial.println("Failed to connect and hit timeout");
    tft.setCursor(20, 120);
    tft.println("WiFi connection failed!");
  } else {
    Serial.println("WiFi connected");
    wifiConnected = true;
    
    // Get custom parameters
    String urlParam = custom_backend_url.getValue();
    if (urlParam.length() > 0) {
      backendUrl = urlParam;
    }
    
    String voltageParam = custom_mains_voltage.getValue();
    if (voltageParam.length() > 0) {
      mainVoltage = voltageParam.toFloat();
    }
    
    tft.setCursor(20, 120);
    tft.println("WiFi connected!");
    tft.setCursor(20, 140);
    tft.println("IP: " + WiFi.localIP().toString());
  }
  
  delay(2000); // Show the splash screen for 2 seconds
  
  digitalWrite(LED_PIN, LOW); // Turn off LED after initialization
  Serial.println("System initialization complete");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check if button is pressed to enter config mode
  checkButton();
  
  // Read current sensor and calculate power metrics
  float rawCurrent = readCurrentSensor();
  currentRMS = calculateRMSCurrent(rawCurrent);
  calculatePower();
  updateEnergy();
  
  // Update display
  if (currentMillis - lastDisplayUpdateTime >= displayUpdateInterval) {
    lastDisplayUpdateTime = currentMillis;
    displayData();
  }
  
  // Send data every 5 seconds
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis;
    
    // Log current readings to serial
    Serial.println("------------------------------");
    Serial.print("Current (A): "); Serial.println(currentRMS, 3);
    Serial.print("Voltage (V): "); Serial.println(mainVoltage, 1);
    Serial.print("Power (W): "); Serial.println(powerWatts, 1);
    Serial.print("Energy (kWh): "); Serial.println(energyKwh, 4);
    
    // Send data to backend
    if (wifiConnected && WiFi.status() == WL_CONNECTED) {
      bool success = sendData();
      if (success) {
        Serial.println("Data sent successfully");
        // Briefly flash LED to indicate successful transmission
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
      } else {
        Serial.println("Failed to send data");
      }
    } else {
      Serial.println("WiFi not connected, data not sent");
      wifiConnected = WiFi.status() == WL_CONNECTED;
    }
  }
  
  // Allow for background tasks and power saving
  delay(100);
}

void setupDisplay() {
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void displayData() {
  // Clear the screen area for the data
  tft.fillScreen(TFT_BLACK);
  
  // Title
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(TFT_CYAN);
  tft.println("Power Monitor");
  
  // Draw separating line
  tft.drawLine(0, 40, tft.width(), 40, TFT_DARKGREY);
  
  // Display current readings
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.setTextColor(TFT_WHITE);
  tft.print("Current:");
  tft.setCursor(150, 50);
  tft.setTextColor(TFT_YELLOW);
  tft.print(currentRMS, 2);
  tft.print(" A");
  
  // Display voltage
  tft.setCursor(10, 70);
  tft.setTextColor(TFT_WHITE);
  tft.print("Voltage:");
  tft.setCursor(150, 70);
  tft.setTextColor(TFT_YELLOW);
  tft.print(mainVoltage, 1);
  tft.print(" V");
  
  // Display power
  tft.setCursor(10, 90);
  tft.setTextColor(TFT_WHITE);
  tft.print("Power:");
  tft.setCursor(150, 90);
  tft.setTextColor(TFT_YELLOW);
  tft.print(powerWatts, 1);
  tft.print(" W");
  
  // Display energy
  tft.setCursor(10, 110);
  tft.setTextColor(TFT_WHITE);
  tft.print("Energy:");
  tft.setCursor(150, 110);
  tft.setTextColor(TFT_YELLOW);
  tft.print(energyKwh, 3);
  tft.print(" kWh");
  
  // Draw power bar graph
  int maxPower = 2000; // 2000W max for scale
  int percent = min(100, (int)(powerWatts * 100 / maxPower));
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 130);
  tft.print("Power Usage:");
  drawProgressBar(10, 145, tft.width() - 20, 20, percent, TFT_DARKGREY, TFT_GREEN);
  
  // Display WiFi status
  tft.setCursor(10, tft.height() - 20);
  tft.setTextColor(TFT_WHITE);
  tft.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN);
    tft.print("Connected");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("Disconnected");
  }
}

void checkButton() {
  unsigned long currentMillis = millis();
  
  // Only check periodically to debounce
  if (currentMillis - lastButtonCheckTime < buttonDebounceTime) {
    return;
  }
  
  lastButtonCheckTime = currentMillis;
  
  // Read the button state
  int buttonState = digitalRead(BUTTON_PIN);
  
  // If the button is pressed (LOW for ESP32 GPIO0)
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button was just pressed
    Serial.println("Config button pressed, starting WiFi config portal");
    
    // Display message
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.setTextColor(TFT_CYAN);
    tft.println("WiFi Setup Mode");
    tft.setTextSize(1);
    tft.setCursor(20, 60);
    tft.setTextColor(TFT_WHITE);
    tft.println("Connect to WiFi network:");
    tft.setCursor(20, 80);
    tft.setTextColor(TFT_YELLOW);
    tft.println(DEVICE_NAME);
    tft.setCursor(20, 100);
    tft.setTextColor(TFT_WHITE);
    tft.println("Then go to IP: 192.168.4.1");
    
    // Start WiFi Manager config portal
    WiFiManager wifiManager;
    
    // Add custom parameter for backend URL
    WiFiManagerParameter custom_backend_url("backend_url", "Backend URL", backendUrl.c_str(), 100);
    wifiManager.addParameter(&custom_backend_url);
    
    // Add parameter for configurable mains voltage
    char voltage_str[10];
    sprintf(voltage_str, "%.1f", mainVoltage);
    WiFiManagerParameter custom_mains_voltage("mains_voltage", "Mains Voltage (V)", voltage_str, 10);
    wifiManager.addParameter(&custom_mains_voltage);
    
    // Start the portal (blocks until completed or timeout)
    if (!wifiManager.startConfigPortal(DEVICE_NAME)) {
      Serial.println("Failed to connect and hit timeout");
    } else {
      // Get custom parameters
      String urlParam = custom_backend_url.getValue();
      if (urlParam.length() > 0) {
        backendUrl = urlParam;
      }
      
      String voltageParam = custom_mains_voltage.getValue();
      if (voltageParam.length() > 0) {
        mainVoltage = voltageParam.toFloat();
      }
      
      wifiConnected = WiFi.status() == WL_CONNECTED;
    }
    
    // After portal closes, redraw the screen
    displayData();
  }
  
  lastButtonState = buttonState;
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
  int retryCount = 0;
  
  // Create JSON payload
  String jsonPayload = createJsonPayload();
  
  // Try to send with retries
  while (!success && retryCount < MAX_RETRY_ATTEMPTS) {
    // Connect to server
    http.begin(backendUrl);
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
    
    if (!success) {
      retryCount++;
      if (retryCount < MAX_RETRY_ATTEMPTS) {
        Serial.print("Retrying ("); Serial.print(retryCount); Serial.println("/3)...");
        delay(500 * retryCount); // Exponential backoff
      }
    }
  }
  
  return success;
}

void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t percent, uint16_t frameColor, uint16_t barColor) {
  // Draw frame
  tft.drawRect(x, y, w, h, frameColor);
  
  // Calculate bar width
  uint16_t barWidth = ((w - 4) * percent) / 100;
  
  // Draw bar (leaving 2 pixels margin)
  tft.fillRect(x + 2, y + 2, barWidth, h - 4, barColor);
  
  // Clear the rest of the bar area
  tft.fillRect(x + 2 + barWidth, y + 2, w - 4 - barWidth, h - 4, TFT_BLACK);
  
  // Add percentage text in the middle
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  
  // Calculate position to center the text
  String percentText = String(percent) + "%";
  int16_t textWidth = percentText.length() * 6; // Approximate width for size 1 text
  int16_t textX = x + (w - textWidth) / 2;
  int16_t textY = y + (h - 8) / 2 + 1; // 8 is approximate height for size 1 text
  
  tft.setCursor(textX, textY);
  tft.print(percentText);
}