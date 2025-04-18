/*
 * SleepManager.h - Deep Sleep Management Module
 * 
 * Handles all deep sleep functionality including:
 * - Sleep configuration
 * - Wake-up sources
 * - Power optimization
 * - Wake-up initialization
 */

#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include "DisplayManager.h"

class SleepManager {
  private:
    uint64_t _lastActivityTime;
    const uint64_t _sleepTimeoutMs;
    bool _deepSleepEnabled;
    DisplayManager* _display;
    
    // Define the pins used in our project so we can set unused pins properly
    const int* _usedPins;
    const int _numUsedPins;
    
  public:
    // Constructor
    SleepManager(const int* usedPins, int numUsedPins, DisplayManager* display, 
                uint64_t sleepTimeoutMs = 60000) :
      _usedPins(usedPins),
      _numUsedPins(numUsedPins),
      _display(display),
      _sleepTimeoutMs(sleepTimeoutMs) {
      
      _lastActivityTime = 0;
      _deepSleepEnabled = true;
    }
    
    // Record activity to reset sleep timer
    void recordActivity() {
      _lastActivityTime = millis();
      digitalWrite(LED_BUILTIN, HIGH);  // Visual indicator of activity
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
    }
    
    // Configure deep sleep wake sources
    void configureWakeupSources(const uint64_t wakeupPins) {
      // Configure EXT1 wake-up source (multiple pins)
      esp_sleep_enable_ext1_wakeup(wakeupPins, ESP_EXT1_WAKEUP_ANY_HIGH);
      
      Serial.println("Sleep mode configured. Will wake on any switch change.");
    }
    
    // Configure unused pins to minimize power leakage
    void configureUnusedPins() {
      // Get total GPIO count for ESP32
      const int NUM_GPIO = 40;
      
      // Loop through all GPIO pins
      for (int i = 0; i < NUM_GPIO; i++) {
        // Skip used pins
        bool isPinUsed = false;
        for (int j = 0; j < _numUsedPins; j++) {
          if (i == _usedPins[j]) {
            isPinUsed = true;
            break;
          }
        }
        
        // Skip special pins that shouldn't be modified
        if (i == 0 || i == 1 || // UART pins for serial
            i == 6 || i == 7 || i == 8 || i == 9 || i == 10 || i == 11 || // SPI flash pins
            i == 16 || i == 17 || // not accessible on HUZZAH32
            i >= 34) { // Input-only pins
          continue;
        }
        
        // Configure unused pins as INPUT (no pullup/pulldown) to minimize power
        if (!isPinUsed) {
          pinMode(i, INPUT);
        }
      }
      
      Serial.println("Unused pins configured for power saving");
    }
    
    // Prepare for deep sleep with full power optimization
    void enterDeepSleep() {
      Serial.println("Preparing for deep sleep with power optimization...");
      
      // 1. Turn off display
      _display->setPower(false);
      
      // 2. Put I2C pins in high-impedance state (INPUT_PULLUP)
      Wire.end();
      
      // 3. Disable WiFi
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      esp_wifi_stop();
      
      // 4. Disable Bluetooth
      btStop();
      esp_bt_controller_disable();
      
      // 5. Disable ADC
      adc_power_off();
      
      // 6. Configure unused pins
      configureUnusedPins();
      
      // 7. Turn off LED
      digitalWrite(LED_BUILTIN, LOW);
      
      // 8. Configure power domains for maximum savings
      // Disable unnecessary power domains (RTC peripherals will stay on for wake-up)
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
      esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
      
      // 9. Flash power saving options
      esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
      
      // Short delay to allow serial to finish
      delay(100);
      
      Serial.println("Entering deep sleep mode...");
      esp_deep_sleep_start();
    }
    
    // Initialize device after wake-up
    void initAfterWakeUp() {
      // Check wake-up reason
      esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
      
      if (wakeupReason == ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Woke up from deep sleep due to switch activity!");
        
        // Get which GPIO triggered the wake-up
        uint64_t wakeupPin = esp_sleep_get_ext1_wakeup_status();
        
        // Convert bit pattern to pin number
        for (int i = 0; i < 40; i++) {
          if (wakeupPin & (1ULL << i)) {
            Serial.printf("Wake-up triggered by GPIO %d\n", i);
            break;
          }
        }
      } else {
        Serial.println("Normal boot (not from deep sleep)");
      }
      
      // Re-enable ADC that was disabled during sleep
      adc_power_on();
    }
    
    // Check if it's time to sleep
    bool shouldSleep() {
      return _deepSleepEnabled && millis() - _lastActivityTime > _sleepTimeoutMs;
    }
    
    // Enable/disable deep sleep
    void setDeepSleepEnabled(bool enabled) {
      _deepSleepEnabled = enabled;
    }
    
    // Get deep sleep enabled state
    bool isDeepSleepEnabled() const {
      return _deepSleepEnabled;
    }
};

#endif // SLEEP_MANAGER_H 