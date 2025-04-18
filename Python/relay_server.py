import os
import json
import time
from flask import Flask, request, jsonify
import tinytuya
from dotenv import load_dotenv

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
        if action == "on":
            result = device.sendcommand(device_id, {"commands": [{"code": "switch_1", "value": True}]})
            return {"status": "success", "action": "on", "result": result}
        elif action == "off":
            result = device.sendcommand(device_id, {"commands": [{"code": "switch_1", "value": False}]})
            return {"status": "success", "action": "off", "result": result}
        elif action == "status":
            result = device.getstatus(device_id)
            return {"status": "success", "action": "status", "result": result}
        else:
            return {"status": "error", "message": "Invalid action"}
    except Exception as e:
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
    return "Tuya Smart Plug Relay Server is running!"

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
    return jsonify({"status": "healthy"}), 200

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port) 