
> [!IMPORTANT]
> I rewrote the firmware in c for better functionality. namely the ability to write a bitmap directly into the framebuffer. Use the firmware_c instead.

Writing directly to the frame buffer isn't easy on the 7.3 inch frame.
See https://github.com/pimoroni/pimoroni-pico/issues/681 and https://github.com/pimoroni/pimoroni-pico/issues/745

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
