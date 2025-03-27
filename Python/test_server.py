#!/usr/bin/env python3
"""
Test script for the Tuya Smart Plug Relay Server
"""

import argparse
import json
import requests
import sys
import time

def main():
    parser = argparse.ArgumentParser(description='Test the Smart Plug Relay Server')
    parser.add_argument('--url', default='http://localhost:5000', help='URL of the relay server')
    parser.add_argument('--action', choices=['status', 'on', 'off', 'toggle'], default='status', 
                      help='Action to perform')
    args = parser.parse_args()
    
    base_url = args.url.rstrip('/')
    
    try:
        # Check if server is running
        print(f"Testing connection to {base_url}...")
        response = requests.get(f"{base_url}/health", timeout=10)
        response.raise_for_status()
        
        print(f"Server is running: {response.json()}")
        
        # Get current status
        if args.action in ['status', 'toggle']:
            print(f"\nGetting current status...")
            response = requests.get(f"{base_url}/api/status", timeout=10)
            response.raise_for_status()
            status_data = response.json()
            print(f"Response: {json.dumps(status_data, indent=2)}")
            
            # Extract current state if available
            current_state = None
            try:
                if status_data.get('status') == 'success':
                    result = status_data.get('result', {})
                    if 'result' in result and 'switch_1' in result['result']:
                        current_state = result['result']['switch_1']
                        print(f"Current state: {'ON' if current_state else 'OFF'}")
            except Exception as e:
                print(f"Error parsing status: {e}")
        
        # Send control command
        if args.action in ['on', 'off'] or (args.action == 'toggle' and current_state is not None):
            # For toggle, invert the current state
            if args.action == 'toggle':
                action = 'off' if current_state else 'on'
            else:
                action = args.action
                
            print(f"\nSending {action.upper()} command...")
            response = requests.post(
                f"{base_url}/api/control",
                json={"action": action},
                timeout=10
            )
            response.raise_for_status()
            control_data = response.json()
            print(f"Response: {json.dumps(control_data, indent=2)}")
            
            # Verify the state changed
            time.sleep(2)  # Wait for the state to change
            print("\nVerifying state change...")
            response = requests.get(f"{base_url}/api/status", timeout=10)
            response.raise_for_status()
            verify_data = response.json()
            
            try:
                if verify_data.get('status') == 'success':
                    result = verify_data.get('result', {})
                    if 'result' in result and 'switch_1' in result['result']:
                        new_state = result['result']['switch_1']
                        expected_state = (action == 'on')
                        if new_state == expected_state:
                            print(f"✅ State successfully changed to {'ON' if new_state else 'OFF'}")
                        else:
                            print(f"❌ State verification failed. Expected: {'ON' if expected_state else 'OFF'}, Got: {'ON' if new_state else 'OFF'}")
            except Exception as e:
                print(f"Error verifying state: {e}")
        
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

if __name__ == "__main__":
    sys.exit(main()) 