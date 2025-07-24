#!/usr/bin/env python3
"""
Test HEAD and GET methods on health endpoint
"""

import requests

RENDER_SERVER_URL = "https://smart-home-server-a076.onrender.com"

def test_method(method, endpoint_path, name):
    """Test a specific HTTP method on an endpoint"""
    url = RENDER_SERVER_URL + endpoint_path
    print(f"\n🔍 Testing {name}")
    print(f"   URL: {url}")
    print(f"   Method: {method}")
    
    try:
        if method == "HEAD":
            response = requests.head(url, timeout=10)
        else:
            response = requests.get(url, timeout=10)
            
        print(f"   Status: {response.status_code}")
        print(f"   Headers: Content-Type = {response.headers.get('content-type', 'N/A')}")
        
        if method == "GET" and response.status_code == 200:
            print(f"   Body length: {len(response.text)} chars")
            print(f"   Body preview: {response.text[:100]}...")
        elif method == "HEAD":
            print(f"   Body length: {len(response.text)} chars (should be 0 for HEAD)")
        
        if response.status_code == 200:
            print(f"   ✅ SUCCESS")
            return True
        else:
            print(f"   ❌ FAILED")
            return False
            
    except Exception as e:
        print(f"   ❌ ERROR: {e}")
        return False

def main():
    print("=" * 60)
    print("  Testing HEAD and GET Methods")
    print("=" * 60)
    
    # Test GET method
    get_success = test_method("GET", "/health", "GET /health")
    
    # Test HEAD method  
    head_success = test_method("HEAD", "/health", "HEAD /health")
    
    print(f"\n" + "=" * 60)
    print("  Summary")
    print("=" * 60)
    print(f"GET /health:   {'✅ PASS' if get_success else '❌ FAIL'}")
    print(f"HEAD /health:  {'✅ PASS' if head_success else '❌ FAIL'}")
    
    if get_success and head_success:
        print(f"\n🎉 Both methods working! UptimeRobot HEAD requests will work!")
        return 0
    else:
        print(f"\n❌ Some methods failed")
        return 1

if __name__ == "__main__":
    main() 