/*
 * ESP32 Smart Plug Controller with Deep Sleep and 7-Segment Display
 * 
 * Features:
 * - Controls multiple smart plugs via relay server
 * - 5 physical toggle switches for control
 * - 7-Segment display showing plug status
 * - Serial command interface when awake
 * - Deep sleep after 60 seconds of inactivity
 * - Wakes up when any switch changes state
 * - Optimized for maximum power savings during sleep
 * - Plugs named: Cactus, Ananas, Dino, and Vinyle
 */

#include "ServerComm.h"
#include "SerialComm.h"
#include "SwitchManager.h"
#include "DisplayManager.h"
#include "SleepManager.h"

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverUrl = "https://smart-home-server-a076.onrender.com";

// Pin definitions
const int ledPin = 13;       // Built-in LED on ESP32 Feather
const int cactusPin = 14;    // GPIO pin for Cactus switch (was plug1Pin)
const int ananasPin = 27;    // GPIO pin for Ananas switch (was plug2Pin)
const int dinoPin = 26;      // GPIO pin for Dino switch (was plug3Pin)
const int vinylePin = 25;    // GPIO pin for Vinyle plug switch
const int allPlugsPin = 33;  // GPIO pin for "All plugs" switch

// I2C pins for display
const int sdaPin = 21;       // SDA pin for I2C
const int sclPin = 22;       // SCL pin for I2C

// Define the pins used in our project so we can set unused pins properly
const int USED_PINS[] = {ledPin, cactusPin, ananasPin, dinoPin, vinylePin, allPlugsPin, sdaPin, sclPin};
const int NUM_USED_PINS = sizeof(USED_PINS) / sizeof(USED_PINS[0]);

// Create communication objects
ServerComm server(serverUrl, ssid, password);
SerialComm serial(&server);
DisplayManager display;
SwitchManager switches(cactusPin, ananasPin, dinoPin, vinylePin, allPlugsPin, &server);
SleepManager sleepManager(USED_PINS, NUM_USED_PINS, &display);

// Add a boot sequence variable
bool initialBootSequenceComplete = false;

void setup() {
 

  // Initialize serial communication
  Serial.begin(115200);

  while (!Serial) {
    delay(100);
  }
 delay(4000);

 while (true)
 {
  Serial.println("Starting setup");
 }
 
  Serial.println("Starting setup");
  
  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // Turn on LED while booting
  
  // Setup after wake-up
  sleepManager.initAfterWakeUp();
  
  // Initialize I2C for display
  Wire.begin(sdaPin, sclPin);
  
  // Initialize display
  if (display.begin()) {
    Serial.println("Display initialized successfully");
  }
  
  // Initialize toggle switches - but don't apply their states yet
  switches.beginWithoutApplying();
  
  // Run the startup sequence - this will handle WiFi connection
  // and apply switch states after countdown completes
  startupSequence();
  
  // Turn off boot LED
  digitalWrite(ledPin, LOW);
  
  // Record activity to start the sleep timer
  sleepManager.recordActivity();
  
  Serial.println("Device ready. Will enter deep sleep after 60 seconds of inactivity.");
}

void loop() {
  // Process serial commands (these will record activity)
  if (serial.update()) {
    sleepManager.recordActivity();
  }
  
  // Check toggle switches (these will record activity if changes detected)
  if (switches.update()) {
    sleepManager.recordActivity();
  }
  
  // Update display with current plug states
  display.update(
    switches.isCactusOn(),
    switches.isAnanasOn(),
    switches.isDinoOn(),
    switches.isVinyleOn()
  );
  
  // Check if it's time to sleep
  if (sleepManager.shouldSleep()) {
    // Configure wake-up sources before entering sleep
    const uint64_t wakeupPins = (1ULL << cactusPin) | (1ULL << ananasPin) | 
                               (1ULL << dinoPin) | (1ULL << vinylePin) | 
                               (1ULL << allPlugsPin);
    sleepManager.configureWakeupSources(wakeupPins);
    sleepManager.enterDeepSleep();
  }
}

// Execute startup sequence
void startupSequence() {
  Serial.println("Starting boot sequence with countdown...");
  
  // Start WiFi connection in the background
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Display the countdown while WiFi connects
  display.showCountdown(5);
  
  // Wait for WiFi if it's not connected yet
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 10) {
    Serial.print(".");
    delay(500);
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection timeout - will retry later");
  }
  
  // Mark boot sequence as complete
  initialBootSequenceComplete = true;
  Serial.println("Boot sequence complete - switches active now");
  
  // Now apply the initial switch states
  switches.applyInitialSwitchStates();
} 