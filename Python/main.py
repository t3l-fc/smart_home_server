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

# Adafruit IO Configuration
AIO_SERVER = "io.adafruit.com"
AIO_SERVERPORT = 8883
AIO_USERNAME = "marsouino"
AIO_KEY = "2e4dabd28afe424085715d39cb85311a"
AIO_FEED = "marsouino/feeds/smart_plugs"

def main():
    """Main entry point for the Smart Plug Controller"""
    print("Starting Smart Plug Controller Relay Server...")
    
    # Get port from environment variable or use default
    port = int(os.environ.get('PORT', 5000))
    
    # Check if running in debug mode
    debug_mode = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
    
    # Start MQTT client with Adafruit IO credentials
    mqtt_success = start_mqtt_client(
        server=AIO_SERVER,
        port=AIO_SERVERPORT,
        username=AIO_USERNAME,
        key=AIO_KEY,
        feed=AIO_FEED
    )
    
    if not mqtt_success:
        print("Warning: Failed to start MQTT client. Will continue with just the web server.")
    
    # Start the Flask app
    app.run(host='0.0.0.0', port=port, debug=debug_mode)

if __name__ == "__main__":
    sys.exit(main())
