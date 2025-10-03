/**
 * AiProcessor implementation
 */

#include "AiProcessor.h"
#include <math.h>

AiProcessor::AiProcessor() {
  powerPrediction = 0.0;
}

void AiProcessor::begin() {
  // Initialize the data history with empty space
  dataHistory.clear();
  
  Serial.println("AiProcessor initialized");
}

void AiProcessor::update(const PowerData &data) {
  // Add new data to history
  dataHistory.push_back(data);
  
  // Keep history size limited to TREND_WINDOW_SIZE
  if (dataHistory.size() > TREND_WINDOW_SIZE) {
    dataHistory.pop_front();
  }
  
  // Update prediction for next interval
  updatePrediction();
}

bool AiProcessor::detectAnomaly(const PowerData &data) {
  // Simple anomaly detection based on standard deviation
  if (dataHistory.size() < 3) {
    return false; // Not enough data for detection
  }
  
  float avg = calculateMovingAverage(data.power);
  float stdDev = calculateStandardDeviation();
  
  // Check if current value deviates significantly from moving average
  float deviation = fabs(data.power - avg);
  
  if (deviation > (stdDev * ANOMALY_THRESHOLD)) {
    Serial.print("Anomaly detected! Current: ");
    Serial.print(data.power);
    Serial.print(" W, Avg: ");
    Serial.print(avg);
    Serial.print(" W, Deviation: ");
    Serial.print(deviation);
    Serial.print(" (threshold: ");
    Serial.print(stdDev * ANOMALY_THRESHOLD);
    Serial.println(")");
    return true;
  }
  
  return false;
}

void AiProcessor::analyzeTrend() {
  if (dataHistory.size() < TREND_WINDOW_SIZE) {
    Serial.println("Not enough data for trend analysis");
    return;
  }
  
  // Calculate simple trend (increasing, decreasing, stable)
  float firstAvg = 0.0;
  float lastAvg = 0.0;
  
  // First half average
  for (size_t i = 0; i < TREND_WINDOW_SIZE / 2; i++) {
    firstAvg += dataHistory[i].power;
  }
  firstAvg /= (TREND_WINDOW_SIZE / 2);
  
  // Second half average
  for (size_t i = TREND_WINDOW_SIZE / 2; i < TREND_WINDOW_SIZE; i++) {
    lastAvg += dataHistory[i].power;
  }
  lastAvg /= (TREND_WINDOW_SIZE / 2);
  
  float change = lastAvg - firstAvg;
  float percentChange = (change / firstAvg) * 100.0;
  
  Serial.print("Power trend analysis: ");
  if (fabs(percentChange) < 5.0) {
    Serial.println("Stable usage");
  } else if (percentChange > 0) {
    Serial.print("Increasing (");
    Serial.print(percentChange);
    Serial.println("%)");
  } else {
    Serial.print("Decreasing (");
    Serial.print(-percentChange);
    Serial.println("%)");
  }
}

float AiProcessor::getPredictedPower() {
  return powerPrediction;
}

float AiProcessor::calculateMovingAverage(float value) {
  if (dataHistory.empty()) {
    return value;
  }
  
  float sum = 0;
  for (const auto &data : dataHistory) {
    sum += data.power;
  }
  
  return sum / dataHistory.size();
}

float AiProcessor::calculateStandardDeviation() {
  if (dataHistory.size() < 2) {
    return 0.0;
  }
  
  // Calculate mean
  float mean = 0.0;
  for (const auto &data : dataHistory) {
    mean += data.power;
  }
  mean /= dataHistory.size();
  
  // Calculate sum of squared differences
  float sumSqDiff = 0.0;
  for (const auto &data : dataHistory) {
    float diff = data.power - mean;
    sumSqDiff += diff * diff;
  }
  
  // Standard deviation
  return sqrt(sumSqDiff / (dataHistory.size() - 1));
}

void AiProcessor::updatePrediction() {
  if (dataHistory.size() < 2) {
    if (!dataHistory.empty()) {
      powerPrediction = dataHistory.back().power;
    }
    return;
  }
  
  // Simple linear regression for prediction
  float sumX = 0;
  float sumY = 0;
  float sumXY = 0;
  float sumX2 = 0;
  int n = dataHistory.size();
  
  for (int i = 0; i < n; i++) {
    float x = i;
    float y = dataHistory[i].power;
    
    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }
  
  // Calculate regression line slope and intercept
  float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
  float intercept = (sumY - slope * sumX) / n;
  
  // Predict next value
  powerPrediction = intercept + slope * n;
  
  // Ensure prediction is not negative
  if (powerPrediction < 0) {
    powerPrediction = 0;
  }
}