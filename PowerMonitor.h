/**
 * PowerMonitor Class
 * Handles current sensor readings and power calculations
 */

#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include "Config.h"

class PowerMonitor {
public:
  PowerMonitor();
  
  void begin();                // Initialize the sensor and calibration
  void update();               // Read sensor and update values
  
  float getCurrentAmps();      // Get the latest current measurement (A)
  float getVoltage();          // Get the configured mains voltage (V)
  float getPowerWatts();       // Get the calculated power (W)
  float getEnergyKwh();        // Get the cumulative energy consumption (kWh)
  
  void calibrate();            // Run calibration routine
  void setVoltage(float v);    // Set the mains voltage

private:
  float currentRMS;            // Calculated RMS current value
  float mainVoltage;           // Configured mains voltage
  float powerWatts;            // Calculated power
  float energyKwh;             // Cumulative energy consumption
  
  unsigned long lastEnergyCalcTime;  // Timestamp for energy calculation
  
  float readCurrentSensor();   // Read raw values from current sensor
  float calculateRMSCurrent(float rawADC); // Calculate RMS current from raw ADC
  void calculatePower();       // Calculate power from current and voltage
  void updateEnergy();         // Update cumulative energy consumption
  
  // Constants for calibration
  float calibrationFactor;     // Calibration factor for the specific setup
};

#endif // POWER_MONITOR_H
