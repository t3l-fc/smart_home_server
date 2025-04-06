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
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>

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

// Deep sleep configuration
const uint64_t SLEEP_TIMEOUT_MS = 60000;  // 60 seconds
uint64_t lastActivityTime = 0;
bool deepSleepEnabled = true;

// Define the pins used in our project so we can set unused pins properly
const int USED_PINS[] = {ledPin, cactusPin, ananasPin, dinoPin, vinylePin, allPlugsPin, sdaPin, sclPin};
const int NUM_USED_PINS = sizeof(USED_PINS) / sizeof(USED_PINS[0]);

// Create communication objects
ServerComm server(serverUrl, ssid, password);
SerialComm serial(&server);
SwitchManager switches(cactusPin, ananasPin, dinoPin, vinylePin, allPlugsPin, &server);
DisplayManager display;

// Add a boot sequence variable
bool initialBootSequenceComplete = false;

// Record activity to reset sleep timer
void recordActivity() {
  lastActivityTime = millis();
  digitalWrite(ledPin, HIGH);  // Visual indicator of activity
  delay(50);
  digitalWrite(ledPin, LOW);
}

// Configure deep sleep wake sources
void configureWakeupSources() {
  // Define which pins can wake the ESP32
  const uint64_t wakeupPins = (1ULL << cactusPin) | (1ULL << ananasPin) | 
                             (1ULL << dinoPin) | (1ULL << vinylePin) | 
                             (1ULL << allPlugsPin);
  
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
    for (int j = 0; j < NUM_USED_PINS; j++) {
      if (i == USED_PINS[j]) {
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
  display.setPower(false);
  
  // 2. Put I2C pins in high-impedance state (INPUT_PULLUP)
  Wire.end();
  pinMode(sdaPin, INPUT_PULLUP);
  pinMode(sclPin, INPUT_PULLUP);
  
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
  
  // 7. Configure wake sources
  configureWakeupSources();
  
  // 8. Turn off LED
  digitalWrite(ledPin, LOW);
  
  // 9. Configure power domains for maximum savings
  // Disable unnecessary power domains (RTC peripherals will stay on for wake-up)
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
  
  // 10. Flash power saving options
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
  applyInitialSwitchStates();
}

// Apply initial switch states based on their positions
void applyInitialSwitchStates() {
  if (switches.isAllPlugsOn()) {
    // Master switch is on, turn on applicable devices
    if (switches.isCactusSwitchOn()) {
      server.controlDevice("cactus", "on");
    }
    if (switches.isAnanasSwitchOn()) {
      server.controlDevice("ananas", "on");
    }
    if (switches.isDinoSwitchOn()) {
      server.controlDevice("dino", "on");
    }
    if (switches.isVinyleSwitchOn()) {
      server.controlDevice("vinyle", "on");
    }
  } else {
    // Master switch is off, ensure all devices are off
    server.controlDevice("all", "off");
  }
}

void setup() {
  // Initialize serial communication
  serial.begin(115200);
  
  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // Turn on LED while booting
  
  // Setup after wake-up
  initAfterWakeUp();
  
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
  recordActivity();
  
  Serial.println("Device ready. Will enter deep sleep after 60 seconds of inactivity.");
}

void loop() {
  // Process serial commands (these will record activity)
  if (serial.update()) {
    recordActivity();
  }
  
  // Check toggle switches (these will record activity if changes detected)
  if (switches.update()) {
    recordActivity();
  }
  
  // Update display with current plug states
  display.update(
    switches.isCactusOn(),
    switches.isAnanasOn(),
    switches.isDinoOn(),
    switches.isVinyleOn()
  );
  
  // Check if it's time to sleep
  if (deepSleepEnabled && millis() - lastActivityTime > SLEEP_TIMEOUT_MS) {
    enterDeepSleep();
  }
} 