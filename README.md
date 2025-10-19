## inky frame

https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555

https://github.com/pimoroni/inky-frame

to get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.

the vscode micropico plugin is good. Read the getting started for it.


### secrets.py
> [!IMPORTANT]
> Create a file named `secrets.py` in the `firmware` folder.

example:

```python
# firmware/secrets.py

WIFI_SSID = "your-ssid"
WIFI_PASSWORD = "your-password"

# Optional services
MQTT_BROKER = "mqtt.example.com"
MQTT_USER = "mqtt-user"
MQTT_PASSWORD = "mqtt-password"

OPENWEATHER_API_KEY = "your-openweather-api-key"
```
