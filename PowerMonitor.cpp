/**
 * PowerMonitor implementation
 */

#include "PowerMonitor.h"

PowerMonitor::PowerMonitor() {
  currentRMS = 0.0;
  mainVoltage = MAINS_VOLTAGE;
  powerWatts = 0.0;
  energyKwh = 0.0;
  lastEnergyCalcTime = 0;
  calibrationFactor = 1.0; // Default value, should be calibrated
}

void PowerMonitor::begin() {
  // Configure ADC
  analogReadResolution(ADC_BITS);
  
  // Set initial time
  lastEnergyCalcTime = millis();
  
  Serial.println("PowerMonitor initialized");
  Serial.print("Using mains voltage: "); Serial.print(mainVoltage); Serial.println(" V");
}

void PowerMonitor::update() {
  // Read current sensor and calculate RMS value
  float rawCurrent = readCurrentSensor();
  currentRMS = calculateRMSCurrent(rawCurrent);
  
  // Calculate power based on current and voltage
  calculatePower();
  
  // Update cumulative energy consumption
  updateEnergy();
}

float PowerMonitor::getCurrentAmps() {
  return currentRMS;
}

float PowerMonitor::getVoltage() {
  return mainVoltage;
}

float PowerMonitor::getPowerWatts() {
  return powerWatts;
}

float PowerMonitor::getEnergyKwh() {
  return energyKwh;
}

void PowerMonitor::setVoltage(float v) {
  if (v > 0) {
    mainVoltage = v;
  }
}

float PowerMonitor::readCurrentSensor() {
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

float PowerMonitor::calculateRMSCurrent(float meanSquared) {
  // Calculate RMS current from mean squared voltage
  float rmsVoltage = sqrt(meanSquared);
  
  // Convert voltage to current using the CT specifications
  // I = V / R / (Turns Ratio)
  float rmsCurrent = (rmsVoltage / CT_BURDEN_RESISTOR) * CT_TURNS_RATIO;
  
  // Apply calibration factor
  rmsCurrent *= calibrationFactor;
  
  // Filter out noise floor - ignore very small readings
  if (rmsCurrent < 0.05) {
    rmsCurrent = 0.0;
  }
  
  return rmsCurrent;
}

void PowerMonitor::calculatePower() {
  // P = I * V for simple calculation (assuming resistive load)
  powerWatts = currentRMS * mainVoltage;
}

void PowerMonitor::updateEnergy() {
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

void PowerMonitor::calibrate() {
  // In a real-world implementation, this would allow for calibration
  // against a known reference current source.
  Serial.println("Calibration function called, but not implemented yet");
}
