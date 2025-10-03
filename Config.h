/**
 * Configuration definitions for ESP32 Power Monitoring System
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <vector>

// Device identification
#define DEVICE_NAME "ESP32_Power_Monitor"
#define FIRMWARE_VERSION "1.0.0"

// Hardware pins
#define CURRENT_SENSOR_PIN 34      // ADC pin connected to SCT-013-000
#define LED_PIN 2                  // Built-in LED pin

// Power monitoring constants
#define MAINS_VOLTAGE 230.0        // Default mains voltage (V)
#define CT_BURDEN_RESISTOR 33.0    // Burden resistor value (ohms)
#define CT_TURNS_RATIO 2000.0      // SCT-013-000 turns ratio
#define ADC_BITS 12                // ESP32 ADC resolution (bits)
#define ADC_COUNTS (1<<ADC_BITS)   // 4096 for 12-bit ADC
#define VREF 3.3                   // ADC reference voltage
#define SAMPLES_PER_CYCLE 100      // Number of samples to take per measurement cycle

// Network and server settings
#define DEFAULT_BACKEND_URL "http://192.168.1.100:8000/api/power-data"
#define WIFI_CONFIG_TIMEOUT 180    // Seconds to wait in config portal before continuing
#define CONNECTION_TIMEOUT 10000   // Milliseconds to wait for connection before retry
#define MAX_RETRY_ATTEMPTS 3       // Number of times to retry connection
#define DATA_BUFFER_SIZE 100       // Maximum number of readings to buffer

// NTP settings
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC 3600        // GMT+1 (modify for your timezone)
#define DAYLIGHT_OFFSET_SEC 3600   // 1 hour DST (modify if needed)

// OTA settings
#define OTA_PASSWORD "PowerMonitor" // Password for OTA updates
#define OTA_PORT 3232              // Port for OTA updates

// AI local processing settings
#define ANOMALY_THRESHOLD 0.2      // Threshold for local anomaly detection
#define TREND_WINDOW_SIZE 10       // Window size for trend analysis

// Data structure for power readings
struct PowerData {
  unsigned long timestamp;
  float current;     // Amperes
  float voltage;     // Volts
  float power;       // Watts
  float energy;      // Kilowatt-hours
  bool anomaly;      // Flag for detected anomalies
};

#endif // CONFIG_H
