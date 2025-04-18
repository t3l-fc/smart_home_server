#include <Arduino.h>
#include "Display.h"
#include "SwitchManager.h"

Display alpha4 = Display();
SwitchManager switchManager = SwitchManager();

// Function prototype
void updateSwitchsState();

void setup() {
  Serial.begin(115200);
  delay(4000);
  
  Serial.println("Display setup");
  alpha4.setup();

  Serial.println("SwitchManager setup");
  switchManager.begin();

  // Display the initial state of the switches
  alpha4.display(
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

void updateSwitchsState() {
   if(
    switchManager.isVinyleChanged() ||
    switchManager.isAnanasChanged() ||
    switchManager.isDinoChanged() ||
    switchManager.isCactusChanged() 
   ) {
      alpha4.display(
        String(switchManager.isVinyleOn()) +
        String(switchManager.isAnanasOn()) +
        String(switchManager.isDinoOn()) +
        String(switchManager.isCactusOn())
      );
  }
}