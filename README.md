## inky frame

https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555

https://github.com/pimoroni/inky-frame

to get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.

the vscode micropico plugin is good. Read the getting started for it.


> **Important:** create a file named `secrets.py` in the `firmware` folder and do NOT commit it.

Example `firmware/secrets.py` (use your real values, and keep this file out of version control):

```python
# firmware/secrets.py
# Fill in your credentials and API keys. DO NOT commit this file.

WIFI_SSID = "your-ssid"
WIFI_PASSWORD = "your-password"

# Optional services
MQTT_BROKER = "mqtt.example.com"
MQTT_USER = "mqtt-user"
MQTT_PASSWORD = "mqtt-password"

OPENWEATHER_API_KEY = "your-openweather-api-key"
```

Add `firmware/secrets.py` to your `.gitignore` so secrets never get pushed.