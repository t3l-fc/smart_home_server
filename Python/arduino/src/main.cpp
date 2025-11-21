#include <Arduino.h>
#include "DisplayManager.h"
#include "SwitchManager.h"
#include "ServerComm.h"
#include "CommManager.h"
#include "DisplayService.h"
#include "RenderManager.h"
#include "DailyReboot.h"
#include "params.h"

DisplayManager displayManager = DisplayManager();
SwitchManager switchManager = SwitchManager();
ServerComm serverComm = ServerComm();
CommManager commManager = CommManager();
DisplayService displayService = DisplayService(&displayManager);
RenderManager renderManager = RenderManager();
DailyReboot dailyReboot = DailyReboot();

// Function prototype
void updateSwitchsState();
void updateDisplay();
bool connectWiFi();
void myCallBack(char *data, uint16_t len);

void setup() {
  // Start serial with a high baud rate
  Serial.begin(115200);
  
  Serial.println("�� STARTING SETUP");
  Serial.println("⚠️  AllPlugs switch temporarily DISABLED - individual switches only");
  
  // Initialize the display manually first
  Serial.print("Display setup : ");
  bool displaySetupOk = displayManager.setup();
  Serial.println(displaySetupOk ? "OK" : "KO");
  
  // Show initial boot message
  displayManager.display("BOOT");
  delay(1000);
  
  // Now initialize the display service
  Serial.print("Starting display service...");
  bool serviceOk = displayService.begin();
  Serial.println(serviceOk ? "OK" : "KO");
  
  delay(1000); // Give the task some time to start
  
  // Continue with the rest of the setup
  Serial.print("SwitchManager setup : ");
  Serial.println(switchManager.setup() ? "OK" : "KO");
  
  // WiFi connection with animated display
  Serial.print("WiFi setup : ");
  displayService.setState(STATE_WIFI_CONNECTING);
  bool wifiOk = connectWiFi();
  displayService.setState(wifiOk ? STATE_WIFI_OK : STATE_WIFI_FAIL);

  // MQTT connection with animated display
  Serial.print("CommManager setup : ");
  displayService.setState(STATE_MQTT_CONNECTING);
  bool mqttOk = commManager.setup();
  displayService.setState(mqttOk ? STATE_MQTT_OK : STATE_MQTT_FAIL);
  
  Serial.print("Setting up subscribe...");
  commManager.setupSubscribe(myCallBack);
  Serial.println("done");
  
  // Configure RenderManager with your Deploy Hook URL
  renderManager.setDeployHookUrl("https://api.render.com/deploy/srv-cviu4tmuk2gs73avnerg?key=uGdGkY0iAIQ");
  renderManager.setCheckInterval(10 * 60 * 1000); // Check every 10 minutes
  renderManager.setMaxFailures(1); // For boot check, redeploy after 1 failure
  renderManager.enable();
  
  Serial.print("RenderManager setup : ");
  Serial.println("OK - Monitoring server health");
  
  // Perform immediate health check after boot
  Serial.println("🚀 Performing initial server health check...");
  displayService.setState(STATE_MQTT_CONNECTING); // Reuse connecting animation for server check
  
  bool serverHealthy = renderManager.performHealthCheck();
  if (serverHealthy) {
    Serial.println("✅ Server is healthy at boot - no action needed");
    displayService.setState(STATE_MQTT_OK);
  } else {
    Serial.println("⚠️ Server not responding at boot - redeploy may have been triggered");
    displayService.setState(STATE_MQTT_FAIL);
    delay(2000); // Show the error state briefly
  }
  
  // Reset to normal failure threshold for ongoing monitoring
  renderManager.setMaxFailures(2);
  
  // Configure DailyReboot system
  Serial.print("DailyReboot setup : ");
  dailyReboot.setRebootTime(3, 0); // Reboot at 3:00 AM
  dailyReboot.setTimezone(-5 * 3600, 3600); // EST timezone (adjust for your location)
  dailyReboot.setCountdownDuration(30); // 30 second countdown
  
  bool ntpOk = dailyReboot.setupNTP();
  Serial.println(ntpOk ? "OK - Daily reboot at 3:00 AM" : "FAILED - NTP sync failed");
  
  if (ntpOk) {
    dailyReboot.enable();
    Serial.printf("⏰ Next reboot: %s (in %d minutes)\n", 
                 dailyReboot.getNextRebootTime().c_str(), 
                 dailyReboot.getMinutesUntilReboot());
  }
  
  // Set to ready state for normal operation
  displayService.setState(STATE_READY);
  
  // Display the initial state of the switches
  updateDisplay();
  Serial.println("Setup complete - Device always awake and responsive!");
  Serial.println("🔍 MONITORING: Watch for AllPlugs debug messages...");
  Serial.println("🚀 RENDER: Automatic server monitoring and redeploy enabled");
}

void loop() {
  // ⚠️ AllPlugs monitoring DISABLED - switch temporarily broken
  
  switchManager.update();
  updateSwitchsState();
  
  // Update RenderManager - monitors server health and triggers redeploy if needed
  renderManager.update();
  
  // Update DailyReboot - checks for daily reboot time
  dailyReboot.update();

  delay(10); // Slightly longer delay
  commManager.mqtt->loop();
  
  yield(); // Give other tasks time to run
}

// isChanged() doit être une et une seule fois par boucle
void updateSwitchsState() {
   // ⚠️ AllPlugs temporarily DISABLED - individual switches work normally
   
   if(switchManager.isAnanasChanged()) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("ananas", switchManager.isAnanasOn());
    Serial.printf("🍍 Ananas: %s\n", switchManager.isAnanasOn() ? "ON" : "OFF");
   }

   if(switchManager.isDinoChanged()) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("dino", switchManager.isDinoOn());
    Serial.printf("🦕 Dino: %s\n", switchManager.isDinoOn() ? "ON" : "OFF");
   }

   if(switchManager.isCactusChanged()) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("cactus", switchManager.isCactusOn());
    Serial.printf("🌵 Cactus: %s\n", switchManager.isCactusOn() ? "ON" : "OFF");
   }

   if(switchManager.isVinyleChanged()) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("vinyle", switchManager.isVinyleOn());
    Serial.printf("💿 Vinyle: %s\n", switchManager.isVinyleOn() ? "ON" : "OFF");
   }

   if(switchManager.isBasketChanged()) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("basket", switchManager.isBasketOn());
    Serial.printf("🧺 Basket: %s\n", switchManager.isBasketOn() ? "ON" : "OFF");
   }
}

// Connect to WiFi
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  Serial.println("");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Failed to connect to WiFi");
    return false;
  }
}

void updateDisplay() {
  // Update the display service with individual switch states (AllPlugs disabled)
  // Order: Vinyle, Ananas, Dino, Cactus, Basket (left to right physically)
  String statusString = "";
  
  // Build a status string showing individual switch states in physical order
  statusString += switchManager.isVinyleOn() ? "1" : "0";   // Position 1 (gauche)
  statusString += switchManager.isAnanasOn() ? "1" : "0";   // Position 2
  statusString += switchManager.isDinoOn() ? "1" : "0";     // Position 3  
  statusString += switchManager.isCactusOn() ? "1" : "0";   // Position 4
  statusString += switchManager.isBasketOn() ? "1" : "0";   // Position 5 (droite)
  
  // Add server status as 6th character (if display supports it)
  // H=Healthy, C=Checking, E=Error, D=Deploying, -=Unknown
  statusString += renderManager.getStatusChar();
  
  displayService.showStatus(statusString);
}

void myCallBack(char *data, uint16_t len) {
  if (!data || len == 0) {
      Serial.println("Invalid MQTT message received");
      return;
  }
  
  // ❌ REMOVED: displayService.registerActivity(); - Only physical switches should wake display
  
  Serial.print("📨 MQTT message received (len:");
  Serial.print(len);
  Serial.println(")");
  Serial.print("Msg length: ");
  Serial.println(len);
  
  Serial.println(data);
}