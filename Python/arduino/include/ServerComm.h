/*
 * ServerComm.h - Server Communication Module
 * 
 * Handles all communication with the Smart Home Server
 */

#ifndef SERVER_COMM_H
#define SERVER_COMM_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class ServerComm {
  private:
    String _serverUrl = "http://192.168.1.100:8080";
    const char* _ssid = "Hyrule";
    const char* _password = "stevebegin";

  public:
    bool setup() {
      connectWiFi();
      listDevices();

      return true;
    }
    
    // Connect to WiFi
    bool connectWiFi() {
      //Serial.print("Connecting to WiFi");
      WiFi.begin(_ssid, _password);
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        //Serial.println("\nConnected to WiFi!");
        //Serial.print("IP address: ");
        //Serial.println(WiFi.localIP());
        return true;
      } else {
        //Serial.println("\nFailed to connect to WiFi!");
        return false;
      }
    }
    
    // Check WiFi connection and reconnect if needed
    bool checkWiFi() {
      if (WiFi.status() != WL_CONNECTED) {
        //Serial.println("WiFi not connected. Reconnecting...");
        return connectWiFi();
      }
      return true;
    }
    
    // List all available devices
    bool listDevices() {
      if (WiFi.status() != WL_CONNECTED) return false;
      
      HTTPClient http;
      Serial.println("Getting list of devices...");
      
      String url = _serverUrl + "/api/devices";
      http.begin(url);
      
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("\nAvailable devices:");
          
          DynamicJsonDocument doc(1024);
          DeserializationError error = deserializeJson(doc, payload);
          
          if (!error) {
            JsonObject devices = doc["devices"];
            for (JsonPair device : devices) {
              const char* key = device.key().c_str();
              const char* name = device.value()["name"];
              Serial.printf("  - %s (%s)\n", name, key);
            }
          }
        } else {
          Serial.printf("HTTP error: %d\n", httpCode);
          String payload = http.getString();
          Serial.println("Error response: " + payload);
          return false;
        }
      } else {
        Serial.printf("Failed to get devices list: %s\n", http.errorToString(httpCode).c_str());
        return false;
      }
      
      http.end();

      return true;
    }
    
    // Get status of one or all devices
    void getStatus(String device) {
      if (!checkWiFi()) return;
      
      HTTPClient http;
      String url;
      
      if (device == "all") {
        url = _serverUrl + "/api/status/all";
        Serial.println("Checking status of all devices...");
      } else {
        url = _serverUrl + "/api/status/" + device;
        Serial.printf("Checking status of device: %s\n", device.c_str());
      }
      
      Serial.print("URL: ");
      Serial.println(url);
      
      http.begin(url);
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Response: " + payload);
          
          DynamicJsonDocument doc(2048);
          DeserializationError error = deserializeJson(doc, payload);
          
          if (!error) {
            if (device == "all") {
              JsonObject results = doc["results"];
              for (JsonPair device : results) {
                const char* deviceName = device.key().c_str();
                bool isOn = false;
                
                JsonVariant result = device.value()["result"]["result"];
                if (result.is<JsonArray>()) {
                  for (JsonVariant item : result.as<JsonArray>()) {
                    if (item["code"] == "switch_1") {
                      isOn = item["value"].as<bool>();
                      break;
                    }
                  }
                }
                
                Serial.printf("  %s: %s\n", deviceName, isOn ? "ON" : "OFF");
              }
            } else {
              bool isOn = false;
              JsonArray result = doc["result"]["result"].as<JsonArray>();
              for (JsonVariant item : result) {
                if (item["code"] == "switch_1") {
                  isOn = item["value"].as<bool>();
                  break;
                }
              }
              Serial.printf("  %s: %s\n", device.c_str(), isOn ? "ON" : "OFF");
            }
          } else {
            Serial.print("JSON parsing error: ");
            Serial.println(error.c_str());
          }
        } else {
          Serial.printf("HTTP error code: %d\n", httpCode);
          String payload = http.getString();
          Serial.println("Error response: " + payload);
        }
      } else {
        Serial.printf("Failed to get status: %s\n", http.errorToString(httpCode).c_str());
      }
      
      http.end();
    }
    
    // Control one or all devices
    void controlDevice(String device, String action) {
      if (!checkWiFi()) return;
      
      HTTPClient http;
      String url;
      
      if (device == "all") {
        url = _serverUrl + "/api/control/all";
        Serial.printf("Setting all devices to: %s\n", action.c_str());
      } else {
        url = _serverUrl + "/api/control/" + device;
        Serial.printf("Setting %s to: %s\n", device.c_str(), action.c_str());
      }
      
      Serial.print("URL: ");
      Serial.println(url);
      
      // Prepare JSON payload
      DynamicJsonDocument doc(128);
      doc["action"] = action;
      String requestBody;
      serializeJson(doc, requestBody);
      
      Serial.print("Request body: ");
      Serial.println(requestBody);
      
      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      
      int httpCode = http.POST(requestBody);
      
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Response: " + payload);
          
          // Get updated status after a brief delay
          delay(1000);
          getStatus(device);
        } else {
          Serial.printf("HTTP error: %d\n", httpCode);
          String payload = http.getString();
          Serial.println("Error response: " + payload);
        }
      } else {
        Serial.printf("Failed to send command: %s\n", http.errorToString(httpCode).c_str());
      }
      
      http.end();
    }
    
    // Control a device directly (used during initialization)
    void controlDeviceDirectly(String device, String action) {
      if (!checkWiFi()) return;
      
      Serial.print("Directly controlling ");
      Serial.print(device);
      Serial.print(" -> ");
      Serial.println(action);
      
      // The rest of this method should be the same as your existing
      // controlDevice implementation in the ServerComm class
    }
};

#endif // SERVER_COMM_H 