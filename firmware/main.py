import gc
import time
from machine import reset
import inky_helper as ih
import nasa_apod as app

# Uncomment the line for your Inky Frame display size
# from picographics import PicoGraphics, DISPLAY_INKY_FRAME_4 as DISPLAY  # 4.0"
# from picographics import PicoGraphics, DISPLAY_INKY_FRAME as DISPLAY      # 5.7"
from picographics import PicoGraphics, DISPLAY_INKY_FRAME_7 as DISPLAY  # 7.3"

# A short delay to give USB chance to initialise
time.sleep(0.5)

# Setup for the display.
graphics = PicoGraphics(DISPLAY)
WIDTH, HEIGHT = graphics.get_bounds()
graphics.set_font("bitmap8")

# Turn any LEDs off that may still be on from last run.
ih.clear_button_leds()
ih.led_warn.off()

# Passes the the graphics object from the launcher to the app
app.graphics = graphics
app.WIDTH = WIDTH
app.HEIGHT = HEIGHT


try:
    from secrets import WIFI_SSID, WIFI_PASSWORD
    ih.network_connect(WIFI_SSID, WIFI_PASSWORD)
except ImportError:
    print("Create secrets.py with your WiFi credentials")

# Get some memory back, we really need it!
gc.collect()

# The main loop executes the update and draw function from the imported app,
# and then goes to sleep ZzzzZZz

while True:
    app.update()
    ih.led_warn.on()
    app.draw()
    ih.led_warn.off()
    ih.sleep(app.UPDATE_INTERVAL)
