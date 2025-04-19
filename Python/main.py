#!/usr/bin/env python3
"""
Smart Plug Controller - Main Entry Point
"""

import os
import sys
import threading
import time
import ssl
import signal
from flask import Flask
from relay_server import app, start_mqtt_client
import json
import paho.mqtt.client as mqtt

# Adafruit IO Configuration
AIO_SERVER = "io.adafruit.com"
AIO_SERVERPORT = 8883
AIO_USERNAME = "marsouino"
AIO_KEY = "2e4dabd28afe424085715d39cb85311a"
AIO_FEED = "marsouino/feeds/smart_plugs"

# MQTT Configuration - These will be overridden by values from main.py
MQTT_BROKER = "io.adafruit.com"  # Default, will be overridden
MQTT_PORT = 8883
MQTT_USER = "marsouino"  # Default, will be overridden
MQTT_PASSWORD = "2e4dabd28afe424085715d39cb85311a"  # Default, will be overridden
MQTT_CLIENT_ID = "smart_plug_relay"
MQTT_FEED = "marsouino/feeds/smart_plugs"  # Default, will be overridden

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker {userdata['broker']} with result code {rc}")
    # Subscribe to the feed
    client.subscribe(userdata["feed"])
    print(f"Subscribed to {userdata['feed']}")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        print(f"MQTT RECEIVED: Topic={msg.topic}, Payload={payload}")
        
        # Parse command format (device:action)
        if ":" in payload:
            device_id, action = payload.split(":", 1)
            print(f"Parsed command: Device={device_id}, Action={action}")
            
            # Find the device in our DEVICES dictionary
            device_tuya_id = None
            for key, device_info in DEVICES.items():
                if key == device_id:
                    device_tuya_id = device_info['id']
                    break
            
            if not device_tuya_id:
                print(f"ERROR: Unknown device: {device_id}")
                return
            
            print(f"Found device ID: {device_tuya_id} for {device_id}")
                
            # Process command
            if action.lower() in ["on", "true", "1"]:
                print(f"Attempting to turn ON {device_id}...")
                result = control_single_device(device_tuya_id, "on")
                print(f"Result of turning ON {device_id}: {result}")
                client.publish(userdata["feed"], json.dumps({"device": device_id, "status": "on", "result": result}))
            elif action.lower() in ["off", "false", "0"]:
                print(f"Attempting to turn OFF {device_id}...")
                result = control_single_device(device_tuya_id, "off")
                print(f"Result of turning OFF {device_id}: {result}")
                client.publish(userdata["feed"], json.dumps({"device": device_id, "status": "off", "result": result}))
            elif action.lower() == "status":
                print(f"Checking status of {device_id}...")
                result = control_single_device(device_tuya_id, "status")
                print(f"Status of {device_id}: {result}")
                client.publish(userdata["feed"], json.dumps({"device": device_id, "status": "status", "result": result}))
            else:
                print(f"ERROR: Invalid command: {action}")
                return
        else:
            print(f"ERROR: Invalid message format: {payload} (expected device:action)")
            
    except Exception as e:
        print(f"ERROR processing message: {str(e)}")
        import traceback
        traceback.print_exc()

def start_mqtt_client(server, port, username, key, feed):
    """Initialize and start MQTT client with Adafruit IO credentials"""
    global mqtt_client
    
    print("Starting MQTT client...")
    print(f"Connecting to {server}:{port} as {username}")
    print(f"Feed: {feed}")
    
    # Store settings for callbacks
    userdata = {
        "broker": server,
        "feed": feed
    }
    
    # Create MQTT client
    mqtt_client = mqtt.Client(client_id="smart_plug_relay", userdata=userdata)
    
    # Set callbacks
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.on_disconnect = on_disconnect
    
    # Set authentication
    mqtt_client.username_pw_set(username, key)
    
    # Enable SSL/TLS
    mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
    
    # Connect to broker
    try:
        mqtt_client.connect(server, port, 60)
        # Start network loop
        mqtt_client.loop_start()
        print(f"MQTT client started successfully and connected to {server}")
        return True
    except Exception as e:
        print(f"Failed to connect to MQTT broker: {e}")
        return False

def main():
    """Main entry point for the Smart Plug Controller"""
    print("Starting Smart Plug Controller Relay Server...")
    
    # Get port from environment variable or use default
    port = int(os.environ.get('PORT', 5000))
    
    # Check if running in debug mode
    debug_mode = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
    
    # Start MQTT client with Adafruit IO credentials
    print(f"Initializing MQTT client with Adafruit IO credentials:")
    print(f"  Server: {AIO_SERVER}")
    print(f"  Port: {AIO_SERVERPORT}")
    print(f"  Username: {AIO_USERNAME}")
    print(f"  Feed: {AIO_FEED}")
    
    mqtt_success = start_mqtt_client(
        server=AIO_SERVER,
        port=AIO_SERVERPORT,
        username=AIO_USERNAME,
        key=AIO_KEY,
        feed=AIO_FEED
    )
    
    if not mqtt_success:
        print("WARNING: Failed to start MQTT client. Will continue with just the web server.")
    else:
        print("MQTT client started successfully and connected to Adafruit IO")
    
    # Start the Flask app
    app.run(host='0.0.0.0', port=port, debug=debug_mode)

if __name__ == "__main__":
    sys.exit(main())
