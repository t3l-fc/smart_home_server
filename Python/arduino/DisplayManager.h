/*
 * DisplayManager.h - 7-Segment Display Module
 * 
 * Handles the Adafruit 0.56" 4-Digit 7-Segment Display w/I2C Backpack
 * Each segment shows the status of the smart plug whose switch is directly below it
 * From left to right: Cactus, Ananas, Dino, Vinyle
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

class DisplayManager {
  private:
    Adafruit_7segment _display;
    bool _isConnected;
    
    // Display states
    unsigned long _lastUpdateTime;
    const unsigned long _updateInterval = 500;  // Update every 500ms
    
  public:
    // Constructor
    DisplayManager() {
      _isConnected = false;
      _lastUpdateTime = 0;
    }
    
    // Initialize the display
    bool begin() {
      _display.begin(0x70);  // Default I2C address is 0x70
      
      // Test if display is responding
      Wire.beginTransmission(0x70);
      byte error = Wire.endTransmission();
      
      if (error == 0) {
        _isConnected = true;
        // Show startup message
        _display.print(8888);
        _display.writeDisplay();
        delay(500);
        _display.clear();
        _display.writeDisplay();
        
        Serial.println("7-Segment Display initialized");
        return true;
      } else {
        Serial.println("7-Segment Display not found");
        return false;
      }
    }
    
    // Update the display to show plug status
    void update(bool cactusOn, bool ananasOn, bool dinoOn, bool vinyleOn) {
      if (!_isConnected) return;
      
      // Update display periodically
      if (millis() - _lastUpdateTime > _updateInterval) {
        _lastUpdateTime = millis();
        
        // Display status: each segment corresponds to the switch below it
        // and shows 1 if ON, 0 if OFF
        
        // We need to use the raw digit writing since we're setting each digit separately
        _display.clear();
        
        // Digit 0 (leftmost) for Cactus
        _display.writeDigitNum(0, cactusOn ? 1 : 0, false);
        
        // Digit 1 for Ananas
        _display.writeDigitNum(1, ananasOn ? 1 : 0, false);
        
        // Digit 3 for Dino (note: digit 2 is the colon in 7-segment displays)
        _display.writeDigitNum(3, dinoOn ? 1 : 0, false);
        
        // Digit 4 (rightmost) for Vinyle
        _display.writeDigitNum(4, vinyleOn ? 1 : 0, false);
        
        _display.writeDisplay();
      }
    }
    
    // Display countdown
    void showCountdown(int seconds) {
      if (!_isConnected) return;
      
      for (int i = seconds; i >= 0; i--) {
        // Clear display
        _display.clear();
        
        // Show countdown number
        if (i > 0) {
          _display.print(i);
        } else {
          // For zero, show "Go"
          _display.writeDigitAscii(0, 'G');
          _display.writeDigitAscii(3, 'o');
        }
        
        _display.writeDisplay();
        delay(1000);
      }
      
      // Clear the display after countdown
      _display.clear();
      _display.writeDisplay();
    }
    
    // Clear the display
    void clear() {
      if (!_isConnected) return;
      _display.clear();
      _display.writeDisplay();
    }
    
    // Power on/off display
    void setPower(bool on) {
      if (!_isConnected) return;
      if (on) {
        _display.setBrightness(7); // Max brightness
      } else {
        _display.setBrightness(0); // Min brightness
        _display.clear();
        _display.writeDisplay();
      }
    }
};

#endif // DISPLAY_MANAGER_H 