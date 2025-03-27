/*
 * Smart Plug Controller for ESP32 Feather HUZZAH32 (MQTT Version)
 * Controls a Teckin smart plug via a Python relay server using MQTT
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";      // Replace with your WiFi SSID
const char* password = "YourWiFiPassword"; // Replace with your WiFi password

// MQTT Broker settings
const char* mqtt_server = "your-mqtt-broker.com";  // Replace with your MQTT broker
const int mqtt_port = 1883;                        // Standard MQTT port
const char* mqtt_username = "your-mqtt-username";  // Replace with your MQTT username
const char* mqtt_password = "your-mqtt-password";  // Replace with your MQTT password
const char* client_id = "ESP32_SmartPlugController"; // Client ID for MQTT

// MQTT Topics
const char* mqtt_topic_command = "tuya/command";
const char* mqtt_topic_status = "tuya/status";

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

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Function prototypes
void setupWiFi();
void reconnectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void togglePlug();
void updateLED();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  // Connect to WiFi
  setupWiFi();
  
  // Set up MQTT client
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Initial connection to MQTT
  reconnectMQTT();
  
  // Get initial status by sending a status request
  if (client.connected()) {
    StaticJsonDocument<200> doc;
    doc["action"] = "status";
    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(mqtt_topic_command, buffer);
    Serial.println("Requesting initial status");
  }
  
  Serial.println("ESP32 Smart Plug Controller (MQTT) Ready");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    setupWiFi();
  }
  
  // Check MQTT connection
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
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
    
    if (client.connected()) {
      StaticJsonDocument<200> doc;
      doc["action"] = "status";
      char buffer[256];
      serializeJson(doc, buffer);
      client.publish(mqtt_topic_command, buffer);
      Serial.println("Requesting status update");
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

void reconnectMQTT() {
  // Loop until we're reconnected
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe to the status topic
      client.subscribe(mqtt_topic_status);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      attempts++;
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  // Create a buffer to store the payload as a string
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  message[length] = '\0';
  Serial.println();
  
  // If message is from the status topic
  if (strcmp(topic, mqtt_topic_status) == 0) {
    // Parse the JSON response
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
      if (doc["status"] == "success") {
        // Check if this is a status response
        if (doc.containsKey("action") && doc["action"] == "status") {
          // The exact path to the switch state depends on the Tuya device's 
          // response format - you may need to adjust this
          if (doc["result"]["result"].containsKey("switch_1")) {
            bool newState = doc["result"]["result"]["switch_1"];
            
            // Update state and LED if changed
            if (newState != deviceState) {
              deviceState = newState;
              updateLED();
              Serial.print("Status updated. Plug is ");
              Serial.println(deviceState ? "ON" : "OFF");
            }
          }
        }
        // Check if this is a control response
        else if (doc.containsKey("action") && (doc["action"] == "on" || doc["action"] == "off")) {
          deviceState = (doc["action"] == "on");
          updateLED();
          Serial.print("Control command successful. Plug is ");
          Serial.println(deviceState ? "ON" : "OFF");
        }
      }
    } else {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
    }
  }
}

void togglePlug() {
  if (!client.connected()) {
    Serial.println("MQTT not connected. Cannot toggle plug.");
    return;
  }
  
  // Create the JSON message
  StaticJsonDocument<200> doc;
  doc["action"] = deviceState ? "off" : "on";
  
  char buffer[256];
  serializeJson(doc, buffer);
  
  // Publish the message
  bool published = client.publish(mqtt_topic_command, buffer);
  
  if (published) {
    Serial.print("Sent toggle command: ");
    Serial.println(buffer);
  } else {
    Serial.println("Failed to send toggle command");
  }
}

void updateLED() {
  // Update LED based on device state
  digitalWrite(LED_PIN, deviceState ? HIGH : LOW);
} 