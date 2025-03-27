/*
 * Smart Plug Controller for ESP32 Feather HUZZAH32
 * Controls a Teckin smart plug via a Python relay server
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";      // Replace with your WiFi SSID
const char* password = "YourWiFiPassword"; // Replace with your WiFi password

// Relay server URL - replace with your deployed server URL
const char* relayServerUrl = "https://your-relay-server.herokuapp.com";

// Button pin for manual control
const int BUTTON_PIN = 12;  // Change to any available GPIO pin
// LED pin for status indication
const int LED_PIN = 13;     // Change to any available GPIO pin

// Variables to track button state
int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Variables to track device state
bool deviceState = false;

// Function prototypes
void setupWiFi();
bool getPlugStatus();
bool controlPlug(String action);
void togglePlug();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // Connect to WiFi
  setupWiFi();
  
  // Get initial status
  deviceState = getPlugStatus();
  updateLED();
  
  Serial.println("ESP32 Smart Plug Controller Ready");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    setupWiFi();
  }
  
  // Button debounce and state monitoring
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // When button is pressed (LOW when using INPUT_PULLUP)
      if (buttonState == LOW) {
        Serial.println("Button pressed - toggling plug");
        togglePlug();
      }
    }
  }
  
  lastButtonState = reading;
  
  // Periodic status check (every 30 seconds)
  static unsigned long lastStatusCheck = 0;
  if (millis() - lastStatusCheck > 30000) {
    lastStatusCheck = millis();
    bool currentState = getPlugStatus();
    
    // Update LED if state changed externally
    if (currentState != deviceState) {
      deviceState = currentState;
      updateLED();
      Serial.print("Status updated. Plug is ");
      Serial.println(deviceState ? "ON" : "OFF");
    }
  }
  
  delay(100);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  // Wait for connection - timeout after 20 seconds
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi. Check credentials or restart.");
  }
}

bool getPlugStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot check status.");
    return deviceState;
  }
  
  HTTPClient http;
  String url = String(relayServerUrl) + "/api/status";
  
  Serial.print("Getting status from: ");
  Serial.println(url);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Response: " + payload);
    
    // Parse the JSON response
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      if (doc["status"] == "success") {
        // The exact path to the switch state depends on the Tuya device's 
        // response format - you may need to adjust this
        if (doc["result"]["result"].containsKey("switch_1")) {
          bool state = doc["result"]["result"]["switch_1"];
          http.end();
          return state;
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
  
  http.end();
  return deviceState; // Return the current state if we couldn't update
}

bool controlPlug(String action) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot control plug.");
    return false;
  }
  
  HTTPClient http;
  String url = String(relayServerUrl) + "/api/control";
  
  Serial.print("Sending control command to: ");
  Serial.println(url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  String payload = "{\"action\":\"" + action + "\"}";
  
  int httpCode = http.POST(payload);
  bool success = false;
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    Serial.println("Response: " + response);
    
    // Parse the JSON response
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      if (doc["status"] == "success") {
        success = true;
        deviceState = (action == "on");
        updateLED();
      }
    } else {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP error code: ");
    Serial.println(httpCode);
  }
  
  http.end();
  return success;
}

void togglePlug() {
  // Toggle the device state
  bool success;
  if (deviceState) {
    success = controlPlug("off");
  } else {
    success = controlPlug("on");
  }
  
  if (success) {
    Serial.print("Successfully toggled plug to ");
    Serial.println(deviceState ? "ON" : "OFF");
  } else {
    Serial.println("Failed to toggle plug");
  }
}

void updateLED() {
  // Update LED based on device state
  digitalWrite(LED_PIN, deviceState ? HIGH : LOW);
} 