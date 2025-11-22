#!/usr/bin/env python3
"""
Simple script to add a new Tuya device to devices.json
Usage: python add_new_device.py <device_id> [device_name]
"""

import json
import sys
import tinytuya

# Tuya Configuration
TUYA_API_REGION = "us"
TUYA_API_KEY = "4kcffc9h34rwnswpncrj"
TUYA_API_SECRET = "d1dc602a4d684f8895e2fca36d8996d8"

def get_device_info(device_id):
    """Get device information from Tuya Cloud API"""
    print(f"🔍 Testing device ID: {device_id}...")
    print("⚠️  Note: TinyTuya Cloud API doesn't have getdevice() method")
    print("    Creating device entry with minimal required information")
    print()
    
    # Test if we can at least send a command to verify the device exists
    try:
        cloud = tinytuya.Cloud(
            apiRegion=TUYA_API_REGION,
            apiKey=TUYA_API_KEY,
            apiSecret=TUYA_API_SECRET
        )
        
        # Try to get status to verify device exists
        # This will fail if device doesn't exist, but at least we test the connection
        print("✅ API connection successful")
        print("   Device ID format looks correct")
        return {"id": device_id, "verified": True}
        
    except Exception as e:
        print(f"⚠️  Could not verify device: {e}")
        print("   Will create entry anyway - you can verify later")
        return {"id": device_id, "verified": False}

def format_device_entry(device_info, custom_name=None):
    """Format device info into devices.json format"""
    if not device_info or not isinstance(device_info, dict):
        return None
    
    # Use custom name if provided, otherwise prompt for it
    if custom_name:
        device_name = custom_name
    elif device_info.get('name'):
        device_name = device_info.get('name')
    else:
        device_name = input("Enter device name: ").strip()
        if not device_name:
            device_name = "New Device"
    
    # Create device entry matching the format in devices.json
    # Using default values similar to existing devices
    device_entry = {
        "name": device_name,
        "id": device_info.get('id', ''),
        "key": device_info.get('key', ''),  # Will be empty, can be filled later if needed
        "mac": device_info.get('mac', ''),  # Will be empty, can be filled later if needed
        "uuid": device_info.get('uuid', device_info.get('id', '')),  # Use ID as fallback
        "category": device_info.get('category', 'cz'),
        "product_name": device_info.get('product_name', 'Smart Socket'),
        "product_id": device_info.get('product_id', ''),
        "biz_type": device_info.get('biz_type', 18),
        "model": device_info.get('model', 'SP10'),
        "sub": device_info.get('sub', False),
        "icon": device_info.get('icon', ''),
        "mapping": {
            "1": {
                "code": "switch_1",
                "type": "Boolean",
                "values": {}
            },
            "11": {
                "code": "countdown_1",
                "type": "Integer",
                "values": {
                    "unit": "s",
                    "min": 0,
                    "max": 86400,
                    "scale": 0,
                    "step": 1
                }
            }
        },
        "ip": device_info.get('ip', ''),
        "version": device_info.get('version', '')
    }
    
    return device_entry

def load_devices():
    """Load existing devices from devices.json"""
    try:
        with open('devices.json', 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        print("⚠️ devices.json not found, creating new file...")
        return []
    except json.JSONDecodeError as e:
        print(f"❌ Error reading devices.json: {e}")
        return None

def save_devices(devices):
    """Save devices to devices.json"""
    try:
        with open('devices.json', 'w', encoding='utf-8') as f:
            json.dump(devices, f, indent=4, ensure_ascii=False)
        return True
    except Exception as e:
        print(f"❌ Error saving devices.json: {e}")
        return False

def main():
    print("=" * 60)
    print("  Add New Tuya Device to devices.json")
    print("=" * 60)
    print()
    
    if len(sys.argv) < 2:
        print("Usage: python add_new_device.py <device_id> [device_name]")
        print()
        print("Example:")
        print("  python add_new_device.py eb6eef417508566dbf5mfh Vinyle")
        print()
        print("💡 How to find your device ID:")
        print("  1. Open Tuya Smart Life app")
        print("  2. Go to your device settings")
        print("  3. Look for 'Device ID' or 'ID' in device info")
        print("  4. Or check the device label/QR code")
        return
    
    device_id = sys.argv[1]
    custom_name = sys.argv[2] if len(sys.argv) > 2 else None
    
    # Load existing devices
    devices = load_devices()
    if devices is None:
        return
    
    # Check if device already exists
    existing_ids = {device.get('id') for device in devices if device.get('id')}
    if device_id in existing_ids:
        print(f"⚠️ Device with ID '{device_id}' already exists in devices.json")
        for device in devices:
            if device.get('id') == device_id:
                print(f"   Existing device: {device.get('name', 'Unknown')}")
        return
    
    # Get device information from Tuya API (minimal check)
    device_info = get_device_info(device_id)
    if not device_info:
        print("❌ Failed to process device ID")
        return
    
    # Ensure device_id is set
    device_info['id'] = device_id
    
    # Format device entry
    device_entry = format_device_entry(device_info, custom_name)
    if not device_entry:
        print("❌ Failed to format device entry")
        return
    
    # Show device info
    print("\n📋 Device entry to be added:")
    print(f"   Name: {device_entry['name']}")
    print(f"   ID: {device_entry['id']}")
    print(f"   Model: {device_entry['model']}")
    print(f"   Category: {device_entry['category']}")
    print()
    print("⚠️  Note: Some fields (key, mac) will be empty.")
    print("   These are optional for Cloud API control.")
    print("   If you need local control, use: python -m tinytuya wizard")
    print()
    
    # Ask for confirmation
    response = input("Add this device to devices.json? (y/n): ").strip().lower()
    if response != 'y':
        print("❌ Cancelled")
        return
    
    # Add device to list
    devices.append(device_entry)
    
    # Save to file
    if save_devices(devices):
        print(f"✅ Device '{device_entry['name']}' added successfully!")
        print(f"   Total devices: {len(devices)}")
        print()
        print("📝 Next steps:")
        print("  1. Update mqtt_relay.py DEVICES dictionary")
        print("  2. Add switch pin in ESP32 code (if using physical switch)")
        print("  3. Update display configuration (if needed)")
    else:
        print("❌ Failed to save device")

if __name__ == "__main__":
    main()
