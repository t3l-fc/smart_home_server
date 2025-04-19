#!/usr/bin/env python3
"""
Test script for the MQTT-based Smart Plug Relay Server
"""

import argparse
import json
import time
import paho.mqtt.client as mqtt
import ssl
import sys
import os

# Adafruit IO Configuration
AIO_SERVER = "io.adafruit.com"
AIO_SERVERPORT = 8883
AIO_USERNAME = "marsouino"
AIO_KEY = "2e4dabd28afe424085715d39cb85311a"
AIO_FEED = "marsouino/feeds/smart_plugs"

# Adafruit IO Root CA Certificate
ADAFRUIT_IO_ROOT_CA = """-----BEGIN CERTIFICATE-----
MIIEjTCCA3WgAwIBAgIQDQd4KhM/xvmlcpbhMf/ReTANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xNzExMDIxMjIzMzdaFw0yNzExMDIxMjIzMzdaMGAxCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xHzAdBgNVBAMTFkdlb1RydXN0IFRMUyBSU0EgQ0EgRzEwggEiMA0GCSqGSIb3
DQEBAQUAA4IBDwAwggEKAoIBAQC+F+jsvikKy/65LWEx/TMkCDIuWegh1Ngwvm4Q
yISgP7oU5d79eoySG3vOhC3w/3jEMuipoH1fBtp7m0tTpsYbAhch4XA7rfuD6whU
gajeErLVxoiWMPkC/DnUvbgi74BJmdBiuGHQSd7LwsuXpTEGG9fYXcbTVN5SATYq
DfbexbYxTMwVJWoVb6lrBEgM3gBBqiiAiy800xu1Nq07JdCIQkBsNpFtZbIZhsDS
fzlGWP4wEmBQ3O67c+ZXkFr2DcrXBEtHam80Gp2SNhou2U5U7UesDL/xgLK6/0d7
6TnEVMSUVJkZ8VeZr+IUIlvoLrtjLbqugb0T3OYXW+CQU0kBAgMBAAGjggFAMIIB
PDAdBgNVHQ4EFgQUlE/UXYvkpOKmgP792PkA76O+AlcwHwYDVR0jBBgwFoAUTiJU
IBiV5uNu5g/6+rkS7QYXjzkwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsG
AQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/AgEAMDQGCCsGAQUFBwEB
BCgwJjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEIGA1Ud
HwQ7MDkwN6A1oDOGMWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEds
b2JhbFJvb3RHMi5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEW
HGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwDQYJKoZIhvcNAQELBQADggEB
AIIcBDqC6cWpyGUSXAjjAcYwsK4iiGF7KweG97i1RJz1kwZhRoo6orU1JtBYnjzB
c4+/sXmnHJk3mlPyL1xuIAt9sMeC7+vreRIF5wFBC0MCN5sbHwhNN1JzKbifNeP5
ozpZdQFmkCo+neBiKR6HqIA+LMTMCMMuv2khGGuPHmtDze4GmEGZtYLyF8EQpa5Y
jPuV6k2Cr/N3XxFpT3hRpt/3usU/Zb9wfKPtWpoznZ4/44c1p9rzFcZYrWkj3A+7
TNBJE0GmP2fhXhP1D/XVfIW/h0yCJGEiV9Glm/uGOa3DXHlmbAcxSyCRraG+ZBkA
7h4SeM6Y8l/7MBRpPCz6l8Y=
-----END CERTIFICATE-----"""

# Available devices
DEVICES = ['cactus', 'ananas', 'dino', 'vinyle', 'all']

def test_server(action=None, device=None):
    """Test the MQTT server by sending commands and checking responses"""
    
    # Store response data to reference later
    response_data = {
        'connected': False,
        'status_message': None
    }
    
    # Create certificate file for SSL
    import tempfile
    import os
    
    ca_cert_file = tempfile.NamedTemporaryFile(delete=False)
    ca_cert_file.write(ADAFRUIT_IO_ROOT_CA.encode())
    ca_cert_file.close()
    
    # Define callback functions
    def on_connect(client, userdata, flags, rc):
        print(f"Connected to Adafruit IO with result code {rc}")
        
        # Set flag to indicate we're connected
        response_data['connected'] = True
        
        # Subscribe to get status updates back
        client.subscribe(AIO_FEED)
        
        # If a command was specified, send it once connected
        if action and device:
            send_command(client, action, device)
    
    def on_message(client, userdata, msg):
        payload = msg.payload.decode('utf-8')
        print(f"Received message: {payload}")
        response_data['status_message'] = payload
    
    def on_disconnect(client, userdata, rc):
        print(f"Disconnected with result code {rc}")
    
    def send_command(client, action, device):
        """Send a command to the device"""
        if action not in ["on", "off", "status"]:
            print(f"Invalid action: {action}. Must be 'on', 'off', or 'status'")
            return False
            
        if device not in DEVICES:
            print(f"Invalid device: {device}. Must be one of {DEVICES}")
            return False
            
        # Format: device_id:action
        message = f"{device}:{action}"
        
        print(f"\nSending {action.upper()} command to device: {device}")
        result = client.publish(AIO_FEED, message)
        
        # Check if message was queued successfully
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print("Command sent successfully")
        else:
            print(f"Failed to send command, return code: {result.rc}")
        
        return True
    
    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    
    # Set credentials
    client.username_pw_set(AIO_USERNAME, AIO_KEY)
    
    # Enable SSL/TLS
    client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
    
    # Connect to Adafruit IO broker
    try:
        print(f"Connecting to Adafruit IO ({AIO_SERVER})...")
        client.connect(AIO_SERVER, AIO_SERVERPORT, 60)
    except Exception as e:
        print(f"Error: Could not connect to Adafruit IO broker: {e}")
        os.unlink(ca_cert_file.name)
        return 1
    
    # Start the network loop in a non-blocking way
    client.loop_start()
    
    try:
        # Wait for connection
        timeout = 10  # seconds
        start_time = time.time()
        
        while not response_data['connected'] and time.time() - start_time < timeout:
            time.sleep(0.1)
            
        if not response_data['connected']:
            print("Error: Connection timeout")
            client.disconnect()
            client.loop_stop()
            os.unlink(ca_cert_file.name)
            return 1
            
        # If no action specified, just get the status
        if not action:
            print("\nGetting status of all devices...")
            send_command(client, "status", "all")
        
        # Wait for a response or timeout
        timeout = 10  # seconds
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            if response_data['status_message']:
                # If we received a response, break
                break
            time.sleep(0.5)
            
        # If we didn't receive a response, but we're asking for status, let's try again
        if not response_data['status_message'] and (not action or action == 'status'):
            print("No response received, trying again...")
            send_command(client, "status", device or "all")
            
            # Wait for response again
            timeout = 5  # seconds
            start_time = time.time()
            
            while time.time() - start_time < timeout:
                if response_data['status_message']:
                    # If we received a response, break
                    break
                time.sleep(0.5)
        
        # Print result
        if response_data['status_message']:
            try:
                # If it's JSON, format it nicely
                parsed_data = json.loads(response_data['status_message'])
                print("\nResponse:")
                print(json.dumps(parsed_data, indent=2))
            except json.JSONDecodeError:
                # Otherwise just print as text
                print(f"\nResponse: {response_data['status_message']}")
        else:
            print("No response received within timeout period")
        
    except KeyboardInterrupt:
        print("Operation canceled by user")
    finally:
        # Clean up
        client.disconnect()
        client.loop_stop()
        os.unlink(ca_cert_file.name)
    
    return 0

def main():
    parser = argparse.ArgumentParser(description='Test the MQTT Smart Plug Relay Server')
    parser.add_argument('--action', choices=['status', 'on', 'off'], 
                      help='Action to perform')
    parser.add_argument('--device', choices=DEVICES, 
                      help='Device ID or name (use "all" for all devices)')
    args = parser.parse_args()
    
    return test_server(args.action, args.device)

if __name__ == "__main__":
    sys.exit(main()) 