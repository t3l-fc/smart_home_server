#!/usr/bin/env python3
"""
Test script for the Tuya Smart Plug Relay Server with multiple devices
"""

import argparse
import json
import requests
import sys
import time

def test_server(base_url="https://smart-home-server-a076.onrender.com", action=None, device=None):
    base_url = base_url.rstrip('/')
    
    try:
        # Check if server is running
        print(f"Testing connection to {base_url}...")
        response = requests.get(f"{base_url}/health", timeout=10)
        response.raise_for_status()
        print(f"Server is running: {response.json()}")
        
        # Get list of available devices
        print("\nGetting list of devices...")
        response = requests.get(f"{base_url}/api/devices", timeout=10)
        response.raise_for_status()
        devices_data = response.json()
        print("Available devices:")
        print(json.dumps(devices_data, indent=2))
        
        # If no specific action requested, stop here
        if not action:
            return 0
            
        # Handle status checks
        if action == "status":
            if device and device != "all":
                # Get status of specific device
                print(f"\nGetting status for device: {device}")
                response = requests.get(f"{base_url}/api/status/{device}", timeout=10)
                response.raise_for_status()
                status_data = response.json()
                print("Response:")
                print(json.dumps(status_data, indent=2))
            else:
                # Get status of all devices
                print("\nGetting status for all devices...")
                response = requests.get(f"{base_url}/api/status/all", timeout=10)
                response.raise_for_status()
                status_data = response.json()
                print("Response:")
                print(json.dumps(status_data, indent=2))
                
        # Handle control commands
        elif action in ["on", "off"]:
            if device and device != "all":
                # Control specific device
                print(f"\nSending {action.upper()} command to device: {device}")
                response = requests.post(
                    f"{base_url}/api/control/{device}",
                    json={"action": action},
                    timeout=10
                )
                response.raise_for_status()
                control_data = response.json()
                print("Response:")
                print(json.dumps(control_data, indent=2))
            else:
                # Control all devices
                print(f"\nSending {action.upper()} command to all devices...")
                response = requests.post(
                    f"{base_url}/api/control/all",
                    json={"action": action},
                    timeout=10
                )
                response.raise_for_status()
                control_data = response.json()
                print("Response:")
                print(json.dumps(control_data, indent=2))
            
            # Verify the state change after a brief delay
            time.sleep(2)
            print("\nVerifying state change...")
            if device and device != "all":
                response = requests.get(f"{base_url}/api/status/{device}", timeout=10)
            else:
                response = requests.get(f"{base_url}/api/status/all", timeout=10)
            response.raise_for_status()
            verify_data = response.json()
            print("Current status:")
            print(json.dumps(verify_data, indent=2))
            
    except requests.exceptions.ConnectionError:
        print(f"Error: Could not connect to the server at {base_url}")
        print("Make sure the server is running and the URL is correct.")
        return 1
    except requests.exceptions.Timeout:
        print(f"Error: Connection to {base_url} timed out")
        print("The server might be starting up or experiencing issues.")
        return 1
    except requests.exceptions.HTTPError as e:
        print(f"Error: HTTP error occurred: {e}")
        return 1
    except Exception as e:
        print(f"Error: An unexpected error occurred: {e}")
        return 1
        
    return 0

def main():
    parser = argparse.ArgumentParser(description='Test the Smart Plug Relay Server')
    parser.add_argument('--url', default='http://localhost:5000', 
                      help='URL of the relay server')
    parser.add_argument('--action', choices=['status', 'on', 'off'], 
                      help='Action to perform')
    parser.add_argument('--device', 
                      help='Device ID or name (use "all" for all devices)')
    args = parser.parse_args()
    
    return test_server(args.url, args.action, args.device)

if __name__ == "__main__":
    sys.exit(main()) 