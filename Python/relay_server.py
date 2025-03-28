import os
import json
from flask import Flask, request, jsonify
import tinytuya
import paho.mqtt.client as mqtt
from dotenv import load_dotenv

# Load environment variables or use defaults
load_dotenv()

# Tuya device configuration
DEVICE_IP = os.environ.get('DEVICE_IP', "192.168.86.249")
DEVICE_ID = os.environ.get('DEVICE_ID', "eb06105cff43c50594rz5n")
ACCESS_ID = os.environ.get('ACCESS_ID', "4kcffc9h34rwnswpncrj") 
ACCESS_KEY = os.environ.get('ACCESS_KEY', "d1dc602a4d684f8895e2fca36d8996d8")
REGION = os.environ.get('REGION', "us")

# MQTT Configuration
MQTT_BROKER = os.environ.get('MQTT_BROKER', "")
MQTT_PORT = int(os.environ.get('MQTT_PORT', 1883))
MQTT_USERNAME = os.environ.get('MQTT_USERNAME', "")
MQTT_PASSWORD = os.environ.get('MQTT_PASSWORD', "")
MQTT_TOPIC_COMMAND = os.environ.get('MQTT_TOPIC_COMMAND', "tuya/command")
MQTT_TOPIC_STATUS = os.environ.get('MQTT_TOPIC_STATUS', "tuya/status")

# Initialize Flask app
app = Flask(__name__)

# Initialize TinyTuya device
device = tinytuya.Cloud(
    apiRegion=REGION,
    apiKey=ACCESS_ID,
    apiSecret=ACCESS_KEY,
    apiDeviceID=DEVICE_ID
)

# Function to control the device
def control_device(action):
    try:
        if action == "on":
            # Turn on the smart plug using the command structure from your working code
            command = {"commands": [{"code": "switch_1", "value": True}]}
            result = device.sendcommand(DEVICE_ID, command)
            return {"status": "success", "action": "on", "result": result}
        elif action == "off":
            # Turn off the smart plug using the command structure from your working code
            command = {"commands": [{"code": "switch_1", "value": False}]}
            result = device.sendcommand(DEVICE_ID, command)
            return {"status": "success", "action": "off", "result": result}
        elif action == "status":
            # Get device status using the method from your working code
            result = device.getstatus(DEVICE_ID)
            return {"status": "success", "action": "status", "result": result}
        else:
            return {"status": "error", "message": "Invalid action"}
    except Exception as e:
        return {"status": "error", "message": str(e)}

# MQTT client setup
if MQTT_BROKER:
    mqtt_client = mqtt.Client()
    
    if MQTT_USERNAME and MQTT_PASSWORD:
        mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    # MQTT callbacks
    def on_connect(client, userdata, flags, rc):
        print(f"Connected with result code {rc}")
        client.subscribe(MQTT_TOPIC_COMMAND)
    
    def on_message(client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            action = payload.get("action")
            if action:
                result = control_device(action)
                # Publish the result to the status topic
                client.publish(MQTT_TOPIC_STATUS, json.dumps(result))
        except Exception as e:
            print(f"Error processing MQTT message: {e}")
    
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    # Connect to MQTT broker if configured
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        print(f"Connected to MQTT broker at {MQTT_BROKER}")
    except Exception as e:
        print(f"Error connecting to MQTT broker: {e}")

# API routes
@app.route('/')
def index():
    return "Tuya Smart Plug Relay Server is running!"

@app.route('/api/control', methods=['POST'])
def api_control():
    data = request.json
    action = data.get('action')
    if not action:
        return jsonify({"status": "error", "message": "No action specified"}), 400
    
    result = control_device(action)
    return jsonify(result)

@app.route('/api/status', methods=['GET'])
def api_status():
    result = control_device("status")
    return jsonify(result)

# Health check endpoint for monitoring
@app.route('/health', methods=['GET'])
def health_check():
    return jsonify({"status": "healthy"}), 200

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port) 