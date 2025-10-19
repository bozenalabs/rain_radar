## inky frame

https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555

https://github.com/pimoroni/inky-frame

to get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.

the vscode micropico plugin is good. Read the getting started for it.
workflow:
- run the Upload project to Pico command
- the go to main.py, and Run current file on Pico. you get logs this way


### secrets.py
> [!IMPORTANT]
> Create a file named `secrets.py` in the `firmware` folder.

example:

```python
# firmware/secrets.py
WIFI_SSID = "your-ssid"
WIFI_PASSWORD = "your-password"
```


### building the image
https://tile.openstreetmap.org/9/255/170.png

https://doc.rainbow.ai/api-ref/tiles/

See API usage:
https://developer.rainbow.ai/reports


### hosting data

using tailscale funnel
