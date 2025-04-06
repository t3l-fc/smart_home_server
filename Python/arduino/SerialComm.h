/*
 * SerialComm.h - Serial Communication Module
 * 
 * Handles all serial communication and command processing
 */

#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <Arduino.h>
#include "ServerComm.h"

class SerialComm {
  private:
    String _inputString;         // String to hold incoming serial data
    bool _stringComplete;        // Whether the string is complete
    ServerComm* _server;         // Pointer to the server communication object
    
  public:
    // Constructor
    SerialComm(ServerComm* server) {
      _inputString = "";
      _stringComplete = false;
      _server = server;
    }
    
    // Initialize serial communication
    void begin(unsigned long baud) {
      Serial.begin(baud);
      delay(1000);
      Serial.println("\nESP32 Multi-Device Smart Plug Controller Starting...");
      Serial.println("Type 'help' for a list of commands");
    }
    
    // Process any available serial data
    bool update() {
      bool activity = false;
      
      // Read serial data
      while (Serial.available()) {
        char inChar = (char)Serial.read();
        if (inChar == '\n' || inChar == '\r') {
          _stringComplete = true;
        } else {
          _inputString += inChar;
        }
        activity = true;  // Any serial input is considered activity
      }
      
      // Process completed commands
      if (_stringComplete) {
        processCommand(_inputString);
        _inputString = "";
        _stringComplete = false;
        activity = true;
      }
      
      return activity;
    }
    
    // Process a command from serial
    void processCommand(String command) {
      command.trim();
      command.toLowerCase();
      
      Serial.print("\nCommand received: ");
      Serial.println(command);
    
      // Split command into parts (command and device)
      String cmd, device;
      int spaceIndex = command.indexOf(' ');
      
      if (spaceIndex != -1) {
        cmd = command.substring(0, spaceIndex);
        device = command.substring(spaceIndex + 1);
        device.trim();
      } else {
        cmd = command;
        device = "all";  // default to all devices
      }
      
      Serial.print("Command: ");
      Serial.print(cmd);
      Serial.print(", Device: ");
      Serial.println(device);
      
      if (cmd == "help") {
        showHelp();
      }
      else if (cmd == "list") {
        _server->listDevices();
      }
      else if (cmd == "wifi") {
        Serial.println("Reconnecting to WiFi...");
        _server->connectWiFi();
      }
      else if (cmd == "status") {
        _server->getStatus(device);
      }
      else if (cmd == "on" || cmd == "off") {
        _server->controlDevice(device, cmd);
      }
      else {
        Serial.println("Unknown command. Type 'help' for available commands.");
      }
    }
    
    // Show help message
    void showHelp() {
      Serial.println("Available commands:");
      Serial.println("  status [device] - Check status (e.g., 'status cactus' or 'status all')");
      Serial.println("  on [device]     - Turn on (e.g., 'on ananas' or 'on all')");
      Serial.println("  off [device]    - Turn off (e.g., 'off dino' or 'off all')");
      Serial.println("  list            - List all available devices");
      Serial.println("  help            - Show this help message");
      Serial.println("  wifi            - Reconnect to WiFi");
      Serial.println("\nDevices can be: cactus, ananas, dino, vinyle, or all");
    }
};

#endif // SERIAL_COMM_H 