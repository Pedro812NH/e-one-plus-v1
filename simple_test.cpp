/**
 * Simple test program for ESP32 to verify compilation
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Power Monitoring System - Simple Test");
}

void loop() {
  Serial.println("Hello, ESP32!");
  delay(1000);
}