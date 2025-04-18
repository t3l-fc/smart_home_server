/*
 * SwitchManager.h - Physical Switch Management Module
 * 
 * Handles the 5 toggle switches with the following behavior:
 * - Individual switches toggle their respective plugs (Cactus, Ananas, Dino, Vinyle)
 * - Individual switches can always turn OFF
 * - Individual switches can turn ON only when the "All Plugs" switch is ON
 * - When "All Plugs" turns ON, all smart plugs with their switches in ON position turn ON
 * - When "All Plugs" turns OFF, all smart plugs turn OFF
 */

#ifndef SWITCH_MANAGER_H
#define SWITCH_MANAGER_H

#include <Arduino.h>
#include "ServerComm.h"

class SwitchManager {
  private:
    // Pin definitions
    const int _cactusPin;   // was _plug1Pin
    const int _ananasPin;   // was _plug2Pin
    const int _dinoPin;     // was _plug3Pin
    const int _vinylePin;
    const int _allPlugsPin;
    
    // Switch position states (physical state)
    bool _cactusSwitchOn;   // was _plug1SwitchOn
    bool _ananasSwitchOn;   // was _plug2SwitchOn
    bool _dinoSwitchOn;     // was _plug3SwitchOn
    bool _vinyleSwitchOn;
    bool _allPlugsSwitchOn;
    
    // Last read switch states (for debouncing)
    bool _lastCactusState;  // was _lastPlug1State
    bool _lastAnanasState;  // was _lastPlug2State
    bool _lastDinoState;    // was _lastPlug3State
    bool _lastVinyleState;
    bool _lastAllPlugsState;
    
    // Debounce timing
    unsigned long _lastDebounceTime[5];
    unsigned long _debounceDelay;
    
    // Server communication
    ServerComm* _server;
    
  public:
    // Constructor
    SwitchManager(int cactusPin, int ananasPin, int dinoPin, int vinylePin, int allPlugsPin, 
                 ServerComm* server, unsigned long debounceDelay = 50) :
      _cactusPin(cactusPin),
      _ananasPin(ananasPin),
      _dinoPin(dinoPin),
      _vinylePin(vinylePin),
      _allPlugsPin(allPlugsPin),
      _server(server),
      _debounceDelay(debounceDelay) {
      
      // Initialize switch states
      _cactusSwitchOn = false;
      _ananasSwitchOn = false;
      _dinoSwitchOn = false;
      _vinyleSwitchOn = false;
      _allPlugsSwitchOn = false;
      
      // Initialize last read states
      _lastCactusState = HIGH;
      _lastAnanasState = HIGH;
      _lastDinoState = HIGH;
      _lastVinyleState = HIGH;
      _lastAllPlugsState = HIGH;
      
      // Initialize debounce times
      for (int i = 0; i < 5; i++) {
        _lastDebounceTime[i] = 0;
      }
    }
    
    // Initialize pins
    void begin() {
      pinMode(_cactusPin, INPUT_PULLUP);
      pinMode(_ananasPin, INPUT_PULLUP);
      pinMode(_dinoPin, INPUT_PULLUP);
      pinMode(_vinylePin, INPUT_PULLUP);
      pinMode(_allPlugsPin, INPUT_PULLUP);
      
      Serial.println("Switch Manager initialized");
      Serial.println("- Cactus switch on pin " + String(_cactusPin));
      Serial.println("- Ananas switch on pin " + String(_ananasPin));
      Serial.println("- Dino switch on pin " + String(_dinoPin));
      Serial.println("- Vinyle switch on pin " + String(_vinylePin));
      Serial.println("- All Plugs switch on pin " + String(_allPlugsPin));
      
      // Read initial switch positions
      _cactusSwitchOn = (digitalRead(_cactusPin) == LOW);
      _ananasSwitchOn = (digitalRead(_ananasPin) == LOW);
      _dinoSwitchOn = (digitalRead(_dinoPin) == LOW);
      _vinyleSwitchOn = (digitalRead(_vinylePin) == LOW);
      _allPlugsSwitchOn = (digitalRead(_allPlugsPin) == LOW);
      
      Serial.println("Initial switch positions:");
      Serial.println("- Cactus: " + String(_cactusSwitchOn ? "ON" : "OFF"));
      Serial.println("- Ananas: " + String(_ananasSwitchOn ? "ON" : "OFF"));
      Serial.println("- Dino: " + String(_dinoSwitchOn ? "ON" : "OFF"));
      Serial.println("- Vinyle: " + String(_vinyleSwitchOn ? "ON" : "OFF"));
      Serial.println("- All Plugs: " + String(_allPlugsSwitchOn ? "ON" : "OFF"));
      
      // Apply initial state based on switch positions
      if (_allPlugsSwitchOn) {
        // Turn on only the devices with their switches in ON position
        if (_cactusSwitchOn) _server->controlDevice("cactus", "on");
        if (_ananasSwitchOn) _server->controlDevice("ananas", "on");
        if (_dinoSwitchOn) _server->controlDevice("dino", "on");
        if (_vinyleSwitchOn) _server->controlDevice("vinyle", "on");
      } else {
        // All plugs should be off initially if master switch is off
        _server->controlDevice("all", "off");
      }
    }
    
    // Update switch states and perform actions
    bool update() {
      bool activity = false;
      
      // Check individual switches first to track their positions
      activity |= checkIndividualSwitchPosition(0, _cactusPin, _lastCactusState, _cactusSwitchOn, "cactus");
      activity |= checkIndividualSwitchPosition(1, _ananasPin, _lastAnanasState, _ananasSwitchOn, "ananas");
      activity |= checkIndividualSwitchPosition(2, _dinoPin, _lastDinoState, _dinoSwitchOn, "dino");
      activity |= checkIndividualSwitchPosition(3, _vinylePin, _lastVinyleState, _vinyleSwitchOn, "vinyle");
      
      // Check the master switch last
      activity |= checkMasterSwitch();
      
      return activity;
    }
    
    // Get current plug states
    bool isCactusOn() const { return _cactusSwitchOn && _allPlugsSwitchOn; }
    bool isAnanasOn() const { return _ananasSwitchOn && _allPlugsSwitchOn; }
    bool isDinoOn() const { return _dinoSwitchOn && _allPlugsSwitchOn; }
    bool isVinyleOn() const { return _vinyleSwitchOn && _allPlugsSwitchOn; }
    bool isAllPlugsOn() const { return _allPlugsSwitchOn; }
    
    // Initialize pins without applying the states
    void beginWithoutApplying() {
      pinMode(_cactusPin, INPUT_PULLUP);
      pinMode(_ananasPin, INPUT_PULLUP);
      pinMode(_dinoPin, INPUT_PULLUP);
      pinMode(_vinylePin, INPUT_PULLUP);
      pinMode(_allPlugsPin, INPUT_PULLUP);
      
      Serial.println("Switch Manager initialized (without applying states)");
      Serial.println("- Cactus switch on pin " + String(_cactusPin));
      Serial.println("- Ananas switch on pin " + String(_ananasPin));
      Serial.println("- Dino switch on pin " + String(_dinoPin));
      Serial.println("- Vinyle switch on pin " + String(_vinylePin));
      Serial.println("- All Plugs switch on pin " + String(_allPlugsPin));
      
      // Read initial switch positions
      _cactusSwitchOn = (digitalRead(_cactusPin) == LOW);
      _ananasSwitchOn = (digitalRead(_ananasPin) == LOW);
      _dinoSwitchOn = (digitalRead(_dinoPin) == LOW);
      _vinyleSwitchOn = (digitalRead(_vinylePin) == LOW);
      _allPlugsSwitchOn = (digitalRead(_allPlugsPin) == LOW);
      
      Serial.println("Initial switch positions (not yet applied):");
      Serial.println("- Cactus: " + String(_cactusSwitchOn ? "ON" : "OFF"));
      Serial.println("- Ananas: " + String(_ananasSwitchOn ? "ON" : "OFF"));
      Serial.println("- Dino: " + String(_dinoSwitchOn ? "ON" : "OFF"));
      Serial.println("- Vinyle: " + String(_vinyleSwitchOn ? "ON" : "OFF"));
      Serial.println("- All Plugs: " + String(_allPlugsSwitchOn ? "ON" : "OFF"));
    }
    
    // Read direct switch positions (not actual device state)
    bool isCactusSwitchOn() const { return _cactusSwitchOn; }
    bool isAnanasSwitchOn() const { return _ananasSwitchOn; }
    bool isDinoSwitchOn() const { return _dinoSwitchOn; }
    bool isVinyleSwitchOn() const { return _vinyleSwitchOn; }
    
    // Apply initial switch states based on their positions
    void applyInitialSwitchStates() {
      if (_allPlugsSwitchOn) {
        // Master switch is on, turn on applicable devices
        if (_cactusSwitchOn) {
          _server->controlDevice("cactus", "on");
        }
        if (_ananasSwitchOn) {
          _server->controlDevice("ananas", "on");
        }
        if (_dinoSwitchOn) {
          _server->controlDevice("dino", "on");
        }
        if (_vinyleSwitchOn) {
          _server->controlDevice("vinyle", "on");
        }
      } else {
        // Master switch is off, ensure all devices are off
        _server->controlDevice("all", "off");
      }
    }
    
  private:
    // Check the master switch
    bool checkMasterSwitch() {
      // Read the current state
      bool currentState = digitalRead(_allPlugsPin);
      
      // Check if the state has changed
      if (currentState != _lastAllPlugsState) {
        // Reset debounce timer
        _lastDebounceTime[4] = millis();
      }
      
      // Check if enough time has passed for debounce
      if ((millis() - _lastDebounceTime[4]) > _debounceDelay) {
        // If state has changed after debounce
        if (currentState != _lastAllPlugsState) {
          bool newMasterState = (currentState == LOW); // LOW = switch in ON position
          
          // State changed from OFF to ON
          if (newMasterState && !_allPlugsSwitchOn) {
            Serial.println("Master Switch turned ON");
            
            // Turn on only the devices that have their individual switches in ON position
            if (_cactusSwitchOn) {
              Serial.println("- Turning Cactus ON (switch is ON)");
              _server->controlDevice("cactus", "on");
            }
            
            if (_ananasSwitchOn) {
              Serial.println("- Turning Ananas ON (switch is ON)");
              _server->controlDevice("ananas", "on");
            }
            
            if (_dinoSwitchOn) {
              Serial.println("- Turning Dino ON (switch is ON)");
              _server->controlDevice("dino", "on");
            }
            
            if (_vinyleSwitchOn) {
              Serial.println("- Turning Vinyle ON (switch is ON)");
              _server->controlDevice("vinyle", "on");
            }
          }
          // State changed from ON to OFF
          else if (!newMasterState && _allPlugsSwitchOn) {
            Serial.println("Master Switch turned OFF - Turning all devices OFF");
            _server->controlDevice("all", "off");
          }
          
          // Update the master switch state
          _allPlugsSwitchOn = newMasterState;
          _lastAllPlugsState = currentState;
          
          return true;
        }
      }
      
      return false;
    }
    
    // Check individual switch positions
    bool checkIndividualSwitchPosition(int switchIndex, int pin, bool &lastState, bool &switchOn, const String &deviceId) {
      // Read the current state
      bool currentState = digitalRead(pin);
      
      // Check if the state has changed
      if (currentState != lastState) {
        // Reset debounce timer
        _lastDebounceTime[switchIndex] = millis();
      }
      
      // Check if enough time has passed for debounce
      if ((millis() - _lastDebounceTime[switchIndex]) > _debounceDelay) {
        // If state has changed after debounce
        if (currentState != lastState) {
          bool newSwitchState = (currentState == LOW); // LOW = switch in ON position
          
          // State changed from OFF to ON
          if (newSwitchState && !switchOn) {
            // Only allow turning ON if master switch is ON
            if (_allPlugsSwitchOn) {
              Serial.printf("%s switch turned ON - Turning device ON\n", deviceId.c_str());
              _server->controlDevice(deviceId, "on");
              switchOn = true;
            }
          }
          // State changed from ON to OFF
          else if (!newSwitchState && switchOn) {
            Serial.printf("%s switch turned OFF - Turning device OFF\n", deviceId.c_str());
            _server->controlDevice(deviceId, "off");
            switchOn = false;
          }
          
          // Update the switch state
          lastState = currentState;
          
          return true;
        }
      }
      
      return false;
    }
};

#endif // SWITCH_MANAGER_H 