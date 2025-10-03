/**
 * DataManager Class
 * Handles data storage, buffering, and transmission to backend
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "Config.h"
#include <ArduinoJson.h>

class DataManager {
public:
  DataManager();
  
  void begin();                          // Initialize the data manager
  bool sendData(const PowerData &data);  // Send power data to backend
  void bufferData(const PowerData &data); // Store data for later transmission
  bool hasBufferedData();                // Check if there is buffered data
  bool sendBufferedData();               // Send buffered data to backend
  
  void setBackendUrl(const String &url); // Set backend URL

private:
  String backendUrl;                 // URL for the backend server
  std::vector<PowerData> dataBuffer; // Buffer for unsent data

  bool sendJsonToBackend(const String &jsonPayload); // Send JSON to backend
  String convertDataToJson(const PowerData &data);   // Convert data to JSON string
  bool loadConfig();                                // Load configuration from storage
  bool saveConfig();                                // Save configuration to storage
};

#endif // DATA_MANAGER_H
