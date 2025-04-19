#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

class DisplayManager {
private:
  Adafruit_AlphaNum4 _alpha4 = Adafruit_AlphaNum4();
  bool _displayOn = true;

public:
  void display(String msg) {
    if (!_displayOn) return; // Skip if display is off
    
    _alpha4.clear();
    // Write each character to the display
    for (uint8_t i = 0; i < 4 && i < msg.length(); i++) {
      _alpha4.writeDigitAscii(i, msg.charAt(i));
    }
    _alpha4.writeDisplay();
  }
  
  // Display with char array
  void display(const char* msg) {
    if (!_displayOn) return; // Skip if display is off
    
    _alpha4.clear();
    // Write each character to the display
    for (uint8_t i = 0; i < 4 && i < strlen(msg); i++) {
      _alpha4.writeDigitAscii(i, msg[i]);
    }
    _alpha4.writeDisplay();
  }
  
  // Initialize the display
  bool setup() {
    _alpha4.begin(0x70);
    delay(100);
    clear();
    _displayOn = true;
    return true;
  }
  
  // Clear the display
  void clear() {
    _alpha4.clear();
    _alpha4.writeDisplay();
  }
  
  // Control display power state
  void writeDisplay(bool isOn) {
    _displayOn = isOn;
    
    if (isOn) {
      // Re-init the display
      _alpha4.begin(0x70);
    } else {
      // Clear display when turning off
      _alpha4.clear();
      _alpha4.writeDisplay();
      
      // Turn off the display completely (no LEDs lit)
      Wire.beginTransmission(0x70);
      Wire.write(0x80); // Control register
      Wire.write(0x00); // Set the display off
      Wire.endTransmission();
    }
  }
  
  // Check if display is on
  bool isDisplayOn() {
    return _displayOn;
  }
  
};

#endif // DISPLAY_H 