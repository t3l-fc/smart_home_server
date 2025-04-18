/*
 * DisplayManager.h - Alphanumeric Display Manager Module
 * 
 * Manages an Adafruit_AlphaNum4 display to show the status of smart plugs
 * - Shows ON/OFF status for 4 smart plugs (Cactus, Ananas, Dino, Vinyle)
 * - Handles countdown display
 * - Controls display power for low power modes
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

class DisplayManager {
  private:
    Adafruit_AlphaNum4 _display = Adafruit_AlphaNum4();
    bool _isInitialized = false;
    
    // Current display state
    bool _cactusOn = false;
    bool _ananasOn = false;
    bool _dinoOn = false;
    bool _vinyleOn = false;
    
    // Helper to display the status
    void updateDisplay() {
      if (!_isInitialized) return;
      
      // Set characters based on device states
      // C for Cactus, A for Ananas, D for Dino, V for Vinyle
      // Uppercase for ON, lowercase for OFF
      _display.writeDigitAscii(0, _cactusOn ? 'C' : 'c');
      _display.writeDigitAscii(1, _ananasOn ? 'A' : 'a');
      _display.writeDigitAscii(2, _dinoOn ? 'D' : 'd');
      _display.writeDigitAscii(3, _vinyleOn ? 'V' : 'v');
      
      // Update the display
      _display.writeDisplay();
    }
    
  public:
    DisplayManager() {}
    
    // Initialize the display
    bool begin() {
      // Initialize the display hardware
      Wire.beginTransmission(0x70);
      Wire.write(0x21);  // Turn on the oscillator
      Wire.endTransmission();
      
      Wire.beginTransmission(0x70);
      Wire.write(0x81);  // Turn on display, no blinking
      Wire.endTransmission();
      
      Wire.beginTransmission(0x70);
      Wire.write(0xEF);  // Set brightness (0xE0 to 0xEF)
      Wire.endTransmission();
      
      // Show startup pattern to verify display is working
      _display.clear();
      _display.writeDigitAscii(0, 'T');
      _display.writeDigitAscii(1, 'E');
      _display.writeDigitAscii(2, 'S');
      _display.writeDigitAscii(3, 'T');
      _display.writeDisplay();
      delay(500);
      
      // Clear display
      _display.clear();
      _display.writeDisplay();
      
      _isInitialized = true;
      return true;
    }
    
    // Update display with current plug states
    void update(bool cactusOn, bool ananasOn, bool dinoOn, bool vinyleOn) {
      if (!_isInitialized) return;
      
      // Store current states
      _cactusOn = cactusOn;
      _ananasOn = ananasOn;
      _dinoOn = dinoOn;
      _vinyleOn = vinyleOn;
      
      // Update the display
      updateDisplay();
    }
    
    // Show a countdown from startValue to 0
    void showCountdown(int startValue) {
      if (!_isInitialized) return;
      
      char countStr[5];
      
      for (int i = startValue; i >= 0; i--) {
        _display.clear();
        
        // Format countdown number with spaces
        sprintf(countStr, "%d   ", i);
        
        // Display up to 4 characters
        for (int j = 0; j < 4 && countStr[j] != '\0'; j++) {
          _display.writeDigitAscii(j, countStr[j]);
        }
        
        _display.writeDisplay();
        delay(1000);
      }
      
      // After countdown, show the current states
      updateDisplay();
    }
    
    // Turn off the display for power saving
    void turnOff() {
      if (!_isInitialized) return;
      
      _display.clear();
      _display.writeDisplay();
    }
    
    // Wake up the display
    void wakeUp() {
      if (!_isInitialized) return;
      
      // Restore the previous state
      updateDisplay();
    }
    
    // Set display power state (used by SleepManager)
    void setPower(bool on) {
      if (!_isInitialized) return;
      
      if (on) {
        // Turn on display and restore previous state
        wakeUp();
      } else {
        // Turn off display
        turnOff();
      }
    }
};

#endif // DISPLAY_MANAGER_H 