#include <Arduino.h>
#include "DisplayManager.h"
#include "SwitchManager.h"
#include "ServerComm.h"

DisplayManager displayManager = DisplayManager();
SwitchManager switchManager = SwitchManager();
ServerComm serverComm = ServerComm();

// Function prototype
void updateSwitchsState();
void updateDisplay();
void setup() {
  Serial.begin(115200);
  delay(4000);
  
  Serial.print("Display setup : ");
  Serial.println(displayManager.setup() ? "OK" : "KO");

  Serial.print("SwitchManager setup : ");
  Serial.println(switchManager.setup() ? "OK" : "KO");

  Serial.print("WiFi setup : ");
  Serial.println(serverComm.connectWiFi() ? "OK" : "KO");

  Serial.print("Server setup : ");
  Serial.println(serverComm.listDevices() ? "OK" : "KO");

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
}

// isChanged() doit être une et une seule fois par boucle
void updateSwitchsState() {
   if(
    switchManager.isAnanasChanged()
   ) {
    updateDisplay();
    serverComm.controlDevice("ananas", switchManager.isAnanasOn() ? "on" : "off");
   }

   if(
    switchManager.isDinoChanged()
   ) {
    updateDisplay();
    serverComm.controlDevice("dino", switchManager.isDinoOn() ? "on" : "off");
   }

   if(
    switchManager.isCactusChanged()
   ) {
    updateDisplay();
    serverComm.controlDevice("cactus", switchManager.isCactusOn() ? "on" : "off");
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