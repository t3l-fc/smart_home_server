/*
 * ESP32 Smart Plug Controller
 * 
 * This sketch allows an ESP32 Feather HUZZAH32 to control a Tuya smart plug
 * through a relay server deployed on Render.
 * 
 * Control via Serial Monitor:
 * - Type "on" to turn the plug on
 * - Type "off" to turn the plug off
 * - Type "status" to check current status
 * - Type "help" to show available commands
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Hyrule";
const char* password = "stevebegin";

// Server details
const char* serverUrl = "https://smart-home-server-a076.onrender.com";

// Pin definitions
const int ledPin = 13;  // Built-in LED on ESP32 Feather

// Variables
bool deviceStatus = false;           // Current status of the plug (false = off)
unsigned long lastStatusCheck = 0;   // Last time we checked the status
const unsigned long statusInterval = 30000; // Status check interval (30 seconds)
String inputString = "";             // String to hold incoming serial data
bool stringComplete = false;         // Whether the string is complete

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Smart Plug Controller Starting...");
  Serial.println("Type 'help' for a list of commands");
  
  // Initialize pins
  pinMode(ledPin, OUTPUT);
  
  // Connect to WiFi
  connectWiFi();
  
  // Check initial status
  deviceStatus = getPlugStatus();
  
  // Set LED to match device status
  digitalWrite(ledPin, deviceStatus);
}

void loop() {
  // Process serial commands
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
  
  // Process completed commands
  if (stringComplete) {
    processSerialCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
  
  // Periodically check status
  if (millis() - lastStatusCheck > statusInterval) {
    bool newStatus = getPlugStatus();
    if (newStatus != deviceStatus) {
      deviceStatus = newStatus;
      digitalWrite(ledPin, deviceStatus);
      Serial.print("Status updated from server: ");
      Serial.println(deviceStatus ? "ON" : "OFF");
    }
    lastStatusCheck = millis();
  }
}

void processSerialCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  Serial.print("> ");
  Serial.println(command);
  
  if (command == "on") {
    Serial.println("Turning plug ON...");
    controlPlug(true);
    deviceStatus = true;
    digitalWrite(ledPin, deviceStatus);
  }
  else if (command == "off") {
    Serial.println("Turning plug OFF...");
    controlPlug(false);
    deviceStatus = false;
    digitalWrite(ledPin, deviceStatus);
  }
  else if (command == "status") {
    deviceStatus = getPlugStatus();
    Serial.print("Current plug status: ");
    Serial.println(deviceStatus ? "ON" : "OFF");
    digitalWrite(ledPin, deviceStatus);
  }
  else if (command == "help") {
    Serial.println("Available commands:");
    Serial.println("  on - Turn the plug ON");
    Serial.println("  off - Turn the plug OFF");
    Serial.println("  status - Check current plug status");
    Serial.println("  help - Show this help message");
    Serial.println("  wifi - Reconnect to WiFi");
  }
  else if (command == "wifi") {
    Serial.println("Reconnecting to WiFi...");
    connectWiFi();
  }
  else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

bool getPlugStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      return deviceStatus; // Return last known status if can't connect
    }
  }
  
  HTTPClient http;
  Serial.println("Checking device status...");
  
  // Make GET request to status endpoint
  String statusUrl = String(serverUrl) + "/api/status";
  http.begin(statusUrl);
  
  int httpCode = http.GET();
  bool status = deviceStatus; // Default to last known status
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Response: " + payload);
      
      // Parse JSON response
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        // Extract status from response - adjust this based on your API response structure
        if (doc["status"] == "success") {
          JsonVariant result = doc["result"];
          if (result.containsKey("result")) {
            JsonArray resultArray = result["result"].as<JsonArray>();
            for (JsonVariant item : resultArray) {
              if (item["code"] == "switch_1") {
                status = item["value"].as<bool>();
                break;
              }
            }
          }
        }
      } else {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP error code: ");
      Serial.println(httpCode);
    }
  } else {
    Serial.print("HTTP request failed: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  
  http.end();
  
  Serial.print("Device status: ");
  Serial.println(status ? "ON" : "OFF");
  
  return status;
}

void controlPlug(bool turnOn) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      return; // Can't proceed without WiFi
    }
  }
  
  HTTPClient http;
  Serial.print("Setting device state to: ");
  Serial.println(turnOn ? "ON" : "OFF");
  
  // Prepare JSON payload
  DynamicJsonDocument doc(128);
  doc["action"] = turnOn ? "on" : "off";
  String requestBody;
  serializeJson(doc, requestBody);
  
  // Make POST request to control endpoint
  String controlUrl = String(serverUrl) + "/api/control";
  http.begin(controlUrl);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(requestBody);
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Response: " + payload);
      
      // Parse JSON response to confirm success
      DynamicJsonDocument respDoc(1024);
      DeserializationError error = deserializeJson(respDoc, payload);
      
      if (!error) {
        if (respDoc["status"] == "success") {
          Serial.println("Command successful");
        } else {
          Serial.println("Command failed");
        }
      }
    } else {
      Serial.print("HTTP error code: ");
      Serial.println(httpCode);
    }
  } else {
    Serial.print("HTTP request failed: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  
  http.end();
}