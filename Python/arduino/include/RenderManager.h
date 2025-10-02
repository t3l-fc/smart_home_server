/*
 * RenderManager.h - Automatic Render Server Management
 * 
 * Monitors the Render server health and automatically triggers
 * redeployment when the server becomes unresponsive.
 * 
 * Features:
 * - Periodic health checks (every 10 minutes)
 * - Automatic redeploy on server failure
 * - Status reporting for display
 */

#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

enum ServerStatus {
  SERVER_UNKNOWN = 0,
  SERVER_HEALTHY = 1,
  SERVER_CHECKING = 2,
  SERVER_FAILED = 3,
  SERVER_REDEPLOYING = 4
};

class RenderManager {
  private:
    String _serverUrl = "https://smart-home-server-a076.onrender.com";
    String _deployHookUrl = ""; // À configurer avec votre Deploy Hook URL
    
    unsigned long _lastCheckTime = 0;
    unsigned long _checkInterval = 10 * 60 * 1000; // 10 minutes en millisecondes
    
    ServerStatus _currentStatus = SERVER_UNKNOWN;
    int _consecutiveFailures = 0;
    int _maxFailuresBeforeRedeploy = 2; // Redéploie après 2 échecs consécutifs
    
    bool _enabled = true;
    
  public:
    RenderManager() {}
    
    // Configuration
    void setServerUrl(const String& url) { _serverUrl = url; }
    void setDeployHookUrl(const String& url) { _deployHookUrl = url; }
    void setCheckInterval(unsigned long intervalMs) { _checkInterval = intervalMs; }
    void setMaxFailures(int maxFailures) { _maxFailuresBeforeRedeploy = maxFailures; }
    void enable() { _enabled = true; }
    void disable() { _enabled = false; }
    
    // Status getters
    ServerStatus getStatus() const { return _currentStatus; }
    int getConsecutiveFailures() const { return _consecutiveFailures; }
    bool isEnabled() const { return _enabled; }
    
    // Main update function - call this in your main loop
    void update() {
      if (!_enabled || WiFi.status() != WL_CONNECTED) {
        return;
      }
      
      unsigned long now = millis();
      
      // Check if it's time for a health check
      if (now - _lastCheckTime >= _checkInterval) {
        performHealthCheck();
        _lastCheckTime = now;
      }
    }
    
    // Force an immediate health check
    bool performHealthCheck() {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("🔌 RenderManager: WiFi not connected");
        _currentStatus = SERVER_FAILED;
        return false;
      }
      
      Serial.println("🔍 RenderManager: Checking server health...");
      _currentStatus = SERVER_CHECKING;
      
      HTTPClient http;
      http.begin(_serverUrl + "/health");
      http.setTimeout(30000); // 30 second timeout
      
      int httpCode = http.GET();
      
      if (httpCode == 200) {
        // Server is healthy
        Serial.println("✅ RenderManager: Server is healthy");
        _currentStatus = SERVER_HEALTHY;
        _consecutiveFailures = 0;
        http.end();
        return true;
      } else {
        // Server failed
        _consecutiveFailures++;
        Serial.printf("❌ RenderManager: Server check failed (HTTP %d), failures: %d/%d\n", 
                     httpCode, _consecutiveFailures, _maxFailuresBeforeRedeploy);
        
        _currentStatus = SERVER_FAILED;
        http.end();
        
        // Trigger redeploy if we've reached the failure threshold
        if (_consecutiveFailures >= _maxFailuresBeforeRedeploy) {
          return triggerRedeploy();
        }
        
        return false;
      }
    }
    
    // Manually trigger a redeploy
    bool triggerRedeploy() {
      if (_deployHookUrl.length() == 0) {
        Serial.println("❌ RenderManager: Deploy hook URL not configured");
        return false;
      }
      
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("🔌 RenderManager: WiFi not connected for redeploy");
        return false;
      }
      
      Serial.println("🚀 RenderManager: Triggering server redeploy...");
      _currentStatus = SERVER_REDEPLOYING;
      
      HTTPClient http;
      http.begin(_deployHookUrl);
      http.addHeader("Content-Type", "application/json");
      
      // Send POST request to trigger redeploy
      int httpCode = http.POST("{}");
      
      if (httpCode >= 200 && httpCode < 300) {
        Serial.printf("✅ RenderManager: Redeploy triggered successfully (HTTP %d)\n", httpCode);
        _consecutiveFailures = 0; // Reset failure counter
        
        // Wait a bit then check if server is back up
        delay(5000);
        http.end();
        
        // Give the server some time to redeploy before next check
        _lastCheckTime = millis() + (2 * 60 * 1000); // Wait 2 more minutes
        
        return true;
      } else {
        Serial.printf("❌ RenderManager: Redeploy failed (HTTP %d)\n", httpCode);
        _currentStatus = SERVER_FAILED;
        http.end();
        return false;
      }
    }
    
    // Get status as string for display
    String getStatusString() const {
      switch (_currentStatus) {
        case SERVER_HEALTHY: return "OK";
        case SERVER_CHECKING: return "CHK";
        case SERVER_FAILED: return "ERR";
        case SERVER_REDEPLOYING: return "DEP";
        default: return "---";
      }
    }
    
    // Get status as single character for 7-segment display
    char getStatusChar() const {
      switch (_currentStatus) {
        case SERVER_HEALTHY: return 'H';      // Healthy
        case SERVER_CHECKING: return 'C';     // Checking
        case SERVER_FAILED: return 'E';       // Error
        case SERVER_REDEPLOYING: return 'D';  // Deploying
        default: return '-';
      }
    }
    
    // Print detailed status to Serial
    void printStatus() const {
      Serial.println("=== RenderManager Status ===");
      Serial.printf("Enabled: %s\n", _enabled ? "Yes" : "No");
      Serial.printf("Status: %s\n", getStatusString().c_str());
      Serial.printf("Consecutive Failures: %d/%d\n", _consecutiveFailures, _maxFailuresBeforeRedeploy);
      Serial.printf("Server URL: %s\n", _serverUrl.c_str());
      Serial.printf("Deploy Hook: %s\n", _deployHookUrl.length() > 0 ? "Configured" : "Not configured");
      Serial.printf("Check Interval: %lu minutes\n", _checkInterval / (60 * 1000));
      Serial.println("============================");
    }
};

#endif
