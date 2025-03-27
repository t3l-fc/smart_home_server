# Smart Plug Controller Architecture

This document outlines the architecture of the Smart Plug Controller project, which enables an ESP32 to control a Teckin smart plug via Tuya's API.

## System Overview

![System Architecture](system_architecture.png)

The system consists of three main components:

1. **ESP32 Feather HUZZAH32**: The physical controller with WiFi connectivity
2. **Python Relay Server**: A cloud-hosted server acting as a bridge
3. **Tuya Cloud**: The cloud service that manages the smart plug

## Communication Flow

1. **User Interaction**: A user presses a button on the ESP32 or the system checks the status periodically
2. **ESP32 to Relay Server**: The ESP32 sends an HTTP request or MQTT message to the relay server
3. **Relay Server to Tuya Cloud**: The relay server uses Tuya API to send commands to the smart plug
4. **Response Path**: The response follows the reverse path back to the ESP32

## Components in Detail

### ESP32 Controller

- **Hardware**: ESP32 Feather HUZZAH32 with button and LED
- **Communication**: WiFi connection to the internet
- **Protocol**: HTTP or MQTT (two options provided)
- **Responsibilities**:
  - Connect to WiFi
  - Send commands to the relay server
  - Receive and display status updates

### Python Relay Server

- **Platform**: Cloud-hosted (e.g., Render, Heroku, PythonAnywhere)
- **Framework**: Flask for HTTP API
- **Libraries**: tinytuya for Tuya API communication, paho-mqtt for MQTT
- **Endpoints**:
  - `/api/control`: For sending on/off commands
  - `/api/status`: For getting the current status
  - `/health`: For health checks
- **MQTT Topics** (if enabled):
  - `tuya/command`: For receiving commands
  - `tuya/status`: For publishing status updates

### Tuya Cloud

- **API**: Tuya Cloud API
- **Credentials**: Access ID, Access Key, and Region provided by Tuya
- **Device**: Teckin Smart Plug registered in the Tuya platform

## Data Flow

### Device Control Flow

```
ESP32 → HTTP POST /api/control → Relay Server → Tuya API → Smart Plug
```

OR with MQTT:

```
ESP32 → MQTT Publish (tuya/command) → Relay Server → Tuya API → Smart Plug
```

### Status Update Flow

```
ESP32 → HTTP GET /api/status → Relay Server → Tuya API → (Response) → ESP32
```

OR with MQTT:

```
ESP32 → MQTT Publish (tuya/command, action:status) → Relay Server → Tuya API
       → Relay Server → MQTT Publish (tuya/status) → ESP32
```

## Security Considerations

- Tuya credentials are stored as environment variables on the server
- HTTPS is used for all HTTP communications
- MQTT authentication is available when using MQTT
- No credentials are hardcoded in the Arduino code

## Fail-safe Design

- The ESP32 periodically checks the connection and reconnects if necessary
- The relay server uses error handling to ensure robustness
- The system maintains the last known state if communication fails
- HTTP timeouts and MQTT reconnect logic are implemented

## Future Enhancements

1. Support for multiple smart plugs
2. Web interface for monitoring and control
3. Integration with additional sensors on the ESP32
4. Power consumption monitoring and reporting 