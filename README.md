## inky frame

https://shop.pimoroni.com/products/inky-frame-7-3?variant=40541882089555

https://github.com/pimoroni/inky-frame

to get bootloader mode, hold BOOTSEL on the rpi and tap reset on the frame.

the vscode micropico plugin is good. Read the getting started for it.
workflow:
- run the Upload project to Pico command
- the go to main.py, and Run current file on Pico. you get logs this way

Im trying to write in c following this https://learn.pimoroni.com/article/pico-development-using-wsl.
Ive made a docker file for it.
Writing it in C should make it possible to read an image from the server directly into the graphics buffer.
You can't do this for the 7.3 inch frame.
See https://github.com/pimoroni/pimoroni-pico/issues/681 and https://github.com/pimoroni/pimoroni-pico/issues/745


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
