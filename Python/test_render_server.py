#!/usr/bin/env python3
"""
Test script for Smart Home Server on Render
Tests HTTP health check, MQTT connectivity, and command sending
"""

import requests
import paho.mqtt.client as mqtt
import ssl
import time
import json
import sys
from datetime import datetime

# Configuration (same as your server)
RENDER_SERVER_URL = "https://smart-home-server-a076.onrender.com"
MQTT_BROKER = "io.adafruit.com"
MQTT_PORT = 8883
MQTT_USERNAME = "marsouino"
MQTT_PASSWORD = "2e4dabd28afe424085715d39cb85311a"
MQTT_FEED = "marsouino/feeds/smart_plugs"

# Test results
test_results = {}

def print_header(title):
    """Print a formatted header"""
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")

def print_test_result(test_name, success, details=""):
    """Print test result with status"""
    status = "✅ PASS" if success else "❌ FAIL"
    print(f"{test_name:<30} {status}")
    if details:
        print(f"  → {details}")
    test_results[test_name] = success

def test_http_health_check():
    """Test 1: HTTP Health Check"""
    print_header("Test 1: HTTP Health Check")
    
    try:
        print(f"Testing: {RENDER_SERVER_URL}")
        response = requests.get(RENDER_SERVER_URL, timeout=10)
        
        if response.status_code == 200:
            data = response.json()
            print_test_result("HTTP Health Check", True, 
                            f"Status: {data.get('status', 'unknown')}")
            
            # Print server details
            print(f"  Server Details:")
            print(f"    Service: {data.get('service', 'unknown')}")
            print(f"    MQTT Connected: {data.get('mqtt_connected', 'unknown')}")
            print(f"    Uptime: {data.get('uptime', 0):.0f} seconds")
            
            return True
        else:
            print_test_result("HTTP Health Check", False, 
                            f"HTTP {response.status_code}")
            return False
            
    except requests.exceptions.Timeout:
        print_test_result("HTTP Health Check", False, "Request timeout")
        return False
    except requests.exceptions.ConnectionError:
        print_test_result("HTTP Health Check", False, "Connection error")
        return False
    except Exception as e:
        print_test_result("HTTP Health Check", False, f"Error: {str(e)}")
        return False

def test_mqtt_connection():
    """Test 2: MQTT Connection"""
    print_header("Test 2: MQTT Connection")
    
    connection_success = False
    
    def on_connect(client, userdata, flags, rc):
        nonlocal connection_success
        if rc == 0:
            connection_success = True
            print_test_result("MQTT Connection", True, "Connected to Adafruit IO")
        else:
            print_test_result("MQTT Connection", False, f"Connection failed with code {rc}")
    
    def on_disconnect(client, userdata, rc):
        if rc != 0:
            print(f"  Unexpected disconnection: {rc}")
    
    try:
        client = mqtt.Client(client_id=f"test_render_{int(time.time())}")
        client.on_connect = on_connect
        client.on_disconnect = on_disconnect
        
        # Set authentication
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        
        # Enable SSL/TLS
        client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
        
        print(f"Connecting to {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        
        # Wait for connection
        time.sleep(3)
        
        if connection_success:
            client.loop_stop()
            client.disconnect()
            return True
        else:
            print_test_result("MQTT Connection", False, "Failed to connect within timeout")
            return False
            
    except Exception as e:
        print_test_result("MQTT Connection", False, f"Error: {str(e)}")
        return False

def test_mqtt_command_and_response():
    """Test 3: MQTT Command and Response"""
    print_header("Test 3: MQTT Command and Response")
    
    responses = []
    connected = False
    
    def on_connect(client, userdata, flags, rc):
        nonlocal connected
        if rc == 0:
            connected = True
            client.subscribe(MQTT_FEED)
            print(f"  Connected and subscribed to {MQTT_FEED}")
            
            # Send a test command after short delay
            time.sleep(1)
            command = "cactus:status"
            print(f"  Sending test command: {command}")
            client.publish(MQTT_FEED, command)
        else:
            print(f"  Failed to connect: {rc}")
    
    def on_message(client, userdata, msg):
        nonlocal responses
        try:
            payload = msg.payload.decode()
            print(f"  Received: {payload}")
            
            # Try to parse as JSON (server responses are JSON)
            try:
                data = json.loads(payload)
                responses.append(data)
            except json.JSONDecodeError:
                responses.append(payload)
                
        except Exception as e:
            print(f"  Error processing message: {e}")
    
    try:
        client = mqtt.Client(client_id=f"test_cmd_{int(time.time())}")
        client.on_connect = on_connect
        client.on_message = on_message
        
        # Set authentication
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
        
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        
        # Wait for responses
        print("  Waiting for server response...")
        start_time = time.time()
        while time.time() - start_time < 10:
            if len(responses) > 1:  # Command echo + server response
                break
            time.sleep(0.1)
        
        client.loop_stop()
        client.disconnect()
        
        if len(responses) >= 2:
            # Look for server response (should be JSON)
            server_response = None
            for response in responses:
                if isinstance(response, dict) and 'device' in response:
                    server_response = response
                    break
            
            if server_response:
                print_test_result("MQTT Command/Response", True, 
                                f"Server responded for device '{server_response.get('device')}'")
                print(f"  Server Response: {json.dumps(server_response, indent=4)}")
                return True
            else:
                print_test_result("MQTT Command/Response", False, 
                                "No valid server response received")
                return False
        else:
            print_test_result("MQTT Command/Response", False, 
                            f"Expected 2+ messages, got {len(responses)}")
            return False
            
    except Exception as e:
        print_test_result("MQTT Command/Response", False, f"Error: {str(e)}")
        return False

def test_server_heartbeat():
    """Test 4: Server Heartbeat Monitoring"""
    print_header("Test 4: Server Heartbeat Monitoring")
    
    heartbeat_received = False
    
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            client.subscribe(MQTT_FEED)
            print("  Listening for server heartbeat messages...")
        else:
            print(f"  Failed to connect: {rc}")
    
    def on_message(client, userdata, msg):
        nonlocal heartbeat_received
        try:
            payload = msg.payload.decode()
            if payload.startswith('{'):
                data = json.loads(payload)
                if data.get('status') == 'heartbeat':
                    heartbeat_received = True
                    print(f"  Heartbeat received! Server uptime: {data.get('uptime', 0):.0f}s")
        except:
            pass
    
    try:
        client = mqtt.Client(client_id=f"test_heartbeat_{int(time.time())}")
        client.on_connect = on_connect
        client.on_message = on_message
        
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
        
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        
        # Wait for heartbeat (server sends one every minute)
        print("  Waiting up to 70 seconds for heartbeat...")
        start_time = time.time()
        while time.time() - start_time < 70:
            if heartbeat_received:
                break
            time.sleep(1)
            if int(time.time() - start_time) % 10 == 0:
                print(f"  Still waiting... ({int(time.time() - start_time)}s)")
        
        client.loop_stop()
        client.disconnect()
        
        if heartbeat_received:
            print_test_result("Server Heartbeat", True, "Heartbeat detected")
            return True
        else:
            print_test_result("Server Heartbeat", False, "No heartbeat received in 70s")
            return False
            
    except Exception as e:
        print_test_result("Server Heartbeat", False, f"Error: {str(e)}")
        return False

def print_summary():
    """Print test summary"""
    print_header("Test Summary")
    
    total_tests = len(test_results)
    passed_tests = sum(test_results.values())
    failed_tests = total_tests - passed_tests
    
    print(f"Total Tests: {total_tests}")
    print(f"Passed: {passed_tests} ✅")
    print(f"Failed: {failed_tests} ❌")
    print(f"Success Rate: {(passed_tests/total_tests)*100:.1f}%")
    
    if failed_tests > 0:
        print(f"\nFailed Tests:")
        for test_name, result in test_results.items():
            if not result:
                print(f"  - {test_name}")
    
    print(f"\nTest completed at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    return failed_tests == 0

def main():
    """Main test function"""
    print_header("Smart Home Server Test Suite")
    print(f"Testing server: {RENDER_SERVER_URL}")
    print(f"Testing MQTT: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"Start time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Run all tests
    tests = [
        test_http_health_check,
        test_mqtt_connection,
        test_mqtt_command_and_response,
        test_server_heartbeat
    ]
    
    for test_func in tests:
        try:
            test_func()
        except KeyboardInterrupt:
            print("\n\nTest interrupted by user.")
            break
        except Exception as e:
            print(f"\nUnexpected error in {test_func.__name__}: {e}")
    
    # Print summary
    all_passed = print_summary()
    
    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main()) 