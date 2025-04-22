#include <Arduino.h>
#include "DisplayManager.h"
#include "SwitchManager.h"
#include "ServerComm.h"
#include "CommManager.h"
#include "DisplayService.h"
#include "params.h"

DisplayManager displayManager = DisplayManager();
SwitchManager switchManager = SwitchManager();
ServerComm serverComm = ServerComm();
CommManager commManager = CommManager();
DisplayService displayService = DisplayService(&displayManager);

// Function prototype
void updateSwitchsState();
void updateDisplay();
bool connectWiFi();
void myCallBack(char *data, uint16_t len);

void setup() {
  // Start serial with a high baud rate
  Serial.begin(115200);
  //delay(3000); // Longer delay to ensure serial is ready
  
  //Serial.println("Starting setup...");
  
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
  //delay(1000);

  // MQTT connection with animated display
  Serial.print("CommManager setup : ");
  displayService.setState(STATE_MQTT_CONNECTING);
  bool mqttOk = commManager.setup();
  displayService.setState(mqttOk ? STATE_MQTT_OK : STATE_MQTT_FAIL);
  //delay(1000);
  
  Serial.print("Setting up subscribe...");
  commManager.setupSubscribe(myCallBack);
  Serial.println("done");
  
  // Set to ready state for normal operation
  displayService.setState(STATE_READY);
  
  // Display the initial state of the switches
  updateDisplay();
  Serial.println("Setup complete");
}

void loop() {
  switchManager.update();
  updateSwitchsState();

  delay(10); // Slightly longer delay
  commManager.mqtt->loop();
  yield(); // Give other tasks time to run
}

// Connect to WiFi
bool connectWiFi() {
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

// isChanged() doit être une et une seule fois par boucle
void updateSwitchsState() {
   if(
    switchManager.isAnanasChanged()
   ) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("ananas", switchManager.isAnanasOn());
   }

   if(
    switchManager.isDinoChanged()
   ) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("dino", switchManager.isDinoOn());
   }

   if(
    switchManager.isCactusChanged()
   ) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("cactus", switchManager.isCactusOn());
   }

   if(
    switchManager.isVinyleChanged()
   ) {
    displayService.registerActivity(); // Reset display timeout
    updateDisplay();
    commManager.controlDevice("vinyle", switchManager.isVinyleOn());
   }  
}

void updateDisplay() {
  // Update the display service with the current switch state
  displayService.showStatus(
    String(switchManager.isVinyleOn()) +
    String(switchManager.isAnanasOn()) +
    String(switchManager.isDinoOn()) +
    String(switchManager.isCactusOn())
  );
}

void myCallBack(char *data, uint16_t len) {
  if (!data || len == 0) {
      Serial.println("Invalid MQTT message received");
      return;
  }
  
  Serial.print("Msg length: ");
  Serial.println(len);
  
  Serial.println(data);
}