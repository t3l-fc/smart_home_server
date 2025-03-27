# Deploying to Render.com

Render.com offers free web services that are perfect for hosting the Python relay server. Here's how to deploy your server to Render:

## Prerequisites

1. Create a [Render account](https://render.com)
2. Fork this repository to your GitHub account or create a new repository with this code

## Deployment Steps

### 1. Connect your repository

1. Log in to your Render account
2. Click on "New" and select "Web Service"
3. Connect your GitHub account if not already done
4. Select the repository with your smart plug controller code

### 2. Configure the web service

Use the following settings:
- **Name**: Choose a name for your service (e.g., "smart-plug-relay")
- **Environment**: Python
- **Region**: Choose a region close to your location
- **Branch**: main (or whichever branch you want to deploy)
- **Build Command**: `pip install -r requirements.txt`
- **Start Command**: `gunicorn relay_server:app`
- **Plan**: Free

### 3. Set environment variables

In the "Environment" section, add the following environment variables:

- `DEVICE_ID`: Your Tuya device ID (`eb06105cff43c50594rz5n`)
- `ACCESS_ID`: Your Tuya access ID (`4kcffc9h34rwnswpncrj`)
- `ACCESS_KEY`: Your Tuya access key (`d1dc602a4d684f8895e2fca36d8996d8`)
- `REGION`: Your Tuya region (`us`)

If you want to use MQTT, also add:
- `MQTT_BROKER`: Your MQTT broker address
- `MQTT_PORT`: Your MQTT broker port
- `MQTT_USERNAME`: Your MQTT username
- `MQTT_PASSWORD`: Your MQTT password

### 4. Deploy the service

Click "Create Web Service" to deploy your relay server.

### 5. Configure your ESP32

Once deployed, Render will provide you with a URL for your service (something like `https://smart-plug-relay.onrender.com`).

Update the `relayServerUrl` in your ESP32 code with this URL:

```cpp
const char* relayServerUrl = "https://your-service-name.onrender.com";
```

## Maintaining Your Deployment

- Render's free tier may put your service to sleep after periods of inactivity
- The service will wake up automatically when it receives a request
- Initial requests after inactivity may be slower
- You can upgrade to a paid plan for more consistent performance if needed

## Checking Logs

To check logs and debug issues:
1. Go to your web service in the Render dashboard
2. Click on "Logs" to view the server logs
3. Use these logs to troubleshoot any connection issues

## Testing Your Deployment

After deployment, you can test your server using any HTTP client:

```
curl -X GET https://your-service-name.onrender.com/api/status
curl -X POST -H "Content-Type: application/json" -d '{"action":"on"}' https://your-service-name.onrender.com/api/control
``` 