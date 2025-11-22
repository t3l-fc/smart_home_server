#ifndef POTENTIOMETER_MANAGER_H
#define POTENTIOMETER_MANAGER_H

#include <Arduino.h>

class PotentiometerManager {
  private:
    const int _potPin;
    const unsigned long _restDelay = 200; // Time to wait after rotation stops (in ms)
    unsigned long _lastChangeTime = 0; // Timestamp when value last changed
    unsigned long _lastSentTime = 0; // Timestamp when value was last sent
    
    int _lastReadValue = -1; // Last read value (not necessarily sent)
    int _lastSentValue = -1; // Last value that was actually sent (-1 = not initialized)
    bool _hasPendingChange = false; // True if there's a change waiting to be sent
    const int _minChangeThreshold = 2; // Minimum change in percentage to trigger update (to avoid noise)
    
    // Convert analog reading (0-4095) to percentage (0-100)
    int analogToPercent(int analogValue) {
      // ESP32 ADC returns 0-4095 (12 bits)
      int percent = map(analogValue, 0, 4095, 0, 100);
      // Clamp to 0-100
      if (percent < 0) percent = 0;
      if (percent > 100) percent = 100;
      return percent;
    }
    
  public:
    // Constructor
    PotentiometerManager(int potPin) : _potPin(potPin) {
    }
    
    // Initialize the potentiometer pin
    bool setup() {
      // Pin 13 is analog-capable on ESP32
      // No special setup needed for analogRead on ESP32
      // But we can explicitly set it as INPUT
      pinMode(_potPin, INPUT);
      
      // Read initial value
      int initialPercent = analogToPercent(analogRead(_potPin));
      _lastReadValue = initialPercent;
      _lastSentValue = initialPercent; // Consider initial value as already "sent"
      _hasPendingChange = false;
      _lastChangeTime = millis();
      _lastSentTime = millis();
      
      Serial.printf("🎚️ Potentiometer initialized on pin %d (initial value: %d%%, rest delay: %lums)\n", 
                    _potPin, initialPercent, _restDelay);
      
      return true;
    }
    
    // Update potentiometer reading and check if ready to send
    // Returns true only when value has been stable for _restDelay ms after a change
    bool update() {
      unsigned long currentTime = millis();
      
      // Read analog value
      int analogValue = analogRead(_potPin);
      int currentPercent = analogToPercent(analogValue);
      
      // Check if current value differs significantly from last SENT value
      if(_lastSentValue == -1 || abs(currentPercent - _lastSentValue) >= _minChangeThreshold) {
        // Value changed compared to what we sent - update read value and reset timer
        _lastReadValue = currentPercent;
        _lastChangeTime = currentTime;
        _hasPendingChange = true;
        // Continue to check if rest period elapsed (don't return yet)
      } else {
        // Value is same or very close to last sent - update read value but don't reset timer
        _lastReadValue = currentPercent;
      }
      
      // Check if we have a pending change and if rest period elapsed
      if(_hasPendingChange) {
        unsigned long timeSinceChange = currentTime - _lastChangeTime;
        if(timeSinceChange >= _restDelay) {
          // Potentiometer has been at rest for long enough
          // Check if current value still differs from what we last sent
          if(abs(_lastReadValue - _lastSentValue) >= _minChangeThreshold) {
            _lastSentValue = _lastReadValue;
            _hasPendingChange = false;
            _lastSentTime = currentTime;
            return true; // Ready to send
          } else {
            // Value changed back to what we sent - cancel pending change
            _hasPendingChange = false;
          }
        }
      }
      
      return false;
    }
    
    // Get current percentage value (0-100) - returns last sent value
    int getPercent() const {
      return _lastSentValue >= 0 ? _lastSentValue : _lastReadValue;
    }
    
    // Get raw analog value (0-4095)
    int getRawValue() const {
      return analogRead(_potPin);
    }
};

#endif // POTENTIOMETER_MANAGER_H

