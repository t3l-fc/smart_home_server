#include <Arduino.h>
#include "DisplayManager.h"
#include "SwitchManager.h"
#include "ServerComm.h"
#include "CommManager.h"
#include "params.h"

DisplayManager displayManager = DisplayManager();
SwitchManager switchManager = SwitchManager();
ServerComm serverComm = ServerComm();
CommManager commManager = CommManager();

// Function prototype
void updateSwitchsState();
void updateDisplay();
bool connectWiFi();
void myCallBack(char *data, uint16_t len);

void setup() {
  Serial.begin(115200);
  delay(8000);
  
  Serial.print("Display setup : ");
  Serial.println(displayManager.setup() ? "OK" : "KO");

  Serial.print("SwitchManager setup : ");
  Serial.println(switchManager.setup() ? "OK" : "KO");
  
  Serial.print("WiFi setup : ");
  Serial.println(connectWiFi() ? "OK" : "KO");

  Serial.print("CommManager setup : ");
  Serial.println(commManager.setup() ? "OK" : "KO");
  commManager.setupSubscribe(myCallBack);
  
  commManager.publish("set", "ananas", switchManager.isAnanasOn());

  //Serial.print("WiFi setup : ");
  //Serial.println(serverComm.connectWiFi() ? "OK" : "KO");

  //Serial.print("Server setup : ");
  //Serial.println(serverComm.listDevices() ? "OK" : "KO");

  // Display the initial state of the switches
  displayManager.display(
    String(switchManager.isVinyleOn()) +
    String(switchManager.isAnanasOn()) +
    String(switchManager.isDinoOn()) +
    String(switchManager.isCactusOn())
  );
  
  delay(1000);
}

void loop() {
  switchManager.update();
  updateSwitchsState();

  delay(1);
  commManager.mqtt->loop();
  yield();
}

// isChanged() doit être une et une seule fois par boucle
void updateSwitchsState() {
   if(
    switchManager.isAnanasChanged()
   ) {
    updateDisplay();
    commManager.publish("set", "ananas", switchManager.isAnanasOn());
    //serverComm.controlDevice("ananas", switchManager.isAnanasOn() ? "on" : "off");
   }

   if(
    switchManager.isDinoChanged()
   ) {
    updateDisplay();
    //serverComm.controlDevice("dino", switchManager.isDinoOn() ? "on" : "off");
   }

   if(
    switchManager.isCactusChanged()
   ) {
    updateDisplay();
    //serverComm.controlDevice("cactus", switchManager.isCactusOn() ? "on" : "off");
   }

   if(
    switchManager.isVinyleChanged()
   ) {
    updateDisplay();
    serverComm.controlDevice("vinyle", switchManager.isVinyleOn() ? "on" : "off");
   }  
}

void updateDisplay() {
  displayManager.display(
    String(switchManager.isVinyleOn()) +
    String(switchManager.isAnanasOn()) +
    String(switchManager.isDinoOn()) +
    String(switchManager.isCactusOn())
  );
}

// Connect to WiFi
bool connectWiFi() {
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }
  
  return WiFi.status() == WL_CONNECTED;
}

void myCallBack(char *data, uint16_t len) {
  Serial.println("myCallBack");
  if (!data || len == 0) {
      Serial.println("Invalid MQTT message received");
      return;
  }
  
  Serial.print("Msg length: ");
  Serial.println(len);
  
  Serial.println(data);
}