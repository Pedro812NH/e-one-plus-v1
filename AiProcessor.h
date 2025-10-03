/**
 * AiProcessor Class
 * Handles local AI processing for power monitoring data
 */

#ifndef AI_PROCESSOR_H
#define AI_PROCESSOR_H

#include "Config.h"
#include <deque>

class AiProcessor {
public:
  AiProcessor();
  
  void begin();                                 // Initialize the AI processor
  void update(const PowerData &data);           // Process new power data
  bool detectAnomaly(const PowerData &data);    // Detect anomalies in current readings
  void analyzeTrend();                          // Analyze power usage trends
  float getPredictedPower();                    // Get predicted power for next interval
  
private:
  std::deque<PowerData> dataHistory;            // Store recent data for analysis
  float powerPrediction;                        // Predicted power for next interval
  
  // Simple moving average calculation
  float calculateMovingAverage(float value);
  
  // Standard deviation calculation for anomaly detection
  float calculateStandardDeviation();
  
  // Linear regression for basic trend prediction
  void updatePrediction();
};

#endif // AI_PROCESSOR_H