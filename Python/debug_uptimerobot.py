#!/usr/bin/env python3
"""
Debug script to simulate UptimeRobot's exact behavior
"""

import requests
import time
import socket
from urllib.parse import urlparse

RENDER_SERVER_URL = "https://smart-home-server-a076.onrender.com/health"

def test_dns_resolution():
    """Test DNS resolution"""
    print("🔍 Testing DNS Resolution")
    try:
        parsed = urlparse(RENDER_SERVER_URL)
        hostname = parsed.hostname
        ip = socket.gethostbyname(hostname)
        print(f"   ✅ {hostname} resolves to {ip}")
        return True
    except Exception as e:
        print(f"   ❌ DNS Error: {e}")
        return False

def test_with_different_user_agents():
    """Test with different User-Agent headers"""
    user_agents = [
        "UptimeRobot/2.0",  # UptimeRobot's actual user agent
        "Mozilla/5.0 (compatible; UptimeRobot/2.0; http://www.uptimerobot.com/)",
        "curl/7.68.0",
        "requests/2.28.1",  # Default requests
        None  # No user agent
    ]
    
    print("\n🔍 Testing Different User-Agent Headers")
    
    for i, ua in enumerate(user_agents):
        print(f"\n   Test {i+1}: {ua or 'No User-Agent'}")
        headers = {'User-Agent': ua} if ua else {}
        
        try:
            response = requests.head(RENDER_SERVER_URL, headers=headers, timeout=30)
            print(f"   Status: {response.status_code}")
            print(f"   Headers: {dict(list(response.headers.items())[:3])}")
            
            if response.status_code == 200:
                print(f"   ✅ SUCCESS")
            else:
                print(f"   ❌ FAILED")
                
        except requests.exceptions.Timeout:
            print(f"   ⏰ TIMEOUT (30s)")
        except Exception as e:
            print(f"   ❌ ERROR: {e}")

def test_connection_details():
    """Test detailed connection information"""
    print("\n🔍 Testing Connection Details")
    
    try:
        # Use a session to see connection details
        session = requests.Session()
        
        # Set UptimeRobot-like headers
        session.headers.update({
            'User-Agent': 'UptimeRobot/2.0',
            'Accept': '*/*',
            'Connection': 'close'
        })
        
        print("   Making HEAD request...")
        start_time = time.time()
        response = session.head(RENDER_SERVER_URL, timeout=30)
        duration = time.time() - start_time
        
        print(f"   Status: {response.status_code}")
        print(f"   Duration: {duration:.2f}s")
        print(f"   Response headers:")
        for key, value in response.headers.items():
            print(f"     {key}: {value}")
            
        return response.status_code == 200
        
    except Exception as e:
        print(f"   ❌ ERROR: {e}")
        return False

def test_from_different_locations():
    """Test using different request configurations"""
    print("\n🔍 Testing Different Request Configurations")
    
    configs = [
        {"verify": True, "allow_redirects": True, "timeout": 30},
        {"verify": False, "allow_redirects": True, "timeout": 30},
        {"verify": True, "allow_redirects": False, "timeout": 30},
        {"verify": True, "allow_redirects": True, "timeout": 60},
    ]
    
    for i, config in enumerate(configs):
        print(f"\n   Config {i+1}: {config}")
        try:
            response = requests.head(RENDER_SERVER_URL, **config)
            print(f"   ✅ Status: {response.status_code}")
        except Exception as e:
            print(f"   ❌ Error: {e}")

def test_alternative_endpoints():
    """Test alternative endpoints"""
    print("\n🔍 Testing Alternative Endpoints")
    
    endpoints = [
        "https://smart-home-server-a076.onrender.com/",
        "https://smart-home-server-a076.onrender.com/health",
        "http://smart-home-server-a076.onrender.com/health",  # HTTP instead of HTTPS
    ]
    
    for endpoint in endpoints:
        print(f"\n   Testing: {endpoint}")
        try:
            response = requests.head(endpoint, timeout=30)
            print(f"   ✅ Status: {response.status_code}")
        except Exception as e:
            print(f"   ❌ Error: {e}")

def main():
    print("=" * 60)
    print("  UptimeRobot Debug Tool")
    print("=" * 60)
    print(f"Target: {RENDER_SERVER_URL}")
    print()
    
    # Run all tests
    test_dns_resolution()
    test_with_different_user_agents()
    test_connection_details()
    test_from_different_locations()
    test_alternative_endpoints()
    
    print("\n" + "=" * 60)
    print("  Recommendations")
    print("=" * 60)
    print("1. Check Render logs for UptimeRobot requests")
    print("2. Consider switching UptimeRobot to GET method")
    print("3. Try changing the URL to root (/) instead of /health")
    print("4. Contact UptimeRobot support about the issue")

if __name__ == "__main__":
    main() 