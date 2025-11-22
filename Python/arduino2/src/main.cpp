#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// WiFi credentials
#define WLAN_SSID "Hyrule"
#define WLAN_PASS "stevebegin"

// Adafruit IO MQTT settings
#define AIO_SERVER  "io.adafruit.com"
#define AIO_SERVERPORT  8883
#define AIO_USERNAME  "marsouino"
#define AIO_KEY "2e4dabd28afe424085715d39cb85311a"
#define MQTT_FEED "marsouino/feeds/smart_plugs"

// LED PWM settings
#define LED_PIN 14           // GPIO pin for LED
#define PWM_CHANNEL 0       // LEDC channel (0-15)
#define PWM_FREQUENCY 15000  // PWM frequency in Hz
#define PWM_RESOLUTION 8    // 8-bit resolution (0-255)

// PWM clamping limits (0% and 100% brightness will map to these PWM values)
#define PWM_MIN 10          // PWM value for 0% brightness (prevents LED from being completely off)
#define PWM_MAX 240         // PWM value for 100% brightness (prevents LED from being at maximum)

// WiFi and MQTT clients
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

// Current brightness (0-100%)
int currentBrightness = 0;
// Last brightness value before turning off (for basket:on)
int lastBrightness = 50; // Default value if basket:on is called before any brightness setting
// Flag to track if LED is really off (via basket:off)
bool isLedOff = false;

// Function declarations
void connectWiFi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setBrightness(int percent);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 LED PWM MQTT Controller ===");
  
  // Setup LED PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, PWM_MIN); // Start with LED at minimum brightness
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT
  setupMQTT();
  
  Serial.println("Setup complete!");
}

void loop() {
  // Maintain MQTT connection
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.loop();
  
  delay(10);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WLAN_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  // Set root CA certificate for Adafruit IO (optional but recommended)
  // For Adafruit IO, we can use a basic certificate or skip verification
  // Note: For production, you should verify the certificate
  secureClient.setInsecure(); // Skip certificate verification (for testing)
  
  mqtt.setServer(AIO_SERVER, AIO_SERVERPORT);
  mqtt.setCallback(mqttCallback);
  
  // Connect to MQTT
  reconnectMQTT();
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(AIO_SERVER);
    Serial.print(":");
    Serial.println(AIO_SERVERPORT);
    
    // Create client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    
    // Connect with username and password
    if (mqtt.connect(clientId.c_str(), AIO_USERNAME, AIO_KEY)) {
      Serial.println("MQTT connected!");
      
      // Subscribe to the feed
      String topic = String(MQTT_FEED);
      if (mqtt.subscribe(topic.c_str())) {
        Serial.print("Subscribed to: ");
        Serial.println(topic);
        Serial.println("Waiting for retained brightness message...");
        
        // Process MQTT messages to receive retained message
        // Only brightness:xx messages are retained, so we'll receive it here if it exists
        // Retained messages are automatically sent when subscribing
        for (int i = 0; i < 10; i++) {
          mqtt.loop();
          delay(100);
        }
        Serial.println("Retained brightness message processed (if available)");
      } else {
        Serial.println("Failed to subscribe!");
      }
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Convert payload to string
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print(message);
  
  // Check if this might be a retained message (at startup)
  // Note: PubSubClient doesn't directly tell us if it's retained,
  // but we can infer it if received right after subscription
  Serial.println(" (processing...)");
  
  // Parse "brightness:xx" format
  // Note: Only brightness messages are retained, so this handles startup state restoration
  if (message.startsWith("brightness:")) {
    int colonIndex = message.indexOf(':');
    if (colonIndex != -1) {
      String brightnessStr = message.substring(colonIndex + 1);
      int brightness = brightnessStr.toInt();
      
      // Clamp brightness to 0-100
      if (brightness < 0) brightness = 0;
      if (brightness > 100) brightness = 100;
      
      // If LED is off (via basket:off), update brightness value but don't turn on
      if (isLedOff) {
        currentBrightness = brightness;
        // Update lastBrightness if the new value is not 0
        if (brightness > 0) {
          lastBrightness = brightness;
        }
        Serial.print("Brightness updated to: ");
        Serial.print(brightness);
        Serial.println("% (LED remains off)");
      } else {
        // Normal behavior: set brightness and turn on LED
        setBrightness(brightness);
        // Update lastBrightness if the new value is not 0
        if (brightness > 0) {
          lastBrightness = brightness;
        }
      }
    }
  }
  // Parse "basket:on" - restore last brightness value
  else if (message.equals("basket:on")) {
    Serial.println("Basket ON - restoring last brightness");
    isLedOff = false; // LED is being turned on
    setBrightness(lastBrightness);
  }
  // Parse "basket:off" - set brightness to 0 (real 0, no clamping)
  else if (message.equals("basket:off")) {
    Serial.println("Basket OFF - setting brightness to 0 (real off)");
    // Save current brightness before turning off (if not already 0)
    if (currentBrightness > 0) {
      lastBrightness = currentBrightness;
    }
    currentBrightness = 0;
    isLedOff = true; // Mark LED as really off
    // Set PWM to real 0 (bypass clamping)
    ledcWrite(PWM_CHANNEL, 0);
    Serial.println("LED turned off (PWM: 0)");
  }
}

void setBrightness(int percent) {
  currentBrightness = percent;
  isLedOff = false; // LED is being turned on/controlled
  
  // Convert percentage (0-100) to PWM value (PWM_MIN to PWM_MAX)
  int pwmValue = map(percent, 0, 100, PWM_MIN, PWM_MAX);
  
  // Set PWM value
  ledcWrite(PWM_CHANNEL, pwmValue);
  
  Serial.print("Brightness set to: ");
  Serial.print(percent);
  Serial.print("% (PWM: ");
  Serial.print(pwmValue);
  Serial.println(")");
}

