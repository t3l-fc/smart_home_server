#include <Arduino.h>
#include "Display.h"
#include "SwitchManager.h"

Display alpha4 = Display();
SwitchManager switchManager = SwitchManager();

void setup() {
  Serial.begin(115200);
  delay(4000);
  
  Serial.println("Display setup");
  alpha4.setup();

  Serial.println("SwitchManager setup");
  switchManager.begin();
  
  delay(1000);
}

void loop() {
  switchManager.update();

  if(switchManager.isVinyleChanged()) {
    Serial.println("Vinyle changed: " + String(switchManager.isVinyleOn()));
  }

  if(switchManager.isAnanasChanged()) {
    Serial.println("Ananas changed: " + String(switchManager.isAnanasOn()));
  }

  if(switchManager.isDinoChanged()) {
    Serial.println("Dino changed: " + String(switchManager.isDinoOn()));
  }

  if(switchManager.isCactusChanged()) {
    Serial.println("Cactus changed: " + String(switchManager.isCactusOn()));
  }

  if(switchManager.isAllPlugsChanged()) {
    Serial.println("All Plugs changed: " + String(switchManager.isAllPlugsOn()));
  }
  
  
}