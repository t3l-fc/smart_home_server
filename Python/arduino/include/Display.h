#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

class Display {
private:
  Adafruit_AlphaNum4 _alpha4 = Adafruit_AlphaNum4();

public:
  Display() {}

  void display(String msg) {
    _alpha4.clear();
    // Write each character to the display
    for (uint8_t i = 0; i < 4 && i < msg.length(); i++) {
      _alpha4.writeDigitAscii(i, msg.charAt(i));
    }
    _alpha4.writeDisplay();
  }
  
  // Initialize the display
  void setup() {
    _alpha4.begin(0x70);
    delay(100);
    clear();
  }
  
  // Clear the display
  void clear() {
    _alpha4.clear();
  }
  
};

#endif // DISPLAY_H 