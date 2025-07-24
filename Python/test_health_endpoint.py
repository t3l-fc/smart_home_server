#!/usr/bin/env python3
"""
Test both root and /health endpoints
"""

import requests
import json

RENDER_SERVER_URL = "https://smart-home-server-a076.onrender.com"

def test_endpoint(endpoint_path, name):
    """Test a specific endpoint"""
    url = RENDER_SERVER_URL + endpoint_path
    print(f"\n🔍 Testing {name}")
    print(f"   URL: {url}")
    
    try:
        response = requests.get(url, timeout=10)
        print(f"   Status: {response.status_code}")
        
        if response.status_code == 200:
            data = response.json()
            print(f"   ✅ SUCCESS")
            print(f"   Response: {json.dumps(data, indent=4)}")
            return True
        else:
            print(f"   ❌ FAILED - HTTP {response.status_code}")
            print(f"   Response: {response.text}")
            return False
            
    except Exception as e:
        print(f"   ❌ ERROR: {e}")
        return False

def main():
    print("=" * 60)
    print("  Testing Root and Health Endpoints")
    print("=" * 60)
    
    # Test root endpoint
    root_success = test_endpoint("", "Root Endpoint (/)")
    
    # Test health endpoint
    health_success = test_endpoint("/health", "Health Endpoint (/health)")
    
    print(f"\n" + "=" * 60)
    print("  Summary")
    print("=" * 60)
    print(f"Root endpoint (/):      {'✅ PASS' if root_success else '❌ FAIL'}")
    print(f"Health endpoint (/health): {'✅ PASS' if health_success else '❌ FAIL'}")
    
    if root_success and health_success:
        print(f"\n🎉 Both endpoints working! UptimeRobot can now use /health")
        return 0
    else:
        print(f"\n❌ Some endpoints failed")
        return 1

if __name__ == "__main__":
    main() 