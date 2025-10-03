/**
 * NetworkManager implementation
 */

#include "NetworkManager.h"

// Static variable to track connection status
static bool s_connected = false;

NetworkManager::NetworkManager() {
  connected = false;
  lastReconnectAttempt = 0;
  ntpConfigured = false;
  otaEnabled = false;
}

void NetworkManager::wifiEventHandler(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Connected to WiFi. IP address: ");
      Serial.println(WiFi.localIP());
      s_connected = true;
      // Turn off LED to indicate normal operation
      digitalWrite(LED_PIN, LOW);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi");
      s_connected = false;
      break;
    default:
      break;
  }
}

void NetworkManager::begin() {
  // Set hostname for easier identification
  WiFi.setHostname(DEVICE_NAME);
  
  // Register WiFi event handlers
  WiFi.onEvent(wifiEventHandler);
  
  // Set up the WiFi Manager
  setupConfigPortal();
  
  // Try to connect to the configured network
  connectToWifi();
  
  // Set up OTA
  setupOTA(DEVICE_NAME);
}

void NetworkManager::update() {
  // Update connection status
  connected = s_connected;
  
  // Check if we are connected
  if (!connected) {
    unsigned long currentMillis = millis();
    
    // Try to reconnect periodically
    if (currentMillis - lastReconnectAttempt > CONNECTION_TIMEOUT) {
      lastReconnectAttempt = currentMillis;
      Serial.println("Attempting to reconnect to Wi-Fi...");
      connectToWifi();
    }
  } else {
    // If connected and NTP not yet configured, set up NTP
    if (!ntpConfigured) {
      configureNTP();
      ntpConfigured = true;
    }
  }
  
  // Handle DNS requests if in portal mode
  wifiManager.process();
  
  // Handle OTA updates if enabled
  if (otaEnabled) {
    ArduinoOTA.handle();
  }
}

bool NetworkManager::isConnected() {
  return connected;
}

unsigned long NetworkManager::getTimestamp() {
  if (ntpConfigured) {
    time_t now;
    time(&now);
    return now;
  } else {
    // Fallback to millis if NTP not configured
    return millis();
  }
}

String NetworkManager::getFormattedTime() {
  if (!ntpConfigured) {
    return "NTP not configured";
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }
  
  char timeString[30];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeString);
}

void NetworkManager::startConfigPortal() {
  // Force start the configuration portal
  Serial.println("Starting configuration portal");
  
  // Turn on built-in LED to indicate configuration mode
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  // Start the portal (blocks until completed or timeout)
  if (!wifiManager.startConfigPortal(DEVICE_NAME)) {
    Serial.println("Failed to connect and hit timeout");
  }
  
  // Turn off LED
  digitalWrite(LED_PIN, LOW);
}

void NetworkManager::resetSettings() {
  // Reset all WiFi settings
  wifiManager.resetSettings();
  Serial.println("WiFi settings reset. Rebooting...");
  delay(1000);
  ESP.restart();
}

void NetworkManager::setupOTA(const String &hostname) {
  // Set up OTA updates
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    
    // Close any open files or stop sensors before OTA
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate complete");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  Serial.println("OTA configured (not enabled yet)");
}

void NetworkManager::enableOTA(bool enable) {
  otaEnabled = enable;
  if (enable) {
    ArduinoOTA.begin();
    Serial.println("OTA updates enabled");
  } else {
    Serial.println("OTA updates disabled");
  }
}

void NetworkManager::setupConfigPortal() {
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT);
  wifiManager.setMinimumSignalQuality(20);  // Set min RSSI (percentage)
  
  // Add custom parameters for backend URL
  WiFiManagerParameter custom_backend_url("backend_url", "Backend URL", DEFAULT_BACKEND_URL, 100);
  wifiManager.addParameter(&custom_backend_url);
  
  // Add parameter for configurable mains voltage
  char voltage_str[10];
  sprintf(voltage_str, "%.1f", MAINS_VOLTAGE);
  WiFiManagerParameter custom_mains_voltage("mains_voltage", "Mains Voltage (V)", voltage_str, 10);
  wifiManager.addParameter(&custom_mains_voltage);
  
  Serial.println("WiFiManager configured with custom parameters");
}

void NetworkManager::configureNTP() {
  // Configure NTP time synchronization
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
  Serial.println("NTP configuration initiated");
  
  // Wait for time to be set
  int retry = 0;
  while (time(nullptr) < 24 * 3600 && retry < 10) {
    Serial.println("Waiting for NTP time sync...");
    delay(1000);
    retry++;
  }
  
  if (retry < 10) {
    Serial.println("NTP time synchronized");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeString[30];
      strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.print("Current time: ");
      Serial.println(timeString);
    }
  } else {
    Serial.println("Failed to synchronize NTP time, will try again later");
    ntpConfigured = false;
  }
}

void NetworkManager::connectToWifi() {
  // Try to connect to the saved WiFi network
  // If cannot connect, start the configuration portal
  Serial.println("Connecting to WiFi...");
  
  // Turn on LED to indicate connection attempt
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  if (!wifiManager.autoConnect(DEVICE_NAME)) {
    Serial.println("Failed to connect and hit timeout");
    // Turn off LED
    digitalWrite(LED_PIN, LOW);
  }
}
