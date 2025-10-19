import gc
import jpegdec
from urllib import urequest
from ujson import load

gc.collect()

graphics = None
WIDTH = None
HEIGHT = None

FILENAME = "nasa-apod-daily.jpg"

# Length of time between updates in minutes.
# Frequent updates will reduce battery life!
UPDATE_INTERVAL = 240

BLACK = 0
WHITE = 1
GREEN = 2
BLUE = 3
RED = 4
YELLOW = 5
ORANGE = 6
TAUPE = 7


def show_error(text):
    graphics.set_pen(4)
    graphics.rectangle(0, 10, WIDTH, 35)
    graphics.set_pen(1)
    graphics.text(text, 5, 16, 400, 2)


def update():
    IMG_URL = "https://muse-hub.taile8f45.ts.net/combined.jpg"

    try:
        # Grab the image
        socket = urequest.urlopen(IMG_URL)

        gc.collect()

        data = bytearray(1024)
        with open(FILENAME, "wb") as f:
            while True:
                if socket.readinto(data) == 0:
                    break
                f.write(data)
        socket.close()
        del data
        gc.collect()
    except OSError as e:
        print(e)
        show_error("Unable to download image")


def draw():
    jpeg = jpegdec.JPEG(graphics)
    gc.collect()  # For good measure...

    graphics.set_pen(WHITE)
    graphics.clear()

    try:
        jpeg.open_file(FILENAME)
        jpeg.decode()
    except OSError:
        graphics.set_pen(RED)
        graphics.rectangle(0, (HEIGHT // 2) - 20, WIDTH, 40)
        graphics.set_pen(WHITE)
        graphics.text("Unable to display image!", 5, (HEIGHT // 2) - 15, WIDTH, 2)
        graphics.text("Check your network settings in secrets.py", 5, (HEIGHT // 2) + 2, WIDTH, 2)

    graphics.set_pen(BLACK)
    graphics.rectangle(0, HEIGHT - 25, WIDTH, 25)
    graphics.set_pen(WHITE)
    graphics.text("testing2", 5, HEIGHT - 20, WIDTH, 2)
   
    gc.collect()
    graphics.update()
