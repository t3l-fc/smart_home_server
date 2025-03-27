# Smart Plug Controller

A Python-based relay server to control a Teckin smart plug (via Tuya API) from an ESP32 Feather HUZZAH32.

## Overview

This project consists of:
1. A Python relay server that can be deployed to a cloud platform
2. Arduino code for the ESP32 that communicates with the relay server

The relay server provides HTTP and MQTT interfaces to control the smart plug.

## Setup for the Python Relay Server

### Local Development

1. Clone this repository
2. Create a virtual environment: `python -m venv venv`
3. Activate the virtual environment:
   - Windows: `venv\Scripts\activate`
   - macOS/Linux: `source venv/bin/activate`
4. Install dependencies: `pip install -r requirements.txt`
5. Configure your environment variables in the `.env` file
6. Run the server: `python relay_server.py`

### Deployment Options

#### Deploy to Heroku

1. Install the [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli)
2. Login to Heroku: `heroku login`
3. Create a Heroku app: `heroku create your-app-name`
4. Set environment variables:
   ```
   heroku config:set DEVICE_ID=your-device-id
   heroku config:set ACCESS_ID=your-access-id
   heroku config:set ACCESS_KEY=your-access-key
   heroku config:set REGION=your-region
   ```
5. Deploy: `git push heroku main`

#### Deploy to Render

1. Create a Render account
2. Connect your GitHub repository
3. Create a new Web Service
4. Set the build command: `pip install -r requirements.txt`
5. Set the start command: `gunicorn relay_server:app`
6. Add environment variables in the Render dashboard

#### Deploy to PythonAnywhere

1. Create a PythonAnywhere account
2. Upload your files using the PythonAnywhere dashboard
3. Create a new web app using Flask
4. Configure the WSGI file to point to your `relay_server.py`
5. Set environment variables through the PythonAnywhere dashboard

## API Endpoints

- `GET /`: Health check for the server
- `GET /health`: Health check endpoint
- `GET /api/status`: Get the current status of the smart plug
- `POST /api/control`: Control the smart plug
  - Request body: `{"action": "on"}` or `{"action": "off"}`

## MQTT Interface

If configured, the relay server can communicate via MQTT:

- Publish to topic `tuya/command` with payload `{"action": "on"}` or `{"action": "off"}` to control the smart plug
- Subscribe to topic `tuya/status` to receive status updates

## ESP32 Setup

See the Arduino sketch in the `arduino` directory for code to run on the ESP32 Feather HUZZAH32. 