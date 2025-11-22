#!/usr/bin/env python3
"""
Script to discover new Tuya devices and add them to devices.json
"""

import json
import tinytuya

# Tuya Configuration (from your existing setup)
TUYA_API_REGION = "us"
TUYA_API_KEY = "4kcffc9h34rwnswpncrj"
TUYA_API_SECRET = "d1dc602a4d684f8895e2fca36d8996d8"

def discover_devices():
    """Discover all Tuya devices on your account"""
    print("🔍 Discovering Tuya devices...")
    print("⚠️  Note: Tuya Cloud API requires deviceID for getdevices()")
    print("    Using alternative method: tinytuya wizard or manual entry")
    print()
    
    # Try using the wizard approach or direct API call
    # The Cloud API getdevices() seems to need a deviceID parameter
    # Let's try a different approach - use the devices we already have as reference
    
    print("💡 Recommended approach:")
    print("   1. Use the Tuya Smart Life app to find your new device")
    print("   2. Get the device ID from the app (usually in device settings)")
    print("   3. Use this script with --details <device_id> to get full info")
    print()
    
    # Try to get devices using the existing device IDs as a test
    existing_devices = load_existing_devices()
    if existing_devices:
        print(f"📋 Found {len(existing_devices)} existing devices in devices.json:")
        for device in existing_devices:
            print(f"  - {device.get('name', 'Unknown')} (ID: {device.get('id', 'Unknown')})")
        print()
    
    # Try alternative: use getdevice() with one of the existing IDs to test connection
    if existing_devices:
        test_id = existing_devices[0].get('id')
        print(f"🔍 Testing API connection with existing device: {test_id}...")
        try:
            cloud = tinytuya.Cloud(
                apiRegion=TUYA_API_REGION,
                apiKey=TUYA_API_KEY,
                apiSecret=TUYA_API_SECRET
            )
            test_result = cloud.getdevice(test_id)
            if test_result and isinstance(test_result, dict):
                print("✅ API connection successful!")
                print("   You can now use --details <device_id> to get info about any device")
                return []
            else:
                print(f"⚠️ API returned: {test_result}")
        except Exception as e:
            print(f"❌ API test failed: {e}")
            print("   Please check your API credentials")
    
    return []

def load_existing_devices():
    """Load existing devices from devices.json"""
    try:
        with open('devices.json', 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print("⚠️ devices.json not found")
        return []

def find_new_devices(all_devices, existing_devices):
    """Find devices that are not in devices.json"""
    existing_ids = {device['id'] for device in existing_devices}
    new_devices = [device for device in all_devices if device.get('id') not in existing_ids]
    
    return new_devices

def get_device_details(device_id):
    """Get detailed information about a specific device"""
    print(f"🔍 Getting details for device {device_id}...")
    
    cloud = tinytuya.Cloud(
        apiRegion=TUYA_API_REGION,
        apiKey=TUYA_API_KEY,
        apiSecret=TUYA_API_SECRET
    )
    
    try:
        # Get device details
        details = cloud.getdevice(device_id)
        
        if details:
            print("✅ Device details retrieved:")
            if isinstance(details, dict):
                print(json.dumps(details, indent=2))
            else:
                print(f"Response type: {type(details)}")
                print(f"Response: {details}")
            return details
        else:
            print("❌ Failed to get device details (empty response)")
            return None
    except Exception as e:
        print(f"❌ Error getting device details: {e}")
        return None

def format_device_for_json(device_details):
    """Format device details for devices.json"""
    if not device_details:
        return None
    
    # Create the device entry in the same format as existing devices
    device_entry = {
        "name": device_details.get('name', 'New Device'),
        "id": device_details.get('id', ''),
        "key": device_details.get('key', ''),
        "mac": device_details.get('mac', ''),
        "uuid": device_details.get('uuid', ''),
        "category": device_details.get('category', 'cz'),
        "product_name": device_details.get('product_name', 'Smart Socket'),
        "product_id": device_details.get('product_id', ''),
        "biz_type": device_details.get('biz_type', 18),
        "model": device_details.get('model', ''),
        "sub": device_details.get('sub', False),
        "icon": device_details.get('icon', ''),
        "mapping": device_details.get('mapping', {
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
        }),
        "ip": device_details.get('ip', ''),
        "version": device_details.get('version', '')
    }
    
    return device_entry

def main():
    print("=" * 60)
    print("  Tuya Device Discovery Tool")
    print("=" * 60)
    
    # Discover all devices
    all_devices = discover_devices()
    if not all_devices:
        print("\n💡 Tip: If no devices were found, check:")
        print("  1. Your Tuya API credentials are correct")
        print("  2. Your devices are registered in the Tuya app")
        print("  3. Try running with --debug to see raw API response")
        return
    
    # Load existing devices
    existing_devices = load_existing_devices()
    print(f"📋 Currently configured devices: {len(existing_devices)}")
    for device in existing_devices:
        print(f"  - {device.get('name', 'Unknown')} (ID: {device.get('id', 'Unknown')})")
    
    # Find new devices
    new_devices = find_new_devices(all_devices, existing_devices)
    
    if not new_devices:
        print("\n✅ No new devices found - all devices are already configured")
        return
    
    print(f"\n🆕 Found {len(new_devices)} new device(s):")
    valid_new_devices = []
    for i, device in enumerate(new_devices, 1):
        if isinstance(device, dict):
            name = device.get('name', 'Unknown')
            device_id = device.get('id', 'Unknown')
            print(f"  {i}. {name} (ID: {device_id})")
            valid_new_devices.append(device)
        else:
            print(f"  {i}. {device} (⚠️ Not in dict format)")
    
    if valid_new_devices:
        print("\n💡 To add a new device manually:")
        print("  1. Copy the device ID from above")
        print("  2. Get full device details using: python discover_new_device.py --details <device_id>")
        print("  3. Add the device entry to devices.json following the same format")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 2:
        if sys.argv[1] == "--details":
            device_id = sys.argv[2]
            cloud = tinytuya.Cloud(
                apiRegion=TUYA_API_REGION,
                apiKey=TUYA_API_KEY,
                apiSecret=TUYA_API_SECRET
            )
            details = get_device_details(device_id)
            if details and isinstance(details, dict):
                device_entry = format_device_for_json(details)
                if device_entry:
                    print("\n" + "=" * 60)
                    print("  Device JSON Entry (copy to devices.json)")
                    print("=" * 60)
                    print(json.dumps(device_entry, indent=4))
                    print("=" * 60)
        elif sys.argv[1] == "--debug":
            print("🔍 Debug mode - showing raw API response...")
            cloud = tinytuya.Cloud(
                apiRegion=TUYA_API_REGION,
                apiKey=TUYA_API_KEY,
                apiSecret=TUYA_API_SECRET
            )
            devices = cloud.getdevices()
            print(f"\nRaw response type: {type(devices)}")
            print(f"Raw response: {json.dumps(devices, indent=2) if isinstance(devices, (dict, list)) else devices}")
        elif sys.argv[1] == "--add":
            try:
                device_num = int(sys.argv[2])
                print(f"Adding device {device_num} (feature to be implemented)")
            except ValueError:
                print("❌ Invalid device number")
    else:
        main()
