#!/usr/bin/env python3
"""
Smart Plug Controller - MQTT to Tuya Relay
This standalone script connects to Adafruit IO MQTT broker and controls Tuya smart plugs.
"""

import os
import json
import time
import ssl
import threading
import sys
import paho.mqtt.client as mqtt
import tinytuya
import random
from http.server import HTTPServer, BaseHTTPRequestHandler

print("Starting Smart Plug Controller - Version 1.0")

# ===== CONFIGURATION =====
# All credentials in one place

# Adafruit IO MQTT Configuration
MQTT_BROKER = "io.adafruit.com"
MQTT_PORT = 8883
MQTT_USERNAME = "marsouino"
MQTT_PASSWORD = "2e4dabd28afe424085715d39cb85311a"
MQTT_FEED = "marsouino/feeds/smart_plugs"

# Tuya Configuration
TUYA_API_REGION = "us"
TUYA_API_KEY = "4kcffc9h34rwnswpncrj"
TUYA_API_SECRET = "d1dc602a4d684f8895e2fca36d8996d8"

# Smart Plug Devices
DEVICES = {
    'cactus': {
        'id': "eb06105cff43c50594rz5n",
        'name': "Cactus"
    },
    'ananas': {
        'id': "eb39191340ad46ad91wbug",
        'name': "Ananas"
    },
    'dino': {
        'id': "03310047840d8e87510e",
        'name': "Dino"
    },
    'vinyle': {
        'id': "eb6eef417508566dbf5mfh",
        'name': "Vinyle"
    }
}

# Global variables
mqtt_client = None
tuya_cloud = None

# ===== TUYA FUNCTIONS =====

def init_tuya():
    """Initialize connection to Tuya Cloud API"""
    global tuya_cloud
    
    print(f"Initializing Tuya Cloud API connection (Region: {TUYA_API_REGION})")
    tuya_cloud = tinytuya.Cloud(
        apiRegion=TUYA_API_REGION,
        apiKey=TUYA_API_KEY,
        apiSecret=TUYA_API_SECRET,
        apiDeviceID=next(iter(DEVICES.values()))['id']  # Use first device as default
    )
    print("Tuya Cloud API connected successfully")

def control_device(device_id, action):
    """Control a specific Tuya device"""
    if not tuya_cloud:
        print("ERROR: Tuya Cloud not initialized")
        return {"status": "error", "message": "Tuya Cloud not initialized"}
        
    try:
        print(f"Controlling device {device_id}: {action}")
        
        if action == "on":
            result = tuya_cloud.sendcommand(device_id, {"commands": [{"code": "switch_1", "value": True}]})
            return {"status": "success", "action": "on", "result": result}
        elif action == "off":
            result = tuya_cloud.sendcommand(device_id, {"commands": [{"code": "switch_1", "value": False}]})
            return {"status": "success", "action": "off", "result": result}
        elif action == "status":
            result = tuya_cloud.getstatus(device_id)
            return {"status": "success", "action": "status", "result": result}
        else:
            return {"status": "error", "message": f"Invalid action: {action}"}
    except Exception as e:
        print(f"ERROR controlling device {device_id}: {str(e)}")
        import traceback
        traceback.print_exc()
        return {"status": "error", "message": str(e)}

def control_all_devices(action):
    """Control all devices with the same action"""
    results = {}
    for key, device in DEVICES.items():
        results[key] = control_device(device['id'], action)
    return results

# ===== MQTT FUNCTIONS =====

def on_connect(client, userdata, flags, rc):
    """Callback when connected to MQTT broker"""
    if rc == 0:
        print(f"Connected to MQTT broker {MQTT_BROKER} successfully")
        # Subscribe to the feed
        client.subscribe(MQTT_FEED)
        print(f"Subscribed to {MQTT_FEED}")
        
        # Publish a connection message
        client.publish(MQTT_FEED, json.dumps({
            "status": "connected",
            "server": "smart_plug_relay",
            "devices": list(DEVICES.keys()),
            "timestamp": time.time()
        }))
    else:
        print(f"Failed to connect to MQTT broker, return code: {rc}")

def on_message(client, userdata, msg):
    """Callback when a message is received"""
    try:
        payload = msg.payload.decode()
        print(f"Received message: {payload}")
        
        # Check if this is a JSON message (likely a response, not a command)
        if payload.startswith('{') and payload.endswith('}'):
            print("Ignoring JSON message (probably a response, not a command)")
            return
        
        # Parse command (device:action)
        if ":" in payload:
            device_id, action = payload.split(":", 1)
            print(f"Parsed command: device={device_id}, action={action}")
            
            # Special case for "all" device
            if device_id.lower() == "all":
                print(f"Controlling ALL devices: {action}")
                result = control_all_devices(action)
                # Publish result
                client.publish(MQTT_FEED, json.dumps({
                    "device": "all",
                    "action": action,
                    "result": result,
                    "timestamp": time.time()
                }))
                return
            
            # Find the Tuya device ID
            tuya_device_id = None
            for key, device in DEVICES.items():
                if key == device_id:
                    tuya_device_id = device['id']
                    break
            
            if not tuya_device_id:
                print(f"ERROR: Unknown device: {device_id}")
                client.publish(MQTT_FEED, json.dumps({
                    "error": f"Unknown device: {device_id}",
                    "available_devices": list(DEVICES.keys()),
                    "timestamp": time.time()
                }))
                return
            
            # Control the device
            result = control_device(tuya_device_id, action)
            
            # Publish result back to MQTT
            client.publish(MQTT_FEED, json.dumps({
                "device": device_id,
                "action": action,
                "result": result,
                "timestamp": time.time()
            }))
            
        else:
            print(f"ERROR: Invalid message format: {payload}")
            client.publish(MQTT_FEED, json.dumps({
                "error": "Invalid message format",
                "expected": "device:action",
                "examples": ["cactus:on", "ananas:off", "all:status"],
                "timestamp": time.time()
            }))
            
    except Exception as e:
        print(f"ERROR processing message: {str(e)}")
        import traceback
        traceback.print_exc()
        
        # Try to publish error
        try:
            client.publish(MQTT_FEED, json.dumps({
                "error": f"Exception: {str(e)}",
                "timestamp": time.time()
            }))
        except:
            pass

def on_disconnect(client, userdata, rc):
    """Callback when disconnected from MQTT broker"""
    if rc != 0:
        print(f"Unexpected disconnection from MQTT broker, code: {rc}")
        print("Will automatically try to reconnect...")
    else:
        print("Disconnected from MQTT broker")

def init_mqtt():
    """Initialize the MQTT client"""
    global mqtt_client
    
    print(f"Initializing MQTT client:")
    print(f"  Broker: {MQTT_BROKER}")
    print(f"  Port: {MQTT_PORT}")
    print(f"  Username: {MQTT_USERNAME}")
    print(f"  Feed: {MQTT_FEED}")
    
    # Create client with random client ID to avoid conflicts
    client_id = f"smart_plug_relay_{random.randint(1000, 9999)}"
    mqtt_client = mqtt.Client(client_id=client_id)
    
    # Set callbacks
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.on_disconnect = on_disconnect
    
    # Set authentication
    mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    # Enable SSL/TLS
    mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
    
    # Set automatic reconnect options
    mqtt_client.reconnect_delay_set(min_delay=1, max_delay=120)
    
    # Connect to broker
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        return True
    except Exception as e:
        print(f"ERROR connecting to MQTT broker: {str(e)}")
        import traceback
        traceback.print_exc()
        return False

def start_mqtt_loop():
    """Start the MQTT network loop in a separate thread"""
    mqtt_client.loop_start()
    print("MQTT client loop started")

# ===== HTTP FUNCTIONS =====

# Add this class to create a simple HTTP server
class SimpleHTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        
        status = {
            "status": "healthy",
            "mqtt_connected": mqtt_client and mqtt_client.is_connected(),
            "uptime": time.time() - start_time if 'start_time' in globals() else 0,
            "service": "mqtt_relay"
        }
        
        self.wfile.write(json.dumps(status).encode())
    
    def log_message(self, format, *args):
        # Suppress log messages to avoid cluttering the console
        return

# Add this function to start the HTTP server
def start_http_server(port=int(os.environ.get('PORT', 10000))):
    """Start a simple HTTP server for health checks"""
    server = HTTPServer(('0.0.0.0', port), SimpleHTTPHandler)
    print(f"Starting HTTP server on port {port}")
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    return server

# ===== MAIN FUNCTION =====

def main():
    """Main function"""
    global start_time, tuya_cloud
    start_time = time.time()
    last_tuya_refresh = time.time()
    TUYA_REFRESH_INTERVAL = 20 * 3600 # Refresh every 6 hours (adjust as needed)

    print("Starting Smart Plug Controller")
    
    # Initialize Tuya connection
    init_tuya()
    
    # Initialize and start MQTT client
    if init_mqtt():
        start_mqtt_loop()
    else:
        print("ERROR: Failed to initialize MQTT client. Exiting.")
        return 1
    
    # Start HTTP server for Render health checks
    port = int(os.environ.get('PORT', 10000))
    http_server = start_http_server(port)
    print(f"HTTP server running on port {port}")
    
    # Keep the main thread running
    try:
        print("Smart Plug Controller running. Press Ctrl+C to exit.")
        
        # Main loop - keep the program running and perform periodic tasks
        while True:
            current_time = time.time()

            # Periodically refresh Tuya connection
            if current_time - last_tuya_refresh > TUYA_REFRESH_INTERVAL:
                print(f"INFO: Refreshing Tuya Cloud connection (Interval: {TUYA_REFRESH_INTERVAL}s)")
                # ... rest of the refresh logic ...

            # Publish a periodic heartbeat/status message
            if mqtt_client and mqtt_client.is_connected():
                mqtt_client.publish(MQTT_FEED, json.dumps({
                    "status": "heartbeat",
                    "timestamp": time.time(),
                    "uptime": time.time() - start_time
                }))
            
            # Sleep for a while
            time.sleep(60)  # Heartbeat every minute
            
    except KeyboardInterrupt:
        print("Keyboard interrupt received. Shutting down...")
    except Exception as e:
        print(f"ERROR in main loop: {str(e)}")
        import traceback
        traceback.print_exc()
    finally:
        # Clean up
        if mqtt_client:
            mqtt_client.publish(MQTT_FEED, json.dumps({
                "status": "disconnecting",
                "timestamp": time.time()
            }))
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
            print("MQTT client stopped and disconnected")
        
        # Stop HTTP server
        http_server.shutdown()
        print("HTTP server stopped")
    
    print("Smart Plug Controller stopped")
    return 0

# ===== PROGRAM ENTRY POINT =====

if __name__ == "__main__":
    sys.exit(main())