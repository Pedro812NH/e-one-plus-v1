/**
 * DataManager implementation
 */

#include "DataManager.h"

DataManager::DataManager() {
  backendUrl = DEFAULT_BACKEND_URL;
}

void DataManager::begin() {
  // Load configuration from file system
  loadConfig();
  
  // Initialize data buffer
  dataBuffer.reserve(DATA_BUFFER_SIZE);
  
  Serial.println("DataManager initialized");
  Serial.print("Backend URL: "); Serial.println(backendUrl);
}

bool DataManager::sendData(const PowerData &data) {
  String jsonPayload = convertDataToJson(data);
  return sendJsonToBackend(jsonPayload);
}

void DataManager::bufferData(const PowerData &data) {
  // Only buffer if we have space
  if (dataBuffer.size() < DATA_BUFFER_SIZE) {
    dataBuffer.push_back(data);
    Serial.print("Data buffered. Buffer size: "); Serial.println(dataBuffer.size());
  } else {
    Serial.println("WARNING: Data buffer full, discarding oldest reading");
    // Remove oldest reading
    dataBuffer.erase(dataBuffer.begin());
    // Add new reading
    dataBuffer.push_back(data);
  }
}

bool DataManager::hasBufferedData() {
  return !dataBuffer.empty();
}

bool DataManager::sendBufferedData() {
  if (dataBuffer.empty()) {
    return true; // No data to send
  }
  
  Serial.print("Attempting to send "); Serial.print(dataBuffer.size()); Serial.println(" buffered readings");
  
  size_t initialSize = dataBuffer.size();
  std::vector<PowerData> failedTransmissions;
  
  // Try to send each buffered item
  for (const auto &data : dataBuffer) {
    if (!sendData(data)) {
      failedTransmissions.push_back(data);
    }
    // Short delay to avoid overwhelming the server
    delay(100);
  }
  
  // Replace buffer with only the failed transmissions
  dataBuffer = failedTransmissions;
  
  Serial.print("Buffered data sent. Remaining buffer size: "); Serial.println(dataBuffer.size());
  
  return dataBuffer.size() < initialSize;
}

void DataManager::setBackendUrl(const String &url) {
  backendUrl = url;
  saveConfig();
}

String DataManager::convertDataToJson(const PowerData &data) {
  // Create JSON document
  StaticJsonDocument<256> doc;
  
  doc["timestamp"] = data.timestamp;
  doc["current_amps"] = data.current;
  doc["voltage_volts"] = data.voltage;
  doc["power_watts"] = data.power;
  doc["energy_kwh"] = data.energy;
  doc["device_id"] = DEVICE_NAME;
  
  // Serialize to string
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}

bool DataManager::sendJsonToBackend(const String &jsonPayload) {
  HTTPClient http;
  bool success = false;
  int retryCount = 0;
  
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
      } else {
        Serial.print("HTTP error: ");
        Serial.println(http.errorToString(httpResponseCode));
      }
    } else {
      Serial.print("Failed to connect, error: ");
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

bool DataManager::loadConfig() {
  // Check if config file exists
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("No config file found, using defaults");
    return false;
  }
  
  // Open config file
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  
  // Parse JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.print("Failed to parse config file: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Load settings
  if (doc.containsKey("backend_url")) {
    backendUrl = doc["backend_url"].as<String>();
  }
  
  Serial.println("Configuration loaded");
  return true;
}

bool DataManager::saveConfig() {
  StaticJsonDocument<512> doc;
  
  // Store current settings
  doc["backend_url"] = backendUrl;
  
  // Open file for writing
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  
  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write config to file");
    configFile.close();
    return false;
  }
  
  configFile.close();
  Serial.println("Configuration saved");
  return true;
}
