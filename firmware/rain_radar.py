import gc
import jpegdec
from urllib import urequest
from ujson import load

gc.collect()

graphics = None
WIDTH = None
HEIGHT = None

IMAGE_FILE_NAME = "rain_radar.jpg"
IMAGE_INFO_FILE_NAME = "image_info.json"

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
    JSON_URL = "https://muse-hub.taile8f45.ts.net/image_info.json"

    try:
        # Grab the image
        socket = urequest.urlopen(IMG_URL)
        gc.collect()
        data = bytearray(1024)
        with open(IMAGE_FILE_NAME, "wb") as f:
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
    else:
        try:
            # Grab the image info JSON
            socket = urequest.urlopen(JSON_URL)
            gc.collect()
            data = bytearray(1024)
            with open(IMAGE_INFO_FILE_NAME, "wb") as f:
                while True:
                    if socket.readinto(data) == 0:
                        break
                    f.write(data)
            socket.close()
            del data
            gc.collect()
            print("Downloaded image info")
        except OSError as e:
            print(e)
            show_error("Unable to download image info")

def open_image_info():
    with open(IMAGE_INFO_FILE_NAME, "r") as f:
        info = load(f)
        return info


def draw():
    # TODO: https://github.com/pimoroni/inky-frame/blob/main/examples/display_png.py
    jpeg = jpegdec.JPEG(graphics)
    gc.collect()  # For good measure...

    graphics.set_pen(WHITE)
    graphics.clear()

    try:
        jpeg.open_file(IMAGE_FILE_NAME)
        jpeg.decode()
    except OSError:
        graphics.set_pen(RED)
        graphics.rectangle(0, (HEIGHT // 2) - 20, WIDTH, 40)
        graphics.set_pen(WHITE)
        graphics.text("Unable to display image!", 5, (HEIGHT // 2) - 15, WIDTH, 2)
        graphics.text("Check your network settings in secrets.py", 5, (HEIGHT // 2) + 2, WIDTH, 2)

    image_info = open_image_info

    graphics.set_pen(BLACK)
    graphics.rectangle(0, HEIGHT - 25, WIDTH, 25)
    graphics.set_pen(WHITE)
    graphics.text(image_info["text"], 5, HEIGHT - 20, WIDTH, 2)
   
    gc.collect()
    graphics.update()
