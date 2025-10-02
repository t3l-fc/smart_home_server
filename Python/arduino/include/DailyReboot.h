/*
 * DailyReboot.h - Daily Automatic Reboot System
 * 
 * Automatically reboots the ESP32 at a specified time each day
 * to prevent memory leaks and maintain system stability.
 * 
 * Features:
 * - Configurable reboot time (hour and minute)
 * - NTP time synchronization
 * - Countdown warning before reboot
 * - Safe reboot with proper cleanup
 */

#ifndef DAILY_REBOOT_H
#define DAILY_REBOOT_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class DailyReboot {
  private:
    int _rebootHour = 3;    // Default: 3 AM
    int _rebootMinute = 0;  // Default: 3:00 AM
    bool _enabled = true;
    
    unsigned long _lastTimeCheck = 0;
    unsigned long _timeCheckInterval = 60000; // Check time every minute
    
    bool _ntpConfigured = false;
    bool _countdownStarted = false;
    unsigned long _countdownStart = 0;
    int _countdownSeconds = 30; // 30 second countdown before reboot
    
    const char* _ntpServer = "pool.ntp.org";
    long _gmtOffset_sec = -5 * 3600;     // EST (adjust for your timezone)
    int _daylightOffset_sec = 3600;     // Daylight saving time
    
  public:
    DailyReboot() {}
    
    // Configuration
    void setRebootTime(int hour, int minute) {
      _rebootHour = hour;
      _rebootMinute = minute;
    }
    
    void setTimezone(long gmtOffsetSec, int daylightOffsetSec) {
      _gmtOffset_sec = gmtOffsetSec;
      _daylightOffset_sec = daylightOffsetSec;
    }
    
    void setCountdownDuration(int seconds) {
      _countdownSeconds = seconds;
    }
    
    void enable() { _enabled = true; }
    void disable() { _enabled = false; }
    bool isEnabled() const { return _enabled; }
    
    // Setup NTP time synchronization
    bool setupNTP() {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⏰ DailyReboot: WiFi not connected for NTP setup");
        return false;
      }
      
      Serial.println("⏰ DailyReboot: Configuring NTP time...");
      configTime(_gmtOffset_sec, _daylightOffset_sec, _ntpServer);
      
      // Wait for time to be set
      int attempts = 0;
      struct tm timeinfo;
      while (!getCurrentTime(&timeinfo) && attempts < 10) {
        Serial.print(".");
        delay(1000);
        attempts++;
      }
      
      if (attempts < 10) {
        _ntpConfigured = true;
        Serial.println("\n⏰ DailyReboot: NTP time configured successfully");
        printCurrentTime();
        return true;
      } else {
        Serial.println("\n❌ DailyReboot: Failed to configure NTP time");
        return false;
      }
    }
    
    // Get current local time
    bool getCurrentTime(struct tm* timeinfo) {
      time_t now;
      time(&now);
      struct tm* tm_ptr = localtime(&now);
      if (tm_ptr && timeinfo) {
        *timeinfo = *tm_ptr;
        return (timeinfo->tm_year > (2020 - 1900)); // Check if time is reasonable
      }
      return false;
    }
    
    // Print current time for debugging
    void printCurrentTime() {
      struct tm timeinfo;
      if (getCurrentTime(&timeinfo)) {
        Serial.printf("⏰ Current time: %02d:%02d:%02d %02d/%02d/%04d\n",
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                     timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
      }
    }
    
    // Main update function - call this in your main loop
    void update() {
      if (!_enabled || !_ntpConfigured) {
        return;
      }
      
      unsigned long now = millis();
      
      // Check time periodically
      if (now - _lastTimeCheck >= _timeCheckInterval) {
        checkRebootTime();
        _lastTimeCheck = now;
      }
      
      // Handle countdown if started
      if (_countdownStarted) {
        handleCountdown();
      }
    }
    
    // Check if it's time to reboot
    void checkRebootTime() {
      struct tm timeinfo;
      if (!getCurrentTime(&timeinfo)) {
        return;
      }
      
      // Check if we're at the reboot time
      if (timeinfo.tm_hour == _rebootHour && timeinfo.tm_min == _rebootMinute && !_countdownStarted) {
        Serial.printf("🔄 DailyReboot: Reboot time reached (%02d:%02d) - starting countdown\n", 
                     _rebootHour, _rebootMinute);
        startCountdown();
      }
    }
    
    // Start the reboot countdown
    void startCountdown() {
      _countdownStarted = true;
      _countdownStart = millis();
      Serial.printf("⚠️ DailyReboot: SYSTEM WILL REBOOT IN %d SECONDS\n", _countdownSeconds);
    }
    
    // Handle the countdown process
    void handleCountdown() {
      unsigned long elapsed = (millis() - _countdownStart) / 1000;
      int remaining = _countdownSeconds - elapsed;
      
      if (remaining <= 0) {
        performReboot();
      } else if (remaining <= 10 || (remaining % 5 == 0 && remaining <= 30)) {
        // Show countdown for last 10 seconds, or every 5 seconds for last 30
        Serial.printf("⚠️ DailyReboot: Rebooting in %d seconds...\n", remaining);
      }
    }
    
    // Perform the actual reboot
    void performReboot() {
      Serial.println("🔄 DailyReboot: REBOOTING NOW - Daily maintenance reboot");
      Serial.println("🔄 DailyReboot: System will restart automatically...");
      Serial.flush(); // Make sure all serial output is sent
      
      delay(1000); // Give time for serial output
      
      // Perform ESP32 restart
      ESP.restart();
    }
    
    // Force immediate reboot (for testing or manual trigger)
    void forceReboot(const String& reason = "Manual reboot") {
      Serial.printf("🔄 DailyReboot: Force reboot requested - %s\n", reason.c_str());
      startCountdown();
    }
    
    // Get next reboot time as string
    String getNextRebootTime() {
      char buffer[20];
      snprintf(buffer, sizeof(buffer), "%02d:%02d", _rebootHour, _rebootMinute);
      return String(buffer);
    }
    
    // Get time until next reboot in minutes
    int getMinutesUntilReboot() {
      struct tm timeinfo;
      if (!getCurrentTime(&timeinfo)) {
        return -1;
      }
      
      int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
      int rebootMinutes = _rebootHour * 60 + _rebootMinute;
      
      int diff = rebootMinutes - currentMinutes;
      if (diff <= 0) {
        diff += 24 * 60; // Add 24 hours if reboot time has passed today
      }
      
      return diff;
    }
    
    // Print status information
    void printStatus() {
      Serial.println("=== DailyReboot Status ===");
      Serial.printf("Enabled: %s\n", _enabled ? "Yes" : "No");
      Serial.printf("NTP Configured: %s\n", _ntpConfigured ? "Yes" : "No");
      Serial.printf("Reboot Time: %s\n", getNextRebootTime().c_str());
      Serial.printf("Minutes Until Reboot: %d\n", getMinutesUntilReboot());
      Serial.printf("Countdown Active: %s\n", _countdownStarted ? "Yes" : "No");
      printCurrentTime();
      Serial.println("==========================");
    }
};

#endif
