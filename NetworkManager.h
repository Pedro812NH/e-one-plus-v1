/**
 * NetworkManager Class
 * Handles Wi-Fi connection, captive portal, and network status
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "Config.h"
#include <WiFiManager.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <time.h>
#include <ArduinoOTA.h>

class NetworkManager {
public:
  NetworkManager();
  
  void begin();                    // Initialize network connection
  void update();                   // Update network state, handle reconnection
  bool isConnected();              // Check if connected to network
  unsigned long getTimestamp();    // Get current timestamp (seconds since epoch)
  String getFormattedTime();       // Get formatted time string
  
  void startConfigPortal();        // Force start the configuration portal
  void resetSettings();            // Reset all saved settings
  void setupOTA(const String &hostname); // Set up Over-The-Air updates
  void enableOTA(bool enable);     // Enable or disable OTA updates
  
private:
  WiFiManager wifiManager;         // WiFiManager instance for captive portal
  bool connected;                  // Current connection status
  unsigned long lastReconnectAttempt; // Last time we tried to reconnect
  bool ntpConfigured;              // Flag to track if NTP is configured
  bool otaEnabled;                 // Flag to track if OTA is enabled
  
  void setupConfigPortal();        // Set up the configuration portal
  void connectToWifi();            // Try to connect to Wi-Fi
  void configureNTP();             // Configure NTP time server
  
  // WiFi event handlers for ESP32
  static void wifiEventHandler(WiFiEvent_t event);
};

#endif // NETWORK_MANAGER_H
