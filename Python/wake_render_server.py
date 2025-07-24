#!/usr/bin/env python3
"""
Wake up and diagnose Render server
"""

import requests
import time
import sys
from datetime import datetime

RENDER_SERVER_URL = "https://smart-home-server-a076.onrender.com"

def ping_server(attempt=1, max_attempts=5):
    """Ping the server and wait for it to wake up"""
    print(f"\n🔄 Attempt {attempt}/{max_attempts} - Pinging server...")
    print(f"   URL: {RENDER_SERVER_URL}")
    print(f"   Time: {datetime.now().strftime('%H:%M:%S')}")
    
    try:
        start_time = time.time()
        response = requests.get(RENDER_SERVER_URL, timeout=30)
        response_time = time.time() - start_time
        
        if response.status_code == 200:
            print(f"✅ Server is ALIVE! (Response time: {response_time:.2f}s)")
            try:
                data = response.json()
                print(f"   Status: {data.get('status', 'unknown')}")
                print(f"   Service: {data.get('service', 'unknown')}")
                print(f"   MQTT Connected: {data.get('mqtt_connected', 'unknown')}")
                print(f"   Uptime: {data.get('uptime', 0):.0f} seconds")
                return True
            except:
                print(f"   Raw response: {response.text[:200]}")
                return True
        else:
            print(f"❌ Server responded with HTTP {response.status_code}")
            print(f"   Response: {response.text[:200]}")
            return False
            
    except requests.exceptions.Timeout:
        print(f"⏰ Timeout after 30 seconds")
        return False
    except requests.exceptions.ConnectionError as e:
        print(f"🔌 Connection error: {str(e)}")
        return False
    except Exception as e:
        print(f"❌ Error: {str(e)}")
        return False

def main():
    print("=" * 60)
    print("  Render Server Wake-up & Diagnostic Tool")
    print("=" * 60)
    print(f"Target: {RENDER_SERVER_URL}")
    
    # Try multiple times with increasing delays
    for attempt in range(1, 6):
        if ping_server(attempt, 5):
            print(f"\n🎉 SUCCESS! Server is responding after {attempt} attempt(s)")
            return 0
        
        if attempt < 5:
            wait_time = min(30, attempt * 10)  # Wait 10, 20, 30, 30 seconds
            print(f"   Waiting {wait_time} seconds before next attempt...")
            for i in range(wait_time, 0, -5):
                print(f"   {i}s...", end="", flush=True)
                time.sleep(5)
            print()
    
    print(f"\n❌ FAILED: Server did not respond after 5 attempts")
    print(f"\nPossible causes:")
    print(f"  1. Server is sleeping (Render free tier sleeps after 15min inactivity)")
    print(f"  2. Server crashed and needs manual restart")
    print(f"  3. Network/DNS issues")
    print(f"  4. Server deployment failed")
    
    print(f"\nSuggested actions:")
    print(f"  1. Check Render dashboard for logs")
    print(f"  2. Try manual restart from Render dashboard")
    print(f"  3. Check if there are recent deployment errors")
    
    return 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print(f"\n\n⏹️  Interrupted by user")
        sys.exit(1) 