import os
import json
import time
import ssl
from flask import Flask, request, jsonify
import tinytuya
from dotenv import load_dotenv
import paho.mqtt.client as mqtt
from paho.mqtt import publish

# Version marker to confirm deployment
print("Loading relay_server.py - Version 2.0 (Adafruit IO)")

# Load environment variables or use defaults
load_dotenv()

# Tuya device configurations
DEVICES = {
    'cactus': {
        'id': os.environ.get('DEVICE1_ID', "eb06105cff43c50594rz5n"),
        'ip': os.environ.get('DEVICE1_IP', "192.168.86.249"),
        'name': "Cactus"
    },
    'ananas': {
        'id': os.environ.get('DEVICE2_ID', "eb39191340ad46ad91wbug"),
        'ip': os.environ.get('DEVICE2_IP', "192.168.86.25"),
        'name': "Ananas"
    },
    'dino': {
        'id': os.environ.get('DEVICE3_ID', "03310047840d8e87510e"),
        'ip': os.environ.get('DEVICE3_IP', "192.168.86.20"),
        'name': "Dino"
    },
    'vinyle': {
        'id': os.environ.get('DEVICE4_ID', "eb6eef417508566dbf5mfh"),
        'ip': os.environ.get('DEVICE4_IP', ""),
        'name': "Vinyle"
    }
}

# Common Tuya cloud configuration
ACCESS_ID = os.environ.get('ACCESS_ID', "4kcffc9h34rwnswpncrj")
ACCESS_KEY = os.environ.get('ACCESS_KEY', "d1dc602a4d684f8895e2fca36d8996d8")
REGION = os.environ.get('REGION', "us")

# MQTT Configuration - Default values, will be overridden by values from main.py
MQTT_BROKER = "io.adafruit.com"
MQTT_PORT = 8883
MQTT_USER = "marsouino"
MQTT_PASSWORD = "2e4dabd28afe424085715d39cb85311a"
MQTT_CLIENT_ID = "smart_plug_relay"
MQTT_FEED = "marsouino/feeds/smart_plugs"

# Topics
BASE_TOPIC = "home/plugs"
COMMAND_TOPIC = f"{BASE_TOPIC}/+/set"  # + is a wildcard for device names
STATUS_TOPIC = f"{BASE_TOPIC}/status"

# Initialize Flask app
app = Flask(__name__)

# Initialize TinyTuya cloud connection
device = tinytuya.Cloud(
    apiRegion=REGION,
    apiKey=ACCESS_ID,
    apiSecret=ACCESS_KEY,
    apiDeviceID=DEVICES['cactus']['id']  # Default device ID
)

def control_single_device(device_id, action):
    """Control a single device"""
    try:
        print(f"Calling Tuya API to {action} device {device_id}")
        if action == "on":
            print(f"Sending 'switch_1: True' command to device {device_id}")
            result = device.sendcommand(device_id, {"commands": [{"code": "switch_1", "value": True}]})
            print(f"Tuya API response: {result}")
            return {"status": "success", "action": "on", "result": result}
        elif action == "off":
            print(f"Sending 'switch_1: False' command to device {device_id}")
            result = device.sendcommand(device_id, {"commands": [{"code": "switch_1", "value": False}]})
            print(f"Tuya API response: {result}")
            return {"status": "success", "action": "off", "result": result}
        elif action == "status":
            print(f"Getting status for device {device_id}")
            result = device.getstatus(device_id)
            print(f"Tuya API status response: {result}")
            return {"status": "success", "action": "status", "result": result}
        else:
            print(f"ERROR: Invalid action {action}")
            return {"status": "error", "message": "Invalid action"}
    except Exception as e:
        print(f"ERROR in control_single_device: {str(e)}")
        import traceback
        traceback.print_exc()
        return {"status": "error", "message": str(e)}

def control_all_devices(action):
    """Control all devices simultaneously"""
    results = {}
    for device_key, device_info in DEVICES.items():
        if device_info['id']:  # Only control devices with valid IDs
            result = control_single_device(device_info['id'], action)
            results[device_key] = result
    return results

# API routes
@app.route('/')
def index():
    return "Tuya Smart Plug Relay Server is running! Using Adafruit IO MQTT."

@app.route('/api/control/<device_id>', methods=['POST'])
def api_control_single(device_id):
    """Control a specific device"""
    data = request.json
    action = data.get('action')
    
    if not action:
        return jsonify({"status": "error", "message": "No action specified"}), 400
    
    # Check if device exists
    device_found = False
    device_actual_id = None
    for device_info in DEVICES.values():
        if device_info['id'] == device_id or device_info.get('name', '').lower() == device_id.lower():
            device_found = True
            device_actual_id = device_info['id']
            break
    
    if not device_found:
        return jsonify({"status": "error", "message": "Device not found"}), 404
    
    result = control_single_device(device_actual_id, action)
    return jsonify(result)

@app.route('/api/control/all', methods=['POST'])
def api_control_all():
    """Control all devices"""
    data = request.json
    action = data.get('action')
    
    if not action:
        return jsonify({"status": "error", "message": "No action specified"}), 400
    
    results = control_all_devices(action)
    return jsonify({"status": "success", "results": results})

@app.route('/api/status/<device_id>', methods=['GET'])
def api_status_single(device_id):
    """Get status of a specific device"""
    # Check if device exists
    device_found = False
    device_actual_id = None
    for device_info in DEVICES.values():
        if device_info['id'] == device_id or device_info.get('name', '').lower() == device_id.lower():
            device_found = True
            device_actual_id = device_info['id']
            break
    
    if not device_found:
        return jsonify({"status": "error", "message": "Device not found"}), 404
    
    result = control_single_device(device_actual_id, "status")
    return jsonify(result)

@app.route('/api/status/all', methods=['GET'])
def api_status_all():
    """Get status of all devices"""
    results = {}
    for device_key, device_info in DEVICES.items():
        if device_info['id']:  # Only check devices with valid IDs
            result = control_single_device(device_info['id'], "status")
            results[device_key] = result
    return jsonify({"status": "success", "results": results})

@app.route('/api/devices', methods=['GET'])
def api_list_devices():
    """List all available devices"""
    device_list = {}
    for key, info in DEVICES.items():
        if info['id']:  # Only include devices with valid IDs
            device_list[key] = {
                'name': info['name'],
                'id': info['id']
            }
    return jsonify({"status": "success", "devices": device_list})

# Health check endpoint
@app.route('/health', methods=['GET'])
def health_check():
    return jsonify({"status": "healthy", "mqtt_broker": MQTT_BROKER, "mqtt_feed": MQTT_FEED}), 200

# Global variable for MQTT client
mqtt_client = None

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker {userdata['broker']} with result code {rc}")
    # Subscribe to the feed
    client.subscribe(userdata["feed"])
    print(f"Subscribed to {userdata['feed']}")
    
    # Publish an initial status message
    client.publish(userdata["feed"], json.dumps({
        "status": "connected",
        "server": "smart_plug_relay",
        "devices": list(DEVICES.keys())
    }))

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
                client.publish(userdata["feed"], json.dumps({
                    "error": f"Unknown device: {device_id}",
                    "available_devices": list(DEVICES.keys())
                }))
                return
            
            print(f"Found device ID: {device_tuya_id} for {device_id}")
                
            # Process command
            if action.lower() in ["on", "true", "1"]:
                print(f"Attempting to turn ON {device_id}...")
                result = control_single_device(device_tuya_id, "on")
                print(f"Result of turning ON {device_id}: {result}")
                client.publish(userdata["feed"], json.dumps({
                    "device": device_id, 
                    "status": "on", 
                    "result": result
                }))
            elif action.lower() in ["off", "false", "0"]:
                print(f"Attempting to turn OFF {device_id}...")
                result = control_single_device(device_tuya_id, "off")
                print(f"Result of turning OFF {device_id}: {result}")
                client.publish(userdata["feed"], json.dumps({
                    "device": device_id, 
                    "status": "off", 
                    "result": result
                }))
            elif action.lower() == "status":
                print(f"Checking status of {device_id}...")
                result = control_single_device(device_tuya_id, "status")
                print(f"Status of {device_id}: {result}")
                client.publish(userdata["feed"], json.dumps({
                    "device": device_id, 
                    "status": "status", 
                    "result": result
                }))
            else:
                print(f"ERROR: Invalid command: {action}")
                client.publish(userdata["feed"], json.dumps({
                    "error": f"Invalid command: {action}",
                    "valid_commands": ["on", "off", "status"]
                }))
                return
        # Special case for "all" command
        elif payload == "all:on":
            print("Turning ON all devices...")
            result = control_all_devices("on")
            client.publish(userdata["feed"], json.dumps({
                "device": "all", 
                "status": "on", 
                "result": result
            }))
        elif payload == "all:off":
            print("Turning OFF all devices...")
            result = control_all_devices("off")
            client.publish(userdata["feed"], json.dumps({
                "device": "all", 
                "status": "off", 
                "result": result
            }))
        elif payload == "all:status":
            print("Checking status of all devices...")
            result = control_all_devices("status")
            client.publish(userdata["feed"], json.dumps({
                "device": "all", 
                "status": "status", 
                "result": result
            }))
        else:
            print(f"ERROR: Invalid message format: {payload} (expected device:action)")
            client.publish(userdata["feed"], json.dumps({
                "error": f"Invalid message format: {payload}",
                "expected_format": "device:action",
                "examples": ["cactus:on", "ananas:off", "dino:status", "all:on"]
            }))
            
    except Exception as e:
        print(f"ERROR processing message: {str(e)}")
        import traceback
        traceback.print_exc()
        
        # Try to publish error back to feed
        try:
            client.publish(userdata["feed"], json.dumps({
                "error": f"Exception in message handler: {str(e)}"
            }))
        except:
            pass

def on_disconnect(client, userdata, rc):
    print(f"Disconnected from MQTT broker with result code {rc}")
    if rc != 0:
        print("Unexpected disconnection. Trying to reconnect...")
        # Try to reconnect (client will handle reconnection automatically)

def start_mqtt_client(server, port, username, key, feed):
    """Initialize and start MQTT client with Adafruit IO credentials"""
    global mqtt_client, MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_FEED
    
    # Update global variables with the provided credentials
    MQTT_BROKER = server
    MQTT_PORT = port
    MQTT_USER = username
    MQTT_PASSWORD = key
    MQTT_FEED = feed
    
    print(f"Starting MQTT client with Adafruit IO credentials:")
    print(f"  Server: {server}")
    print(f"  Port: {port}")
    print(f"  Username: {username}")
    print(f"  Feed: {feed}")
    
    # Store settings for callbacks
    userdata = {
        "broker": server,
        "feed": feed
    }
    
    try:
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
        mqtt_client.connect(server, port, 60)
        
        # Start network loop
        mqtt_client.loop_start()
        
        print(f"MQTT client started successfully and connected to {server}")
        return True
    except Exception as e:
        print(f"Failed to connect to MQTT broker: {e}")
        import traceback
        traceback.print_exc()
        return False

# No standalone main function here - it will be called from main.py
# We now use the start_mqtt_client function to initialize MQTT

if __name__ == '__main__':
    # This block only runs if relay_server.py is executed directly
    print("relay_server.py should be imported by main.py, not run directly")
    port = int(os.environ.get('PORT', 5000))
    
    # Start MQTT client with default values
    start_mqtt_client(MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_FEED)
    
    # Start Flask app
    app.run(host='0.0.0.0', port=port) 