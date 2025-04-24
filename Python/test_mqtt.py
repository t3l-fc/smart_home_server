#!/usr/bin/env python3
"""
Test script for MQTT Smart Plug Controller
"""

import paho.mqtt.client as mqtt
import ssl
import time
import json
import sys
import argparse
import os

# Adafruit IO Configuration
MQTT_BROKER = "io.adafruit.com"
MQTT_PORT = 8883
MQTT_USERNAME = "marsouino"
MQTT_PASSWORD = "2e4dabd28afe424085715d39cb85311a"
MQTT_FEED = "marsouino/feeds/smart_plugs"

# Available devices
DEVICES = ['cactus', 'ananas', 'dino', 'vinyle', 'all']

def test_mqtt_command(device, action, wait_time=5):
    """
    Send a command to the MQTT feed and wait for response
    
    Args:
        device: Device name to control
        action: Command to send (on, off, status)
        wait_time: How long to wait for a response in seconds
    
    Returns:
        True if successful, False otherwise
    """
    if device not in DEVICES:
        print(f"ERROR: Unknown device '{device}'. Must be one of: {', '.join(DEVICES)}")
        return False
        
    if action not in ['on', 'off', 'status']:
        print(f"ERROR: Unknown action '{action}'. Must be 'on', 'off', or 'status'")
        return False
    
    # Store responses
    responses = []
    
    # Callback when connected
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print(f"Connected to Adafruit IO MQTT")
            # Subscribe to get responses
            client.subscribe(MQTT_FEED)
            # Send command
            command = f"{device}:{action}"
            print(f"Sending command: {command}")
            client.publish(MQTT_FEED, command)
        else:
            print(f"Failed to connect to MQTT broker, return code: {rc}")
    
    # Callback when message received
    def on_message(client, userdata, msg):
        try:
            payload = msg.payload.decode()
            print(f"Received: {payload}")
            
            # Try to parse as JSON
            try:
                data = json.loads(payload)
                responses.append(data)
            except json.JSONDecodeError:
                # Store as string if not JSON
                responses.append(payload)
                
        except Exception as e:
            print(f"Error processing message: {e}")
    
    # Create MQTT client
    client = mqtt.Client(client_id=f"test_client_{int(time.time())}")
    client.on_connect = on_connect
    client.on_message = on_message
    
    # Set authentication
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    # Enable SSL/TLS
    client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
    
    # Connect to broker
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        
        # Wait for response
        print(f"Waiting up to {wait_time} seconds for response...")
        start_time = time.time()
        
        while time.time() - start_time < wait_time and len(responses) < 2:
            time.sleep(0.1)
        
        # First message is usually echo of our command
        if len(responses) > 1:
            # Try to find the relevant response for our device
            for response in responses[1:]:  # Skip the first response (echo)
                if isinstance(response, dict):
                    # Check if this response is for our device
                    if 'device' in response and response['device'] == device:
                        print("\nResponse from server:")
                        print(json.dumps(response, indent=2))
                        return True
            
            # If we didn't find a specific match, show the last response
            print("\nLast response from server:")
            print(json.dumps(responses[-1], indent=2) if isinstance(responses[-1], dict) else responses[-1])
            return True
        else:
            print("\nNo response received from server within timeout period")
            return False
            
    except Exception as e:
        print(f"Error: {e}")
        return False
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected from MQTT broker")
        
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Test MQTT Smart Plug Controller')
    parser.add_argument('device', choices=DEVICES,
                      help='Device to control')
    parser.add_argument('action', choices=['on', 'off', 'status'],
                      help='Action to perform')
    parser.add_argument('--wait', type=int, default=5,
                      help='Seconds to wait for response (default: 5)')
    
    args = parser.parse_args()
    
    print(f"Testing MQTT command: {args.device}:{args.action}")
    success = test_mqtt_command(args.device, args.action, args.wait)
    
    if success:
        print("\nTest completed successfully!")
        sys.exit(0)
    else:
        print("\nTest failed")
        sys.exit(1)

def main():
    """Main function"""
    global start_time, tuya_cloud # Make sure tuya_cloud is global if modifying it here
    start_time = time.time()
    last_tuya_refresh = time.time()
    TUYA_REFRESH_INTERVAL = 6 * 3600 # Refresh every 6 hours (adjust as needed)

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
                try:
                    init_tuya() # Re-run the initialization
                    last_tuya_refresh = current_time
                    print("INFO: Tuya Cloud connection refreshed successfully.")
                except Exception as e:
                    print(f"ERROR: Failed to refresh Tuya connection: {e}")
                    # Optional: Decide if you want to retry sooner or handle differently

            # Publish a periodic heartbeat/status message
            if mqtt_client and mqtt_client.is_connected():
                mqtt_client.publish(MQTT_FEED, json.dumps({
                    "status": "heartbeat",
                    "timestamp": current_time,
                    "uptime": current_time - start_time
                }))

            # Sleep for a while
            time.sleep(60) # Check every minute

    except KeyboardInterrupt:
        print("Keyboard interrupt received. Shutting down...")
    # ... (rest of your finally block) ...