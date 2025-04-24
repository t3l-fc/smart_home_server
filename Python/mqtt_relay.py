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
import traceback
import logging

# --- Basic Logging Configuration ---
# Configure logging to output to stdout, which Render should capture
logging.basicConfig(
    level=logging.INFO, # Set the default logging level (e.g., INFO, DEBUG)
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S',
    stream=sys.stdout # Explicitly direct logs to standard output
)
# -----------------------------------

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
TUYA_DEVICE_ID = "eb06105cff43c50594rz5n"

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
mqtt_connected = False
start_time = None

# ===== TUYA FUNCTIONS =====

def init_tuya():
    """Initialize Tuya Cloud API connection."""
    global tuya_cloud
    logging.info("Attempting to initialize Tuya Cloud connection...")
    try:
        tuya_cloud = tinytuya.Cloud(
            apiRegion=TUYA_API_REGION,
            apiKey=TUYA_API_KEY,
            apiSecret=TUYA_API_SECRET,
            apiDeviceID=TUYA_DEVICE_ID)
        logging.info("Tuya Cloud connection initialized successfully.")
        return True # Indicate success
    except Exception as e:
        # logging.exception automatically includes traceback info
        logging.exception("ERROR: Failed to initialize Tuya Cloud connection")
        tuya_cloud = None # Ensure it's None if failed
        return False # Indicate failure

def control_device(device_id, command):
    """Sends command to Tuya device via Cloud API."""
    if not tuya_cloud:
        logging.error("Tuya Cloud API not initialized. Cannot control device %s.", device_id)
        return False

    logging.info("Attempting to send command '%s' to device '%s'...", command, device_id)
    try:
        cmd = {
            'commands': [{
                'code': 'switch_1',
                'value': command.lower() == 'on'
            }]
        }
        logging.debug("Sending command payload: %s", cmd) # Use DEBUG for detailed payload
        result = tuya_cloud.sendcommand(device_id, cmd)
        logging.info("Tuya API response for device %s: %s", device_id, result)

        if result and result.get('success', False):
             logging.info("SUCCESS: Command '%s' sent to device '%s' successfully.", command, device_id)
             return True
        else:
             logging.warning("Tuya API reported failure or unexpected response for device %s: %s", device_id, result)
             return False

    except Exception as e:
        logging.exception("ERROR: Failed to send command '%s' to device '%s'", command, device_id)
        return False

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
        logging.info(f"Connected to MQTT broker {MQTT_BROKER} successfully")
        # Subscribe to the feed
        client.subscribe(MQTT_FEED)
        logging.info(f"Subscribed to {MQTT_FEED}")
        
        # Publish a connection message
        client.publish(MQTT_FEED, json.dumps({
            "status": "connected",
            "server": "smart_plug_relay",
            "devices": list(DEVICES.keys()),
            "timestamp": time.time()
        }))
    else:
        logging.error(f"Failed to connect to MQTT broker, return code: {rc}")

def on_message(client, userdata, msg):
    """Callback when a message is received"""
    try:
        payload = msg.payload.decode()
        logging.info(f"Received message: {payload}")
        
        # Check if this is a JSON message (likely a response, not a command)
        if payload.startswith('{') and payload.endswith('}'):
            logging.info("Ignoring JSON message (probably a response, not a command)")
            return
        
        # Parse command (device:action)
        if ":" in payload:
            device_id, action = payload.split(":", 1)
            logging.info(f"Parsed command: device={device_id}, action={action}")
            
            # Special case for "all" device
            if device_id.lower() == "all":
                logging.info(f"Controlling ALL devices: {action}")
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
                logging.error(f"ERROR: Unknown device: {device_id}")
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
            logging.error(f"ERROR: Invalid message format: {payload}")
            client.publish(MQTT_FEED, json.dumps({
                "error": "Invalid message format",
                "expected": "device:action",
                "examples": ["cactus:on", "ananas:off", "all:status"],
                "timestamp": time.time()
            }))
            
    except Exception as e:
        logging.exception(f"ERROR processing message: {str(e)}")
        
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
        logging.warning(f"Unexpected disconnection from MQTT broker, code: {rc}")
        logging.info("Will automatically try to reconnect...")
    else:
        logging.info("Disconnected from MQTT broker")

def init_mqtt():
    """Initialize the MQTT client"""
    global mqtt_client
    
    logging.info(f"Initializing MQTT client:")
    logging.info(f"  Broker: {MQTT_BROKER}")
    logging.info(f"  Port: {MQTT_PORT}")
    logging.info(f"  Username: {MQTT_USERNAME}")
    logging.info(f"  Feed: {MQTT_FEED}")
    
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
        logging.exception(f"ERROR connecting to MQTT broker: {str(e)}")
        return False

def start_mqtt_loop():
    """Start the MQTT network loop in a separate thread"""
    mqtt_client.loop_start()
    logging.info("MQTT client loop started")

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
    logging.info(f"Starting HTTP server on port {port}")
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

    logging.info("-----------------------------------------")
    logging.info("Starting Smart Plug Controller")
    logging.info(f"Tuya Refresh Interval: {TUYA_REFRESH_INTERVAL}s")
    logging.info("-----------------------------------------")
    
    # Initialize Tuya connection
    init_tuya()
    
    # Initialize and start MQTT client
    if init_mqtt():
        start_mqtt_loop()
    else:
        logging.error("ERROR: Failed to initialize MQTT client. Exiting.")
        return 1
    
    # Start HTTP server for Render health checks
    port = int(os.environ.get('PORT', 10000))
    http_server = start_http_server(port)
    logging.info(f"HTTP server running on port {port}")
    
    # Keep the main thread running
    try:
        logging.info("Smart Plug Controller running. Press Ctrl+C to exit.")
        
        # Main loop - keep the program running and perform periodic tasks
        while True:
            current_time = time.time()

            # Periodically refresh Tuya connection
            if current_time - last_tuya_refresh > TUYA_REFRESH_INTERVAL:
                logging.info(f"INFO: Refreshing Tuya Cloud connection (Interval: {TUYA_REFRESH_INTERVAL}s)")
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
        logging.info("Keyboard interrupt received. Shutting down...")
    except Exception as e:
        logging.exception("ERROR in main loop: {str(e)}")
    finally:
        # Clean up
        if mqtt_client:
            mqtt_client.publish(MQTT_FEED, json.dumps({
                "status": "disconnecting",
                "timestamp": time.time()
            }))
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
            logging.info("MQTT client stopped and disconnected")
        
        # Stop HTTP server
        http_server.shutdown()
        logging.info("HTTP server stopped")
    
    logging.info("Smart Plug Controller stopped")
    return 0

# ===== PROGRAM ENTRY POINT =====

if __name__ == "__main__":
    sys.exit(main())