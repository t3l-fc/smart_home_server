#!/usr/bin/env python3
"""
Smart Plug Controller - Main Entry Point
"""

import os
import sys
from relay_server import app

def main():
    """Main entry point for the Smart Plug Controller"""
    print("Starting Smart Plug Controller Relay Server...")
    
    # Get port from environment variable or use default
    port = int(os.environ.get('PORT', 5000))
    
    # Check if running in debug mode
    debug_mode = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
    
    # Start the Flask app
    app.run(host='0.0.0.0', port=port, debug=debug_mode)

if __name__ == "__main__":
    sys.exit(main())
