#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <Arduino.h>
#include "DisplayManager.h"

// Display state definitions
enum DisplayState {
  STATE_BOOT,
  STATE_WIFI_CONNECTING,
  STATE_WIFI_OK,
  STATE_WIFI_FAIL,
  STATE_MQTT_CONNECTING,
  STATE_MQTT_OK,
  STATE_MQTT_FAIL,
  STATE_READY,
  STATE_CUSTOM,
  STATE_POWER_SAVE
};

class DisplayService {
private:
  TaskHandle_t _displayTaskHandle = NULL;
  DisplayManager* _displayManager;
  
  // Variables for state management (protected by mutex)
  SemaphoreHandle_t _mutex;
  volatile DisplayState _currentState = STATE_BOOT;
  char _customMessage[5] = ""; // Change to fixed-size array
  
  // Power saving variables
  volatile unsigned long _lastActivityTime = 0;
  const unsigned long _displayTimeout = 30000; // 30 seconds
  volatile bool _displayActive = true;
  
  // Static task function that calls the member function
  static void displayTaskCode(void *parameter) {
    DisplayService* service = (DisplayService*)parameter;
    service->displayTask();
  }
  
  // The actual task that runs on core 0
  void displayTask() {
    char displayText[5] = ""; // Fixed-size buffer
    int animationFrame = 0;
    const char* animChars = "|/-\\";
    unsigned long lastUpdate = 0;
    
    while(true) {
      // Get current timestamp
      unsigned long now = millis();
      
      // Check if we need to enter power save mode
      if (_displayActive && (now - _lastActivityTime > _displayTimeout)) {
        if (xSemaphoreTake(_mutex, (TickType_t)10) == pdTRUE) {
          // Only change to power save if we're in a stable state
          if (_currentState == STATE_READY || _currentState == STATE_CUSTOM) {
            _currentState = STATE_POWER_SAVE;
            _displayActive = false;
            // Clear the display to save power
            _displayManager->clear();
            _displayManager->writeDisplay(false); // Turn off display
          }
          xSemaphoreGive(_mutex);
        }
      }
      
      // Update animation at regular intervals
      if (now - lastUpdate >= 300) {  // Slower update rate
        lastUpdate = now;
        animationFrame = (animationFrame + 1) % 4;
        
        // Take mutex to safely read state
        if (xSemaphoreTake(_mutex, (TickType_t)10) == pdTRUE) {
          DisplayState state = _currentState;
          
          // Skip display updates in power save mode
          if (state != STATE_POWER_SAVE) {
            // Generate display text based on state
            switch (state) {
              case STATE_BOOT:
                strcpy(displayText, "BOOT");
                break;
                
              case STATE_WIFI_CONNECTING:
                strcpy(displayText, "WIF");
                displayText[3] = animChars[animationFrame];
                displayText[4] = '\0';
                break;
                
              case STATE_WIFI_OK:
                strcpy(displayText, "WION");
                break;
                
              case STATE_WIFI_FAIL:
                strcpy(displayText, "WIKO");
                break;
                
              case STATE_MQTT_CONNECTING:
                strcpy(displayText, "MQT");
                displayText[3] = animChars[animationFrame];
                displayText[4] = '\0';
                break;
                
              case STATE_MQTT_OK:
                strcpy(displayText, "MQON");
                break;
                
              case STATE_MQTT_FAIL:
                strcpy(displayText, "MQKO");
                break;
                
              case STATE_READY:
                // Don't update in ready state unless custom message
                break;
                
              case STATE_CUSTOM:
                strcpy(displayText, _customMessage);
                break;
            }
            
            // Update the display if text has content
            if (strlen(displayText) > 0) {
              _displayManager->display(displayText);
            }
          }
          
          xSemaphoreGive(_mutex);
        }
      }
      
      // Give other tasks a chance to run
      vTaskDelay(50); // Longer delay to reduce CPU usage
    }
  }

public:
  DisplayService(DisplayManager* displayManager) {
    _displayManager = displayManager;
    _mutex = xSemaphoreCreateMutex();
  }
  
  bool begin() {
    // Create task on core 0 with higher priority and larger stack
    return xTaskCreatePinnedToCore(
      displayTaskCode,           // Function to implement the task
      "DisplayTask",             // Name of the task
      4096,                      // Stack size in words (increased)
      this,                      // Task input parameter
      2,                         // Priority of the task (increased)
      &_displayTaskHandle,       // Task handle
      0                          // Core where the task should run
    ) == pdPASS;
  }
  
  void setState(DisplayState state) {
    if (xSemaphoreTake(_mutex, (TickType_t)10) == pdTRUE) {
      _currentState = state;
      // Any state change counts as activity
      _lastActivityTime = millis();
      
      // Wake up display if it was off
      if (!_displayActive) {
        _displayActive = true;
        _displayManager->writeDisplay(true); // Turn on display
      }
      
      xSemaphoreGive(_mutex);
    }
  }
  
  void showCustomMessage(const char* message) {
    if (xSemaphoreTake(_mutex, (TickType_t)10) == pdTRUE) {
      _currentState = STATE_CUSTOM;
      strncpy(_customMessage, message, 4);
      _customMessage[4] = '\0'; // Ensure null termination
      
      // Update last activity time
      _lastActivityTime = millis();
      
      // Wake up display if it was off
      if (!_displayActive) {
        _displayActive = true;
        _displayManager->writeDisplay(true); // Turn on display
      }
      
      xSemaphoreGive(_mutex);
    }
  }
  
  void showStatus(const char* status) {
    if (xSemaphoreTake(_mutex, (TickType_t)10) == pdTRUE) {
      _currentState = STATE_CUSTOM;
      strncpy(_customMessage, status, 4);
      _customMessage[4] = '\0'; // Ensure null termination
      
      // Update last activity time
      _lastActivityTime = millis();
      
      // Wake up display if it was off
      if (!_displayActive) {
        _displayActive = true;
        _displayManager->writeDisplay(true); // Turn on display
      }
      
      xSemaphoreGive(_mutex);
    }
  }
  
  // Register activity to prevent display timeout
  void registerActivity() {
    if (xSemaphoreTake(_mutex, (TickType_t)10) == pdTRUE) {
      _lastActivityTime = millis();
      
      // Wake up display if it was off
      if (!_displayActive) {
        _displayActive = true;
        _currentState = STATE_CUSTOM; // Return to previous state
        _displayManager->writeDisplay(true); // Turn on display
      }
      
      xSemaphoreGive(_mutex);
    }
  }
  
  // Overloaded methods for String compatibility
  void showCustomMessage(String message) {
    showCustomMessage(message.c_str());
  }
  
  void showStatus(String status) {
    showStatus(status.c_str());
  }
};

#endif // DISPLAY_SERVICE_H 