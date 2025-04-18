#ifndef SWITCH_MANAGER_H
#define SWITCH_MANAGER_H

#include <Arduino.h>

class SwitchManager {
  private:
    // Pin definitions
    const int _vinylePin = 33;
    const int _ananasPin = 15;
    const int _dinoPin = 32;
    const int _cactusPin = 14;
    const int _allPlugsPin = 27;
    
    // Switch position states (physical state)
    bool _vinyleSwitch;
    bool _ananasSwitch;
    bool _dinoSwitch;
    bool _cactusSwitch;
    bool _allPlugsSwitch;

    bool _previousVinyleState;
    bool _previousAnanasState;
    bool _previousDinoState;
    bool _previousCactusState;
    bool _previousAllPlugsState;

    const int _debounceTime = 100;
    unsigned long _lastDebounceTime = 0;
    
  public:
    // Initialize pins
    void begin() {
      pinMode(_cactusPin, INPUT_PULLUP);
      pinMode(_ananasPin, INPUT_PULLUP);
      pinMode(_dinoPin, INPUT_PULLUP);
      pinMode(_vinylePin, INPUT_PULLUP);
      pinMode(_allPlugsPin, INPUT_PULLUP);

      // Initialize switch states
      _vinyleSwitch = !digitalRead(_vinylePin);
      _ananasSwitch = !digitalRead(_ananasPin);
      _dinoSwitch = !digitalRead(_dinoPin);
      _cactusSwitch = !digitalRead(_cactusPin);
      _allPlugsSwitch = !digitalRead(_allPlugsPin);

      _previousVinyleState = _vinyleSwitch;
      _previousAnanasState = _ananasSwitch;
      _previousDinoState = _dinoSwitch;
      _previousCactusState = _cactusSwitch;
      _previousAllPlugsState = _allPlugsSwitch;
      
      Serial.println("Switch Manager initialized");
      Serial.println("- Vinyle switch on pin " + String(_vinylePin));
      Serial.println("- Ananas switch on pin " + String(_ananasPin));
      Serial.println("- Dino switch on pin " + String(_dinoPin));
      Serial.println("- Cactus switch on pin " + String(_cactusPin));
      Serial.println("- All Plugs switch on pin " + String(_allPlugsPin));
    }
    
    void update() {
      if(millis() - _lastDebounceTime < _debounceTime) return;

      if(digitalRead(_vinylePin) && _vinyleSwitch) {
        _vinyleSwitch = false;
        _lastDebounceTime = millis();
      }
      else if(!digitalRead(_vinylePin) && !_vinyleSwitch) {
        _vinyleSwitch = true;
        _lastDebounceTime = millis();
      }

      if(digitalRead(_ananasPin) && _ananasSwitch) {
        _ananasSwitch = false;
        _lastDebounceTime = millis();
      }
      else if(!digitalRead(_ananasPin) && !_ananasSwitch) {
        _ananasSwitch = true;
        _lastDebounceTime = millis();
      }

      if(digitalRead(_dinoPin) && _dinoSwitch) {
        _dinoSwitch = false;
        _lastDebounceTime = millis();
      }
      else if(!digitalRead(_dinoPin) && !_dinoSwitch) {
        _dinoSwitch = true;
        _lastDebounceTime = millis();
      }

      if(digitalRead(_cactusPin) && _cactusSwitch) {
        _cactusSwitch = false;
        _lastDebounceTime = millis();
      }
      else if(!digitalRead(_cactusPin) && !_cactusSwitch) {
        _cactusSwitch = true;
        _lastDebounceTime = millis();
      }

      if(digitalRead(_allPlugsPin) && _allPlugsSwitch) {
        _allPlugsSwitch = false;
        _lastDebounceTime = millis();
      }
      else if(!digitalRead(_allPlugsPin) && !_allPlugsSwitch) {
        _allPlugsSwitch = true;
        _lastDebounceTime = millis();
      }
    }
    
    bool isVinyleChanged() {
      if(_previousVinyleState != _vinyleSwitch) {
        _previousVinyleState = _vinyleSwitch;
        return true;
      }
      return false;
    }

    bool isAnanasChanged() {
      if(_previousAnanasState != _ananasSwitch) {
        _previousAnanasState = _ananasSwitch;
        return true;
      }
      return false;
    }
    
    bool isDinoChanged() {
      if(_previousDinoState != _dinoSwitch) {
        _previousDinoState = _dinoSwitch;
        return true;
      }
      return false;
    } 

    bool isCactusChanged() {
      if(_previousCactusState != _cactusSwitch) {
        _previousCactusState = _cactusSwitch;
        return true;
      }
      return false;
    }

    bool isAllPlugsChanged() {
      if(_previousAllPlugsState != _allPlugsSwitch) {
        _previousAllPlugsState = _allPlugsSwitch;
        return true;
      }
      return false;
    }

    bool isCactusOn() const { return _cactusSwitch; }
    bool isAnanasOn() const { return _ananasSwitch; }
    bool isDinoOn() const { return _dinoSwitch; }
    bool isVinyleOn() const { return _vinyleSwitch; }
    bool isAllPlugsOn() const { return _allPlugsSwitch; }
    
};

#endif // SWITCH_MANAGER_H 